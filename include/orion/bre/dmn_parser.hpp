/*
 * ORION Optimized Rule Integration & Operations Nat        void parse_output_clauses(rapidxml::xml_node<char>* table, DecisionTable& decision_table);
        void parse_rules(rapidxml::xml_node<char>* table, DecisionTable& decision_table);
    };
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
#pragma once
#include "dmn_model.hpp"
#include <string>

// Forward declaration for rapidxml types
namespace rapidxml
{
    template <class Ch>
    class xml_node;
}

namespace orion::bre
{
    DecisionTable parse_dmn_decision_table(std::string_view xml);
    // Returns { decisionName, literalExpressionText } or throws if not found
    std::pair<std::string, std::string> parse_dmn_literal_decision(std::string_view xml);
    // Returns { bkmName, parameters, expressionText } or throws if not found  
    std::tuple<std::string, std::vector<std::string>, std::string> parse_dmn_business_knowledge_model(
        std::string_view xml, std::string_view bkm_name = "");

    // DMN Model structure (needs to be defined)
    struct DmnModel
    {
        std::vector<Decision> decisions;
    };

    // DMN Parser class
    class DmnParser
    {
    public:
        DmnModel parse(std::string_view xml);

    private:
        DecisionTable parse_decision_table_from_node(rapidxml::xml_node<char>* table,
                                                     rapidxml::xml_node<char>* decision_node);
        
        // Helper methods for parse_decision_table_from_node (refactored from 120-line function)
        void parse_input_clauses(rapidxml::xml_node<char>* table, DecisionTable& decision_table);
        void parse_output_clauses(rapidxml::xml_node<char>* table, DecisionTable& decision_table);
        void parse_rules(rapidxml::xml_node<char>* table, DecisionTable& decision_table);
    };
}
