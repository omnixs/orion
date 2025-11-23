/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy at https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modifications: This file has been modified by ORION contributors. See VCS history.
 */

/**
 * @file bkm_manager.hpp
 * @brief Business Knowledge Model (BKM) management and evaluation
 * 
 * This module handles Business Knowledge Models according to the DMN specification.
 * BKMs are reusable functions that encapsulate business logic and can be invoked
 * by decisions or other BKMs.
 * 
 * ## DMN Specification Compliance
 * 
 * According to DMN 1.5 specification section 5.3.2:
 * - BKMs are functions that encapsulate business knowledge
 * - They accept parameters and return computed results
 * - They support composition (can call other BKMs)
 * - They use FEEL expressions for implementation
 * 
 * ## References
 * - DMN 1.5 Specification: https://www.omg.org/spec/DMN/
 * - Section 5.3.2: Business Knowledge Models
 * - Section 6.3.9: Business Knowledge Model metamodel
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include "business_knowledge_model.hpp"

namespace orion::bre
{
    /**
     * @brief Manager for Business Knowledge Models
     * 
     * Handles parsing, storage, and invocation of Business Knowledge Models
     * from DMN definitions. Supports function composition and parameter binding.
     */
    class BKMManager
    {
    public:
        /**
         * @brief Parse and add BKM from DMN XML
         * @param dmn_xml The DMN XML content
         * @param error_message Output parameter for error details
         * @param bkm_name Optional specific BKM name to parse (empty = parse all)
         * @return true if parsing succeeded
         */
        bool load_bkm_from_dmn(std::string_view dmn_xml, std::string& error_message, std::string_view bkm_name = "");

        /**
         * @brief Add a BKM instance
         * @param bkm The BKM to add (ownership transferred)
         */
        void add_bkm(std::unique_ptr<BusinessKnowledgeModel> bkm);

        /**
         * @brief Invoke a BKM by name
         * @param bkm_name Name of the BKM to invoke
         * @param args Arguments to pass to the BKM
         * @param context Evaluation context (variable bindings)
         * @return Result of BKM evaluation
         */
        [[nodiscard]] nlohmann::json invoke_bkm(std::string_view bkm_name,
                                 const std::vector<nlohmann::json>& args,
                                 const nlohmann::json& context) const;

        /**
         * @brief Check if a BKM exists
         * @param bkm_name Name to check
         * @return true if BKM exists
         */
        [[nodiscard]] bool has_bkm(std::string_view bkm_name) const;

        /**
         * @brief Get BKM by name (read-only access)
         * @param bkm_name Name of the BKM
         * @return Pointer to BKM or nullptr if not found
         */
        [[nodiscard]] const BusinessKnowledgeModel* get_bkm(std::string_view bkm_name) const;

        /**
         * @brief Get all BKM names
         * @return Vector of BKM names
         */
        [[nodiscard]] std::vector<std::string> get_bkm_names() const;

        /**
         * @brief Remove a BKM
         * @param bkm_name Name of BKM to remove
         * @return true if BKM was removed
         */
        [[nodiscard]] bool remove_bkm(std::string_view bkm_name);

        /**
         * @brief Clear all BKMs
         */
        void clear();

        /**
         * @brief Create a copyable map of BKMs for evaluation contexts
         * @return Map of BKM name to BKM copy (without unique_ptr)
         */
        [[nodiscard]] std::map<std::string, BusinessKnowledgeModel> create_bkm_map() const;

    private:
        std::map<std::string, std::unique_ptr<BusinessKnowledgeModel>> bkms_;
    };

    /**
     * @brief Factory function to parse BKM from DMN XML
     * @param dmn_xml The DMN XML content
     * @param bkm_name Specific BKM name to parse (empty = first found)
     * @param error_message Output parameter for error details
     * @return Unique pointer to parsed BKM or nullptr on failure
     */
    std::unique_ptr<BusinessKnowledgeModel> parse_business_knowledge_model(
        std::string_view dmn_xml,
        std::string_view bkm_name,
        std::string& error_message);

    /**
     * @brief Parse specific BKM invocation pattern from FEEL expression
     * @param expression FEEL expression that may contain BKM calls
     * @param context Evaluation context
     * @param available_bkms Map of available BKMs
     * @return Result of evaluation or error
     */
    nlohmann::json evaluate_bkm_expression(std::string_view expression,
                                         const nlohmann::json& context,
                                         const std::map<std::string, BusinessKnowledgeModel>& available_bkms);
} // namespace orion::bre
