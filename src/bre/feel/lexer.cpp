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

#include <orion/bre/feel/lexer.hpp>
#include <cctype>
#include <stdexcept>

namespace orion::bre::feel {
    // Helper: Check if we're in a context where '-' should be unary (part of number)
    bool Lexer::is_unary_minus_context(const std::vector<Token>& tokens) const
    {
        if (tokens.empty()) {
            return true; // Start of expression
        }
        TokenType last_type = tokens.back().type;
        return (last_type == TokenType::OPERATOR ||
                last_type == TokenType::LPAREN ||
                last_type == TokenType::LBRACKET ||
                last_type == TokenType::COMMA);
    }

    // Helper: Check if current position starts a number token
    bool Lexer::starts_number_token(char current, const std::vector<Token>& tokens) const
    {
        // Regular digit
        if (std::isdigit(current)) {
            return true;
        }
        
        // Negative number: '-' followed by digit (e.g., -42)
        if (current == '-' && is_unary_minus_context(tokens) && 
            position_ + 1 < input_.length() && std::isdigit(input_[position_ + 1])) {
            return true;
        }
        
        // Negative decimal: '-' followed by '.digit' (e.g., -.872)
        if (current == '-' && is_unary_minus_context(tokens) && 
            position_ + 2 < input_.length() && input_[position_ + 1] == '.' && 
            std::isdigit(input_[position_ + 2])) {
            return true;
        }
        
        // Leading-dot decimal: '.' followed by digit (e.g., .872)
        if (current == '.' && position_ + 1 < input_.length() && 
            std::isdigit(input_[position_ + 1])) {
            return true;
        }
        
        return false;
    }

    // Helper: Process single-character punctuation token
    void Lexer::process_punctuation(char current, std::vector<Token>& tokens)
    {
        TokenType type;
        switch (current) {
            case '(': type = TokenType::LPAREN; break;
            case ')': type = TokenType::RPAREN; break;
            case '[': type = TokenType::LBRACKET; break;
            case ']': type = TokenType::RBRACKET; break;
            case ',': type = TokenType::COMMA; break;
            case ':': type = TokenType::COLON; break;
            case '.': type = TokenType::DOT; break;
            default: return; // Not punctuation
        }
        tokens.emplace_back(type, std::string(1, current), position_);
        advance();
    }

    std::vector<Token> Lexer::tokenize(std::string_view expression)
    {
        input_ = expression;
        position_ = 0;
        std::vector<Token> tokens;

        while (position_ < input_.length())
        {
            skip_whitespace();
            
            if (position_ >= input_.length()) {
                break;
            }

            char current = peek();

            // Numbers (including negative and leading-dot decimals)
            if (starts_number_token(current, tokens))
            {
                tokens.push_back(tokenize_number());
            }
            // Strings
            else if (current == '"')
            {
                tokens.push_back(tokenize_string());
            }
            // Identifiers and keywords
            else if (std::isalpha(current) || current == '_')
            {
                tokens.push_back(tokenize_identifier());
            }
            // Single-character punctuation
            else if (current == '(' || current == ')' || current == '[' || current == ']' ||
                     current == ',' || current == ':' || current == '.')
            {
                process_punctuation(current, tokens);
            }
            // Operators
            else if (current == '+' || current == '-' || current == '*' || current == '/' ||
                     current == '<' || current == '>' || current == '=' || current == '!')
            {
                tokens.push_back(tokenize_operator());
            }
            else
            {
                // Unknown character
                throw std::runtime_error(std::string("Unexpected character at position ") + 
                                         std::to_string(position_) + ": '" + current + "'");
            }
        }

        // Always end with END_OF_INPUT
        tokens.emplace_back(TokenType::END_OF_INPUT, "", position_);
        return tokens;
    }

    bool Lexer::is_keyword(std::string_view text) const
    {
        return text == "true" || text == "false" || text == "null" ||
               text == "and" || text == "or" || text == "not" ||
               text == "if" || text == "then" || text == "else" ||
               text == "in" || text == "for" || text == "some" || text == "every" ||
               text == "return" || text == "between" || text == "instance" || text == "of";
    }

    char Lexer::peek() const
    {
        if (position_ >= input_.length()) {
            return '\0';
        }
        return input_[position_];
    }

    char Lexer::advance()
    {
        if (position_ >= input_.length()) {
            return '\0';
        }
        return input_[position_++];
    }

    void Lexer::skip_whitespace()
    {
        while (position_ < input_.length() && std::isspace(input_[position_]))
        {
            position_++;
        }
    }

    Token Lexer::tokenize_number()
    {
        size_t start = position_;
        std::string text;

        // Handle negative sign
        if (peek() == '-')
        {
            text += advance();
        }

        // Integer part (optional for leading-dot decimals like .872)
        while (std::isdigit(peek()))
        {
            text += advance();
        }

        // Decimal part
        if (peek() == '.')
        {
            text += advance();
            while (std::isdigit(peek()))
            {
                text += advance();
            }
        }

        // Scientific notation (e.g., 1e-5, 2.5E+10)
        if (peek() == 'e' || peek() == 'E')
        {
            text += advance();
            if (peek() == '+' || peek() == '-')
            {
                text += advance();
            }
            while (std::isdigit(peek()))
            {
                text += advance();
            }
        }

        return Token(TokenType::NUMBER, text, start);
    }

    Token Lexer::tokenize_string()
    {
        size_t start = position_;
        std::string text;

        // Opening quote
        if (peek() != '"')
        {
            throw std::runtime_error("Expected opening quote for string literal");
        }
        text += advance();

        // String content (with escape sequence support)
        while (peek() != '"' && peek() != '\0')
        {
            if (peek() == '\\')
            {
                text += advance(); // Add backslash
                if (peek() != '\0')
                {
                    text += advance(); // Add escaped character
                }
            }
            else
            {
                text += advance();
            }
        }

        // Closing quote
        if (peek() != '"')
        {
            throw std::runtime_error("Unterminated string literal");
        }
        text += advance();

        return Token(TokenType::STRING, text, start);
    }

    // Helper: Check if next character is an operator or punctuation
    bool Lexer::is_operator_or_punctuation(char chr) const
    {
        return chr == '+' || chr == '-' || chr == '*' || chr == '/' ||
               chr == '<' || chr == '>' || chr == '=' || chr == '!' ||
               chr == '(' || chr == ')' || chr == '[' || chr == ']' ||
               chr == ',' || chr == '.';
    }

    // Helper: Skip whitespace from given position and return first non-space position
    size_t Lexer::skip_whitespace_from(size_t pos) const
    {
        while (pos < input_.length() && std::isspace(input_[pos]))
        {
            pos++;
        }
        return pos;
    }

    // Helper: Extract next word starting from position
    std::string Lexer::extract_next_word(size_t pos) const
    {
        std::string word;
        while (pos < input_.length() && 
               (std::isalnum(input_[pos]) || input_[pos] == '_'))
        {
            word += input_[pos];
            pos++;
        }
        return word;
    }

    // Helper: Check if we should stop at current space
    bool Lexer::should_stop_at_space(std::string_view current_text) const
    {
        // Stop if current text is already a keyword
        if (is_keyword(current_text))
        {
            return true;
        }

        // Look ahead past whitespace
        size_t lookahead = skip_whitespace_from(position_ + 1);
        
        if (lookahead >= input_.length())
        {
            return true; // End of input
        }

        // Stop if next character is an operator or punctuation
        if (is_operator_or_punctuation(input_[lookahead]))
        {
            return true;
        }

        // Stop if next word is a keyword
        std::string next_word = extract_next_word(lookahead);
        if (!next_word.empty() && is_keyword(next_word))
        {
            return true;
        }

        return false; // Continue including spaces
    }

    // Helper: Trim trailing whitespace from string
    std::string Lexer::trim_trailing_spaces(std::string text) const
    {
        while (!text.empty() && std::isspace(text.back()))
        {
            text.pop_back();
        }
        return text;
    }

    // Main tokenizer: Handle identifiers with potential spaces
    Token Lexer::tokenize_identifier()
    {
        size_t start = position_;
        std::string text;

        // First character (letter or underscore)
        text += advance();

        // Subsequent characters (letters, digits, underscores, spaces)
        // FEEL allows spaces in identifiers (e.g., "Full Name", "Monthly Salary")
        while (std::isalnum(peek()) || peek() == '_' || peek() == ' ')
        {
            // Check if we should stop at this space
            if (peek() == ' ' && should_stop_at_space(text))
            {
                break;
            }
            text += advance();
        }

        // Trim trailing spaces from identifier
        text = trim_trailing_spaces(text);

        // Check if it's a keyword
        TokenType type = is_keyword(text) ? TokenType::KEYWORD : TokenType::IDENTIFIER;
        return Token(type, text, start);
    }

    Token Lexer::tokenize_operator()
    {
        size_t start = position_;
        std::string text;

        char current = advance();
        text += current;

        // Two-character operators
        if (current == '*' && peek() == '*')
        {
            // Exponentiation **
            text += advance();
        }
        else if (current == '<' && peek() == '=')
        {
            // Less than or equal <=
            text += advance();
        }
        else if (current == '>' && peek() == '=')
        {
            // Greater than or equal >=
            text += advance();
        }
        else if (current == '!' && peek() == '=')
        {
            // Not equal !=
            text += advance();
        }
        else if (current == '=' && peek() == '=')
        {
            // Alternative equality ==  (treat as =)
            text += advance();
        }

        return Token(TokenType::OPERATOR, text, start);
    }
} // namespace orion::bre
