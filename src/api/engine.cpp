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
#include <expected>
#include <stdexcept>
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

        // Pimpl implementation class
        class BusinessRulesEngine::Impl
        {
        public:
            std::map<std::string, std::unique_ptr<DecisionTable>> decision_tables_;
            BKMManager bkm_manager_; // Use BKMManager instead of raw map
            std::map<std::string, std::unique_ptr<LiteralDecision>> literal_decisions_;

            // Helper methods
            [[nodiscard]] nlohmann::json resolve_variable(std::string_view name, const nlohmann::json& context) const;
            [[nodiscard]] std::string format_result(std::string_view decision_name, const nlohmann::json& result) const;

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
                
                // Process all decisions in the model
                for (auto& decision : model.decisions)
                {
                    // Handle decision table if present
                    if (decision.decisionTable.has_value())
                    {
                        auto dTable = make_unique<DecisionTable>(std::move(decision.decisionTable.value()));
                        dTable->name = decision.name;
                        pimpl->add_decision_table(std::move(dTable));
                    }
                    
                    // Handle literal expression if present
                    if (!decision.expression.empty())
                    {
                        auto litDec = make_unique<LiteralDecision>();
                        litDec->name = decision.name;
                        litDec->expression_text = decision.expression;
                        
                        // Pre-parse expression as AST for performance (using tryParseExpressionToAST from dmn_parser.cpp)
                        // This function is currently static in dmn_parser.cpp, need to make it accessible
                        
                        pimpl->add_literal_decision(std::move(litDec));
                    }
                }

                // Parse all Business Knowledge Models using BKMManager
                string temp_error;
                pimpl->bkm_manager_.load_bkm_from_dmn(dmn_xml, temp_error, "");
                // BKM parsing is optional, continue if none found

                return {};
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

            // Evaluate all decision tables
            for (const auto& [name, dt] : pimpl->decision_tables_)
            {
                json result = dt->evaluate(data);
                results[name] = result;
            }

            // Evaluate all literal decisions with BKM support
            for (const auto& [name, ld] : pimpl->literal_decisions_)
            {
                try
                {
                    // Use BKMManager to create BKM map for evaluation
                    auto bkm_map = pimpl->bkm_manager_.create_bkm_map();

                    json result = ld->evaluate(data, bkm_map);
                    results[name] = result;
                }
                catch ([[maybe_unused]] const exception& e)
                {
                    // Set error result for this decision
                    results[name] = json{};
                }
            }

            // Always return results as object with decision names as keys
            // This ensures compatibility with TCK test expectations
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
        }

        vector<string> BusinessRulesEngine::validate_models() const
        {
            // Future enhancement: Implement DMN model validation
            // Could validate: decision table structure, hit policies, expression syntax, etc.
            return {}; // No validation errors
        }

        // Impl helper methods implementation
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
