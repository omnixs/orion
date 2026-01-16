/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modifications: This file has been modified by ORION contributors. See VCS history.
 */

#include <orion/api/engine.hpp>
#include <orion/bre/dmn_model.hpp>
#include <orion/bre/dmn_parser.hpp>
#include <orion/bre/drg_evaluator.hpp>
#include <orion/api/logger.hpp>
#include <orion/bre/evaluation_context.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <orion/bre/contract_violation.hpp>
#include <expected>
#include <stdexcept>
#include <set>
#include <orion/bre/bkm_manager.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using namespace std;
using json = nlohmann::json;

namespace orion
{
    namespace api
    {
        // Import BRE types into api namespace for implementation
        using bre::DecisionTable;
        using bre::BKMManager;
        using bre::LiteralDecision;
        using bre::BusinessKnowledgeModel;
        using bre::DmnParser;
        using bre::DRGEvaluator;

        // Pimpl implementation class
        class BusinessRulesEngine::Impl
        {
        public:
            std::map<std::string, std::unique_ptr<DecisionTable>> decision_tables_;
            BKMManager bkm_manager_; // Use BKMManager instead of raw map
            std::map<std::string, std::unique_ptr<LiteralDecision>> literal_decisions_;
            std::unique_ptr<DRGEvaluator> drg_evaluator_; // Decision Requirement Graph evaluator (optional)
            std::vector<bre::Decision> decisions_; // Store parsed decisions for DRG
            std::string namespace_uri_; // Store namespace from DMN model
            bre::feel::RegexCache regex_cache_; // Regex cache for FEEL evaluation

            // Helper methods
            [[nodiscard]] nlohmann::json resolve_variable(std::string_view name, const nlohmann::json& context) const;
            [[nodiscard]] std::string format_result(std::string_view decision_name, const nlohmann::json& result) const;
            
            // Evaluation helper methods
            void evaluate_with_drg(const nlohmann::json& data, nlohmann::json& results, bre::EvaluationContext& eval_ctx) const;
            void evaluate_without_drg(const nlohmann::json& data, nlohmann::json& results, bre::EvaluationContext& eval_ctx) const;
            bool try_evaluate_decision_table(std::string_view name, const nlohmann::json& context, bre::EvaluationContext& eval_ctx, nlohmann::json& result) const;
            bool try_evaluate_literal_decision(std::string_view name, const nlohmann::json& context, bre::EvaluationContext& eval_ctx, nlohmann::json& result) const;

            // Internal component management (moved from public API)
            void add_decision_table(std::unique_ptr<DecisionTable> table);
            void add_business_knowledge_model(std::unique_ptr<BusinessKnowledgeModel> bkm);
            void add_literal_decision(std::unique_ptr<LiteralDecision> decision);
        };

        // Constructor/Destructor
        BusinessRulesEngine::BusinessRulesEngine() : pimpl(std::make_unique<Impl>()) {}
        BusinessRulesEngine::~BusinessRulesEngine() = default;

        // Move constructor and assignment
        BusinessRulesEngine::BusinessRulesEngine(BusinessRulesEngine&&) noexcept = default;
        BusinessRulesEngine& BusinessRulesEngine::operator=(BusinessRulesEngine&&) noexcept = default;

        // BusinessRulesEngine implementation
        std::expected<void, string> BusinessRulesEngine::load_dmn_model(string_view dmn_xml)
        {
            if (dmn_xml.empty()) [[unlikely]]
            {
                return std::unexpected(string("DMN XML cannot be empty"));
            }

            try
            {
                // Parse the entire DMN model to get all decisions
                DmnParser parser;
                auto model = parser.parse(dmn_xml);
                
                pimpl->namespace_uri_ = model.namespace_uri;
                
                // Process decisions into tables/literal decisions FIRST
                // (extract/move decision tables before DRG analyzes structure)
                // DRG only needs IDs and informationRequirements, not the actual tables
                for (auto& decision : model.decisions)
                {
                    // Handle decision table if present
                    if (decision.decisionTable.has_value())
                    {
                        auto dTable = make_unique<DecisionTable>(std::move(decision.decisionTable.value()));
                        dTable->name = decision.name;
                        pimpl->add_decision_table(std::move(dTable));
                    }
                    
                    // Handle literal decisions (FEEL expressions)
                    else if (!decision.expression.empty())
                    {
                        auto literalDec = make_unique<LiteralDecision>();
                        literalDec->name = decision.name;
                        literalDec->expression_text = decision.expression;
                        pimpl->add_literal_decision(std::move(literalDec));
                    }
                }
                
                // Try to build DRG if there are dependencies (takes ownership)
                // Decision tables are already extracted, but IDs/names/requirements remain
                // Let all exceptions propagate - caller should handle DRG construction errors
                pimpl->drg_evaluator_ = std::make_unique<DRGEvaluator>(std::move(model.decisions));

                // Parse all Business Knowledge Models using BKMManager
                string temp_error;
                pimpl->bkm_manager_.load_bkm_from_dmn(dmn_xml, temp_error, "");
                // BKM parsing is optional, continue if none found

                return {};
            }
            catch (const orion::bre::ContractViolation&)
            {
                // Re-throw contract violations (critical errors that should not be caught)
                throw;
            }
            catch (const exception& e)
            {
                return std::unexpected(string(e.what()));
            }
        }

        string BusinessRulesEngine::evaluate(string_view data_json) const
        {
            json data = json::parse(data_json);
            json results = json::object();
            bre::EvaluationContext eval_ctx{pimpl->regex_cache_};
            
            if (pimpl->drg_evaluator_)
            {
                pimpl->evaluate_with_drg(data, results, eval_ctx);
            }
            else
            {
                pimpl->evaluate_without_drg(data, results, eval_ctx);
            }
            
            return results.dump();
        }

        vector<string> BusinessRulesEngine::get_decision_table_names() const
        {
            vector<string> names;
            for (const auto& [name, _] : pimpl->decision_tables_)
            {
                names.push_back(name);
            }
            return names;
        }

        vector<string> BusinessRulesEngine::get_business_knowledge_model_names() const
        {
            return pimpl->bkm_manager_.get_bkm_names();
        }

        vector<string> BusinessRulesEngine::get_literal_decision_names() const
        {
            vector<string> names;
            for (const auto& [name, _] : pimpl->literal_decisions_)
            {
                names.push_back(name);
            }
            return names;
        }

        bool BusinessRulesEngine::remove_decision_table(string_view name)
        {
            return pimpl->decision_tables_.erase(string(name)) > 0;
        }

        bool BusinessRulesEngine::remove_business_knowledge_model(string_view name)
        {
            return pimpl->bkm_manager_.remove_bkm(name);
        }

        bool BusinessRulesEngine::remove_literal_decision(string_view name)
        {
            return pimpl->literal_decisions_.erase(string(name)) > 0;
        }

        void BusinessRulesEngine::clear()
        {
            pimpl->decision_tables_.clear();
            pimpl->bkm_manager_.clear();
            pimpl->literal_decisions_.clear();
            pimpl->namespace_uri_.clear();
        }
        
        string BusinessRulesEngine::get_namespace() const
        {
            return pimpl->namespace_uri_;
        }

        vector<string> BusinessRulesEngine::validate_models() const
        {
            // Future enhancement: Implement DMN model validation
            // Could validate: decision table structure, hit policies, expression syntax, etc.
            return {}; // No validation errors
        }

        // Impl helper methods implementation
        void BusinessRulesEngine::Impl::evaluate_with_drg(const json& data, json& results, bre::EvaluationContext& eval_ctx) const
        {
            // Create augmented context for cascading evaluations (don't modify original data)
            json augmented_context = data;
            
            // Get evaluation order for all decisions
            std::vector<std::string> eval_order;
            try
            {
                eval_order = drg_evaluator_->get_evaluation_order();
            }
            catch (const std::exception&)
            {
                // Fall back to independent evaluation
                eval_order.clear();
                for (const auto& [name, _] : decision_tables_)
                    eval_order.push_back(name);
                for (const auto& [name, _] : literal_decisions_)
                    eval_order.push_back(name);
            }
            
            // Evaluate decisions in order, augmenting context as we go
            for (const auto& name : eval_order)
            {
                json result;
                
                // Try decision table first
                if (try_evaluate_decision_table(name, augmented_context, eval_ctx, result))
                {
                    results[name] = result;
                    augmented_context[name] = result;  // Add successful evaluations to context for dependent decisions
                    continue;
                }
                
                // Try literal decision
                bool literal_success = try_evaluate_literal_decision(name, augmented_context, eval_ctx, result);
                
                // Always add result to output (even if evaluation failed - DMN spec requires error results)
                results[name] = result;
                
                // Only add successful evaluations to augmented context for dependent decisions
                if (literal_success)
                {
                    augmented_context[name] = result;
                }
            }
        }
        
        void BusinessRulesEngine::Impl::evaluate_without_drg(const json& data, json& results, bre::EvaluationContext& eval_ctx) const
        {
            // Evaluate decision tables
            for (const auto& [name, table] : decision_tables_)
            {
                json result;
                if (!try_evaluate_decision_table(name, data, eval_ctx, result))
                {
                    result = json{};  // Set empty result on error
                }
                results[name] = result;
            }
            
            // Evaluate literal decisions
            for (const auto& [name, decision] : literal_decisions_)
            {
                json result;
                if (!try_evaluate_literal_decision(name, data, eval_ctx, result))
                {
                    result = json{};  // Set empty result on error
                }
                results[name] = result;
            }
        }
        
        bool BusinessRulesEngine::Impl::try_evaluate_decision_table(string_view name, const json& context, bre::EvaluationContext& eval_ctx, json& result) const
        {
            auto it = decision_tables_.find(string(name));
            if (it == decision_tables_.end())
            {
                return false;
            }
            
            try
            {
                result = it->second->evaluate(context, eval_ctx);
                return true;  // Success - result is valid
            }
            catch (const exception&)
            {
                result = json{};  // Set empty on error
                return false;  // Failure - error occurred
            }
        }
        
        bool BusinessRulesEngine::Impl::try_evaluate_literal_decision(string_view name, const json& context, bre::EvaluationContext& eval_ctx, json& result) const
        {
            auto it = literal_decisions_.find(string(name));
            if (it == literal_decisions_.end())
            {
                return false;
            }
            
            try
            {
                auto bkm_map = bkm_manager_.create_bkm_map();
                result = it->second->evaluate(context, bkm_map, eval_ctx);
                return true;  // Success - result is valid
            }
            catch (const exception&)
            {
                result = json{};  // Set empty on error
                return false;  // Failure - error occurred
            }
        }
        
            json BusinessRulesEngine::Impl::resolve_variable(string_view name, const json& context) const
        {
            if (context.contains(name))
            {
                return context[name];
            }
            return json{};
        }

            string BusinessRulesEngine::Impl::format_result(string_view decision_name, const json& result) const
        {
            json wrapper = json::object();
            wrapper[decision_name.empty() ? "result" : decision_name] = result;
            return wrapper.dump();
        }

        // Internal component management methods (moved from public API)
        void BusinessRulesEngine::Impl::add_decision_table(unique_ptr<DecisionTable> table)
        {
            if (table)
            {
                decision_tables_[table->name] = std::move(table);
            }
        }

        void BusinessRulesEngine::Impl::add_business_knowledge_model(unique_ptr<BusinessKnowledgeModel> bkm)
        {
            bkm_manager_.add_bkm(std::move(bkm));
        }

        void BusinessRulesEngine::Impl::add_literal_decision(unique_ptr<LiteralDecision> decision)
        {
            if (decision)
            {
                literal_decisions_[decision->name] = std::move(decision);
            }
        }

        // Note: parseBusinessKnowledgeModel, parseDecisionTable, and parseLiteralDecision 
        // are now implemented in dmn_parser.cpp to avoid multiple definitions
        // The DmnParser should have a parse() method that returns a DmnModel

        // ...existing code...
    }
} // namespace orion::api
