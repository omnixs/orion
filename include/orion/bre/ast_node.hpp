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
 * @file ast_node.hpp
 * @brief Abstract Syntax Tree (AST) nodes for FEEL expression parsing and evaluation
 * 
 * ## What is an Abstract Syntax Tree (AST)?
 * 
 * An Abstract Syntax Tree is a tree data structure that represents the syntactic 
 * structure of source code. Each node in the tree represents a construct occurring 
 * in the programming language.
 * 
 * In the context of ORION BRE, the AST is used to represent FEEL expressions 
 * (Friendly Enough Expression Language from the DMN standard).
 * 
 * ## Example: FEEL Expression Parsing
 * 
 * **Expression**: `"Greeting " + Name`
 * 
 * **Gets parsed into this AST structure**:
 * ```
 *          BINARY_OP(+)
 *         /            \
 * LITERAL_STRING    VARIABLE
 * ("Greeting ")     ("Name")
 * ```
 * 
 * **More Complex Example**: `(age + 5) * 2`
 * 
 * **AST Structure**:
 * ```
 *          BINARY_OP(*)
 *         /            \
 *    BINARY_OP(+)   LITERAL_NUMBER(2)
 *    /         \
 * VARIABLE   LITERAL_NUMBER(5)
 * ("age")
 * ```
 * 
 * ## Benefits of Using AST:
 * 
 * 1. **Structured Representation** - Converts text into a tree that's easy to traverse programmatically
 * 2. **Recursive Evaluation** - Can evaluate expressions by walking the tree
 * 3. **Type Safety** - Each node type knows how to handle its specific operation
 * 4. **Extensibility** - Easy to add new expression types by adding new node types
 * 
 * ## References:
 * 
 * - **DMN Specification**: https://www.omg.org/spec/DMN/
 * - **FEEL Language**: https://docs.camunda.org/manual/7.15/reference/dmn/feel/
 * - **Abstract Syntax Trees**: https://en.wikipedia.org/wiki/Abstract_syntax_tree
 * - **DMN FEEL Tutorial**: https://blog.kie.org/2018/01/dmn-tutorial-using-feel-expressions.html
 * - **Compiler Design - AST**: https://craftinginterpreters.com/representing-code.html
 */

#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace orion::bre
{
    using json = nlohmann::json;
    /**
     * @brief FEEL AST Node Types
     * 
     * Defines the different types of nodes that can appear in a FEEL expression AST.
     * Each type corresponds to a specific language construct.
     */
    enum class ASTNodeType : std::uint8_t
    {
        LITERAL_NUMBER, ///< Numeric literal (e.g., 42, 3.14)
        LITERAL_STRING, ///< String literal (e.g., "Hello")
        LITERAL_LIST, ///< List literal (e.g., [1, 2, 3])
        VARIABLE, ///< Variable reference (e.g., Full Name, age)
        BINARY_OP, ///< Binary operation (e.g., +, -, *, /)
        UNARY_OP, ///< Unary operation (e.g., -, not)
        FUNCTION_CALL, ///< Function call (e.g., sum(values))
        PROPERTY_ACCESS, ///< Property access (e.g., person.age)
        CONDITIONAL ///< Conditional expression (e.g., if x > 10 then "high" else "low")
    };

    // Forward declaration for FunctionParameter
    struct ASTNode;

    /**
     * @brief Represents a function parameter in a FEEL function call
     * 
     * Supports both positional and named parameters per DMN 1.5 specification.
     * 
     * **Positional parameter**: name is empty, only valueExpr is set
     * **Named parameter**: both name and valueExpr are set
     * 
     * @example Positional: `abs(-42)`
     * ```cpp
     * FunctionParameter param;
     * param.name = "";  // Empty for positional
     * param.valueExpr = std::make_unique<ASTNode>(ASTNodeType::LITERAL_NUMBER, "-42");
     * ```
     * 
     * @example Named: `abs(n: -42)`
     * ```cpp
     * FunctionParameter param;
     * param.name = "n";
     * param.valueExpr = std::make_unique<ASTNode>(ASTNodeType::LITERAL_NUMBER, "-42");
     * ```
     */
    struct FunctionParameter
    {
        std::string name; ///< Parameter name (empty for positional parameters)
        std::unique_ptr<ASTNode> valueExpr; ///< Expression that evaluates to the parameter value
    };

    /**
     * @brief FEEL AST Node structure
     * 
     * Represents a single node in the Abstract Syntax Tree for FEEL expressions.
     * Each node contains:
     * - **type**: The kind of node (number, string, variable, operation, etc.)
     * - **value**: The actual value or operator (like "Hello" or "+")
     * - **children**: Child nodes for composite expressions (operands for operations)
     * 
     * @example
     * For the expression `"Greeting " + Name`:
     * ```cpp
     * auto root = std::make_unique<ASTNode>(ASTNodeType::BINARY_OP, "+");
     * root->children.push_back(std::make_unique<ASTNode>(ASTNodeType::LITERAL_STRING, "Greeting "));
     * root->children.push_back(std::make_unique<ASTNode>(ASTNodeType::VARIABLE, "Name"));
     * 
     * // Evaluate with context
     * json context = {{"Name", "World"}};
     * json result = root->evaluate(context); // Returns "Greeting World"
     * ```
     */
    struct ASTNode
    {
        ASTNodeType type; ///< The type of this AST node
        std::string value; ///< The value or operator associated with this node
        std::vector<std::unique_ptr<ASTNode>> children; ///< Child nodes (empty for leaf nodes)
        std::vector<FunctionParameter> parameters; ///< Function parameters (only for FUNCTION_CALL nodes)

        /**
         * @brief Construct a new AST Node
         * @param node_type The type of the node
         * @param node_value The value associated with the node (default: empty string)
         */
        ASTNode(ASTNodeType node_type, std::string node_value = "") : type(node_type), value(std::move(node_value))
        {
        }

        /**
         * @brief Virtual destructor for proper cleanup of derived classes
         */
        virtual ~ASTNode() = default;
        
        // Rule of five for polymorphic base class
        ASTNode(const ASTNode&) = delete;
        ASTNode& operator=(const ASTNode&) = delete;
        ASTNode(ASTNode&&) = default;
        ASTNode& operator=(ASTNode&&) = default;

        /**
         * @brief Check if this function call uses named parameters
         * 
         * Only relevant for FUNCTION_CALL nodes. Returns true if any parameter
         * has a non-empty name.
         * 
         * @return true if using named parameters, false otherwise
         */
        [[nodiscard]] bool has_named_parameters() const
        {
            if (type != ASTNodeType::FUNCTION_CALL)
            {
                return false;
            }
            return std::ranges::any_of(parameters, [](const auto& param) {
                return !param.name.empty();
            });
        }
        
        /**
         * @brief Evaluate this AST node in the given context
         * 
         * Recursively evaluates the AST node and its children, returning the result.
         * 
         * @param context JSON object containing variable bindings for evaluation
         * @return Evaluated result as JSON value (number, string, bool, null, etc.)
         * @throws std::runtime_error if evaluation fails (undefined variable, type error, etc.)
         * 
         * @example
         * ```cpp
         * json context = {{"age", 25}, {"priority", 8}};
         * json result = ast->evaluate(context);
         * ```
         */
        [[nodiscard]] json evaluate(const json& context) const;
    };

    /**
     * @brief Stream operator for ASTNodeType
     * 
     * Enables printing ASTNodeType enum values to streams (useful for debugging and testing).
     * 
     * @param output_stream Output stream
     * @param type AST node type to print
     * @return Reference to the output stream
     */
    inline std::ostream& operator<<(std::ostream& output_stream, ASTNodeType type)
    {
        switch (type)
        {
            case ASTNodeType::LITERAL_NUMBER: return output_stream << "LITERAL_NUMBER";
            case ASTNodeType::LITERAL_STRING: return output_stream << "LITERAL_STRING";
            case ASTNodeType::VARIABLE: return output_stream << "VARIABLE";
            case ASTNodeType::BINARY_OP: return output_stream << "BINARY_OP";
            case ASTNodeType::UNARY_OP: return output_stream << "UNARY_OP";
            case ASTNodeType::FUNCTION_CALL: return output_stream << "FUNCTION_CALL";
            case ASTNodeType::PROPERTY_ACCESS: return output_stream << "PROPERTY_ACCESS";
            default: return output_stream << "UNKNOWN(" << static_cast<int>(type) << ")";
        }
    }
} // namespace orion::bre
