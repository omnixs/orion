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

/**
 * @file feel_parser.hpp
 * @brief Parser for FEEL expressions - builds Abstract Syntax Trees
 * 
 * The parser converts a token stream (from Lexer) into an Abstract Syntax Tree (AST).
 * This is the second phase of AST-based FEEL expression evaluation.
 * 
 * ## Parsing Strategy: Recursive Descent with Operator Precedence
 * 
 * The parser uses recursive descent parsing with explicit operator precedence handling.
 * Each precedence level has its own parsing method:
 * 
 * **Precedence Levels** (highest to lowest):
 * 1. **Primary**: Literals, identifiers, parenthesized expressions
 * 2. **Exponentiation**: `**` (right-associative)
 * 3. **Multiplicative**: `*`, `/`
 * 4. **Additive**: `+`, `-`
 * 5. **Comparison**: `<`, `>`, `<=`, `>=`, `=`, `!=`
 * 6. **Logical AND**: `and`
 * 7. **Logical OR**: `or`
 * 
 * ## Example Parsing
 * 
 * **Expression**: `"age >= 18 and priority > 5"`
 * 
 * **Token Stream**:
 * ```
 * [IDENTIFIER("age"), OPERATOR(">="), NUMBER("18"), KEYWORD("and"), 
 *  IDENTIFIER("priority"), OPERATOR(">"), NUMBER("5")]
 * ```
 * 
 * **Resulting AST**:
 * ```
 *           BINARY_OP(and)
 *          /              \
 *    BINARY_OP(>=)     BINARY_OP(>)
 *    /         \        /          \
 * VAR(age)  LIT(18)  VAR(priority) LIT(5)
 * ```
 * 
 * ## References
 * 
 * - **DMN 1.5 Specification - FEEL Grammar**: Chapter 10 (docs/formal-24-01-01.pdf)
 * - **Recursive Descent Parsing**: https://en.wikipedia.org/wiki/Recursive_descent_parser
 * - **Operator Precedence**: https://en.wikipedia.org/wiki/Order_of_operations
 */

#pragma once
#include "orion/bre/ast_node.hpp"
#include <orion/bre/feel/lexer.hpp>
#include <memory>
#include <stdexcept>

namespace orion::bre::feel {
    /**
     * @brief FEEL Parser - Converts token stream to AST
     * 
     * Implements recursive descent parsing with operator precedence.
     * 
     * @example
     * ```cpp
     * Lexer lexer;
     * auto tokens = lexer.tokenize("age >= 18 and priority > 5");
     * 
     * Parser parser;
     * auto ast = parser.parse(tokens);
     * // ast now contains the root of the AST
     * ```
     */
    class Parser
    {
    public:
    /**
     * @brief Construct a new Parser
     */
    Parser() = default; /**
     * @brief Parse a token stream into an AST
     * 
     * @param tokens Vector of tokens from lexer (must end with END_OF_INPUT)
     * @return Root node of the AST
     * @throws std::runtime_error if parsing fails (syntax error)
     */
    std::unique_ptr<ASTNode> parse(const std::vector<Token>& tokens);

    /**
     * @brief Evaluate a FEEL expression directly with JSON context
     * 
     * This provides a convenient interface for direct evaluation without
     * requiring separate AST construction and evaluation steps.
     * 
     * @param expression The FEEL expression to evaluate
     * @param context The JSON context for variable resolution
     * @return The result of evaluation as JSON
     * @throws std::runtime_error if parsing or evaluation fails
     */
    static nlohmann::json eval_expression(std::string_view expression, const nlohmann::json& context);  private:
        const std::vector<Token>* tokens_ = nullptr;  ///< Current token stream
        size_t position_ = 0;                         ///< Current position in token stream
        
        /**
         * @brief Get current token without advancing
         * @return Current token reference
         */
        [[nodiscard]] const Token& peek() const;
        
        /**
         * @brief Get current token and advance position
         * @return Current token reference (before advancing)
         */
        const Token& advance();
        
        /**
         * @brief Check if current token matches expected type
         * @param type Expected token type
         * @return true if match
         */
        [[nodiscard]] bool check(TokenType type) const;
        
        /**
         * @brief Check if current token text matches expected value
         * @param text Expected token text
         * @return true if match
         */
        [[nodiscard]] bool check_text(std::string_view text) const;
        
        /**
         * @brief Consume expected token or throw error
         * @param type Expected token type
         * @param message Error message if mismatch
         * @return Consumed token
         */
        const Token& expect(TokenType type, std::string_view message);
        
        /**
         * @brief Check if at end of input
         * @return true if current token is END_OF_INPUT
         */
        [[nodiscard]] bool is_at_end() const;
        
        // Precedence-based parsing methods (lowest to highest precedence)
        
        /**
         * @brief Parse conditional expression (lowest precedence)
         * Grammar: conditional → "if" logicalOr "then" conditional "else" conditional | logicalOr
         */
        std::unique_ptr<ASTNode> parse_conditional();
        
        /**
         * @brief Parse logical OR expression
         * Grammar: logicalOr → logicalAnd ( "or" logicalAnd )*
         */
        std::unique_ptr<ASTNode> parse_logical_or();
        
        /**
         * @brief Parse logical AND expression
         * Grammar: logicalAnd → comparison ( "and" comparison )*
         */
        std::unique_ptr<ASTNode> parse_logical_and();
        
        /**
         * @brief Parse comparison expression
         * Grammar: comparison → additive ( ("<" | ">" | "<=" | ">=" | "=" | "!=") additive )*
         */
        std::unique_ptr<ASTNode> parse_comparison();
        
        /**
         * @brief Parse additive expression
         * Grammar: additive → multiplicative ( ("+" | "-") multiplicative )*
         */
        std::unique_ptr<ASTNode> parse_additive();
        
        /**
         * @brief Parse multiplicative expression
         * Grammar: multiplicative → exponentiation ( ("*" | "/") exponentiation )*
         */
        std::unique_ptr<ASTNode> parse_multiplicative();
        
        /**
         * @brief Parse exponentiation expression (right-associative)
         * Grammar: exponentiation → primary ( "**" exponentiation )?
         */
        std::unique_ptr<ASTNode> parse_exponentiation();
        
    /**
     * @brief Parse primary expression (highest precedence)
     * Grammar: primary → NUMBER | STRING | IDENTIFIER | "true" | "false" | "null" | "(" expression ")"
     */
    std::unique_ptr<ASTNode> parse_primary();
    
    // Helper methods for parse_primary to reduce cognitive complexity
    std::unique_ptr<ASTNode> parse_number_literal();
    std::unique_ptr<ASTNode> parse_string_literal();
    std::unique_ptr<ASTNode> parse_keyword_or_not_function();
    std::unique_ptr<ASTNode> parse_identifier_or_function();
    std::unique_ptr<ASTNode> parse_function_call(std::string_view function_name);
    void parse_function_parameters(ASTNode* func_node, std::string_view function_name);
    std::unique_ptr<ASTNode> parse_variable_with_properties(std::string_view var_name);
    std::unique_ptr<ASTNode> parse_parenthesized_expression();
    std::unique_ptr<ASTNode> parse_list_literal();
    std::unique_ptr<ASTNode> parse_unary_minus();
};
} // namespace orion::bre