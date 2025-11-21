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
 * @file feel_lexer.hpp
 * @brief Lexical analyzer (tokenizer) for FEEL expressions
 * 
 * The lexer converts a raw FEEL expression string into a sequence of tokens.
 * This is the first phase of AST-based FEEL expression evaluation.
 * 
 * ## Tokenization Example
 * 
 * **Input Expression**: `"age >= 18 and priority > 5"`
 * 
 * **Token Stream Output**:
 * ```
 * Token(IDENTIFIER, "age")
 * Token(OPERATOR, ">=")
 * Token(NUMBER, "18")
 * Token(KEYWORD, "and")
 * Token(IDENTIFIER, "priority")
 * Token(OPERATOR, ">")
 * Token(NUMBER, "5")
 * Token(END_OF_INPUT, "")
 * ```
 * 
 * ## Supported Token Types
 * 
 * - **Numbers**: Integers, decimals, scientific notation (42, 3.14, 1e-5)
 * - **Strings**: Double-quoted literals ("Hello", "Greeting ")
 * - **Identifiers**: Variable names (age, Full Name, Monthly Salary)
 * - **Keywords**: FEEL reserved words (true, false, null, and, or, not, if, then, else)
 * - **Operators**: Arithmetic (+, -, *, /, **), comparison (<, >, <=, >=, =, !=), logical (and, or, not)
 * - **Punctuation**: Parentheses, commas, brackets
 * 
 * ## References
 * 
 * - **DMN 1.5 Specification - FEEL Grammar**: Chapter 10 (docs/formal-24-01-01.pdf)
 * - **Lexical Analysis**: https://en.wikipedia.org/wiki/Lexical_analysis
 */

#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <ostream>

namespace orion::bre::feel {
    /**
     * @brief Token types recognized by the FEEL lexer
     */
    enum class TokenType : std::uint8_t
    {
        // Literals
        NUMBER,           ///< Numeric literal (42, 3.14, 1e-5)
        STRING,           ///< String literal ("Hello")
        
        // Identifiers and keywords
        IDENTIFIER,       ///< Variable name (age, Full Name)
        KEYWORD,          ///< Reserved word (true, false, null, and, or, not, if, then, else)
        
        // Operators
        OPERATOR,         ///< Arithmetic, comparison, logical operators
        
        // Punctuation
        LPAREN,           ///< Left parenthesis (
        RPAREN,           ///< Right parenthesis )
        LBRACKET,         ///< Left bracket [
        RBRACKET,         ///< Right bracket ]
        COMMA,            ///< Comma ,
        DOT,              ///< Dot . (for property access)
        COLON,            ///< Colon : (for named parameters)
        
        // Special
        END_OF_INPUT,     ///< End of token stream
        UNKNOWN           ///< Unrecognized token (lexer error)
    };

    /**
     * @brief A single token in a FEEL expression
     * 
     * Contains the token type and the original text from the source expression.
     */
    struct Token
    {
        TokenType type;      ///< The type of this token
        std::string text;    ///< The original text of the token
        size_t position;     ///< Position in source string (for error reporting)

        /**
         * @brief Construct a new Token
         * @param token_type Token type
         * @param token_text Token text
         * @param pos Position in source string
         */
        Token(TokenType token_type, std::string token_text, size_t pos = 0)
            : type(token_type), text(std::move(token_text)), position(pos)
        {
        }
    };

    /**
     * @brief FEEL Lexer (Tokenizer)
     * 
     * Converts a FEEL expression string into a sequence of tokens for parsing.
     * 
     * @example
     * ```cpp
     * Lexer lexer;
     * auto tokens = lexer.tokenize("age >= 18 and priority > 5");
     * for (const auto& token : tokens) {
     *     // Process each token
     * }
     * ```
     */
    class Lexer
    {
    public:
        /**
         * @brief Tokenize a FEEL expression
         * 
         * Converts the input expression into a sequence of tokens.
         * Always ends with an END_OF_INPUT token.
         * 
         * @param expression The FEEL expression to tokenize
         * @return Vector of tokens (always ends with END_OF_INPUT)
         * @throws std::runtime_error if lexical errors are encountered
         */
        std::vector<Token> tokenize(const std::string& expression);

    private:
        std::string input_;     ///< Current input expression
        size_t position_;       ///< Current position in input
        
        /**
         * @brief Check if a string is a FEEL keyword
         * @param text The text to check
         * @return true if the text is a reserved keyword
         */
        [[nodiscard]] bool is_keyword(const std::string& text) const;
        
        /**
         * @brief Get the next character without advancing position
         * @return Current character or '\0' if at end
         */
        [[nodiscard]] char peek() const;
        
        /**
         * @brief Get the next character and advance position
         * @return Current character or '\0' if at end
         */
        char advance();
        
        /**
         * @brief Skip whitespace characters
         */
        void skip_whitespace();
        
        /**
         * @brief Tokenize a number (integer, decimal, or scientific notation)
         * @return Number token
         */
        Token tokenize_number();
        
        /**
         * @brief Tokenize a string literal (double-quoted)
         * @return String token
         */
        Token tokenize_string();
        
        /**
         * @brief Tokenize an identifier or keyword
         * @return Identifier or keyword token
         */
        Token tokenize_identifier();
        
        /**
         * @brief Tokenize an operator (arithmetic, comparison, logical)
         * @return Operator token
         */
        Token tokenize_operator();
        
        // Helper functions for tokenize_identifier()
        
        /**
         * @brief Check if character is an operator or punctuation
         * @param chr Character to check
         * @return true if character is an operator or punctuation
         */
        [[nodiscard]] bool is_operator_or_punctuation(char chr) const;
        
        /**
         * @brief Skip whitespace starting from given position
         * @param pos Starting position
         * @return Position of first non-whitespace character
         */
        [[nodiscard]] size_t skip_whitespace_from(size_t pos) const;
        
        /**
         * @brief Extract next word starting from position
         * @param pos Starting position
         * @return Extracted word (letters, digits, underscores)
         */
        [[nodiscard]] std::string extract_next_word(size_t pos) const;
        
        /**
         * @brief Check if we should stop at current space during identifier tokenization
         * @param current_text The identifier text accumulated so far
         * @return true if we should stop at the space
         */
        [[nodiscard]] bool should_stop_at_space(const std::string& current_text) const;
        
        /**
         * @brief Trim trailing whitespace from string
         * @param text String to trim
         * @return Trimmed string
         */
        [[nodiscard]] std::string trim_trailing_spaces(std::string text) const;
        
        // Helper functions for tokenize()
        
        /**
         * @brief Check if we're in a context where '-' is unary (part of number)
         * @param tokens Current token vector
         * @return true if '-' should be treated as part of number
         */
        [[nodiscard]] bool is_unary_minus_context(const std::vector<Token>& tokens) const;
        
        /**
         * @brief Check if current position starts a number token
         * @param current Current character
         * @param tokens Current token vector
         * @return true if position starts a number
         */
        [[nodiscard]] bool starts_number_token(char current, const std::vector<Token>& tokens) const;
        
        /**
         * @brief Process single-character punctuation and add to tokens
         * @param current Current character
         * @param tokens Token vector to append to
         */
        void process_punctuation(char current, std::vector<Token>& tokens);
    };
    
    /**
     * @brief Stream output operator for TokenType (for debugging/testing)
     */
    inline std::ostream& operator<<(std::ostream& output_stream, TokenType type)
    {
        switch (type)
        {
            case TokenType::NUMBER: return output_stream << "NUMBER";
            case TokenType::STRING: return output_stream << "STRING";
            case TokenType::IDENTIFIER: return output_stream << "IDENTIFIER";
            case TokenType::KEYWORD: return output_stream << "KEYWORD";
            case TokenType::OPERATOR: return output_stream << "OPERATOR";
            case TokenType::LPAREN: return output_stream << "LPAREN";
            case TokenType::RPAREN: return output_stream << "RPAREN";
            case TokenType::LBRACKET: return output_stream << "LBRACKET";
            case TokenType::RBRACKET: return output_stream << "RBRACKET";
            case TokenType::COMMA: return output_stream << "COMMA";
            case TokenType::DOT: return output_stream << "DOT";
            case TokenType::COLON: return output_stream << "COLON";
            case TokenType::END_OF_INPUT: return output_stream << "END_OF_INPUT";
            case TokenType::UNKNOWN: return output_stream << "UNKNOWN";
            default: return output_stream << "INVALID_TOKEN_TYPE";
        }
    }
} // namespace orion::bre
