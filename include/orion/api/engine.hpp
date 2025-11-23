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

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <expected>

namespace orion::api
{
    // Main stateful BRE engine
    class BusinessRulesEngine
    {
    private:
        class Impl;
        std::unique_ptr<Impl> pimpl;

    public:
        BusinessRulesEngine();
        ~BusinessRulesEngine();

        // Non-copyable but moveable
        BusinessRulesEngine(const BusinessRulesEngine&) = delete;
        BusinessRulesEngine& operator=(const BusinessRulesEngine&) = delete;
        BusinessRulesEngine(BusinessRulesEngine&&) noexcept;
        BusinessRulesEngine& operator=(BusinessRulesEngine&&) noexcept;

        // Parse and load DMN model
        // Returns void on success, error message on failure
        [[nodiscard]] std::expected<void, std::string> load_dmn_model(std::string_view dmn_xml);

        // Remove components
        [[nodiscard]] bool remove_decision_table(std::string_view name);
        [[nodiscard]] bool remove_business_knowledge_model(std::string_view name);
        [[nodiscard]] bool remove_literal_decision(std::string_view name);

        // Evaluate with loaded models
        [[nodiscard]] std::string evaluate(std::string_view data_json) const;

        // Introspection
        [[nodiscard]] std::vector<std::string> get_decision_table_names() const;
        [[nodiscard]] std::vector<std::string> get_business_knowledge_model_names() const;
        [[nodiscard]] std::vector<std::string> get_literal_decision_names() const;

        // Clear all loaded models
        void clear();

        // Validation
        [[nodiscard]] std::vector<std::string> validate_models() const;
    };

} // namespace orion::api
