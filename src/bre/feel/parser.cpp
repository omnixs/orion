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

#include <orion/bre/feel/parser.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace orion::bre::feel {
    namespace {
        /**
         * @brief Append a Unicode code point to a string as UTF-8.
         *
         * Used when decoding \\uXXXX / \\UXXXXXX escapes in string literals.
         * Invalid code points are replaced with U+FFFD.
         */
        void append_utf8(std::string& out, unsigned long code_point)
        {
            if (code_point > 0x10FFFFUL || (code_point >= 0xD800UL && code_point <= 0xDFFFUL))
            {
                code_point = 0xFFFDUL; // Replacement character
            }

            if (code_point < 0x80UL)
            {
                out += static_cast<char>(code_point);
            }
            else if (code_point < 0x800UL)
            {
                out += static_cast<char>(0xC0UL | (code_point >> 6));
                out += static_cast<char>(0x80UL | (code_point & 0x3FUL));
            }
            else if (code_point < 0x10000UL)
            {
                out += static_cast<char>(0xE0UL | (code_point >> 12));
                out += static_cast<char>(0x80UL | ((code_point >> 6) & 0x3FUL));
                out += static_cast<char>(0x80UL | (code_point & 0x3FUL));
            }
            else
            {
                out += static_cast<char>(0xF0UL | (code_point >> 18));
                out += static_cast<char>(0x80UL | ((code_point >> 12) & 0x3FUL));
                out += static_cast<char>(0x80UL | ((code_point >> 6) & 0x3FUL));
                out += static_cast<char>(0x80UL | (code_point & 0x3FUL));
            }
        }

        /**
         * @brief Decode FEEL string escape sequences (DMN 1.5 section 10.3.1.2).
         *
         * The lexer deliberately preserves raw text including backslashes, so
         * decoding happens here at parse time and costs nothing at evaluation
         * time. Unknown escapes are preserved verbatim.
         */
        std::string decode_string_escapes(std::string_view text)
        {
            if (text.find('\\') == std::string_view::npos)
            {
                return std::string(text); // Fast path: nothing to decode
            }

            std::string decoded;
            decoded.reserve(text.size());
            for (size_t i = 0; i < text.size(); ++i)
            {
                if (text[i] != '\\' || i + 1 >= text.size())
                {
                    decoded += text[i];
                    continue;
                }

                const char escape = text[++i];
                switch (escape)
                {
                    case 'n':  decoded += '\n'; break;
                    case 'r':  decoded += '\r'; break;
                    case 't':  decoded += '\t'; break;
                    case '\'': decoded += '\''; break;
                    case '"':  decoded += '"';  break;
                    case '\\': decoded += '\\'; break;
                    case 'u':
                    case 'U':
                    {
                        // \uXXXX (4 hex digits) or \UXXXXXX (6 hex digits) -> UTF-8
                        const size_t digits = (escape == 'u') ? 4 : 6;
                        if (i + digits < text.size())
                        {
                            const std::string_view hex = text.substr(i + 1, digits);
                            const bool all_hex = std::all_of(hex.begin(), hex.end(), [](unsigned char ch) {
                                return std::isxdigit(ch) != 0;
                            });
                            if (all_hex)
                            {
                                const unsigned long code_point = std::stoul(std::string(hex), nullptr, 16);
                                append_utf8(decoded, code_point);
                                i += digits;
                                break;
                            }
                        }
                        // Not a valid escape: keep it verbatim
                        decoded += '\\';
                        decoded += escape;
                        break;
                    }
                    default:
                        // Unknown escape: preserve both characters
                        decoded += '\\';
                        decoded += escape;
                        break;
                }
            }
            return decoded;
        }
    } // namespace

    std::unique_ptr<ASTNode> Parser::parse(const std::vector<Token>& tokens)
    {
        tokens_ = &tokens;
        position_ = 0;
        depth_ = 0;
        
        if (tokens.empty() || is_at_end())
        {
            throw std::runtime_error("Cannot parse empty token stream");
        }
        
        // Start parsing from lowest precedence level (conditional expressions)
        auto ast = parse_conditional();
        
        // Verify we consumed all tokens (except END_OF_INPUT)
        if (!is_at_end())
        {
            std::ostringstream oss;
            oss << "Unexpected token after expression: '" << peek().text 
                << "' at position " << peek().position;
            throw std::runtime_error(oss.str());
        }
        
        return ast;
    }
    
    const Token& Parser::peek() const
    {
        if ((tokens_ == nullptr) || tokens_->empty())
        {
            // Create a static END_OF_INPUT token to return when there are no tokens
            static const Token end_token(TokenType::END_OF_INPUT, "", 0);
            return end_token; // Return static END_OF_INPUT when empty or null
        }
        if (position_ >= tokens_->size())
        {
            return (*tokens_)[tokens_->size() - 1]; // Return END_OF_INPUT from tokens
        }
        return (*tokens_)[position_];
    }
    
    const Token& Parser::advance()
    {
        const Token& current = peek();
        if (!is_at_end())
        {
            position_++;
        }
        return current;
    }
    
    bool Parser::check(TokenType type) const
    {
        return peek().type == type;
    }
    
    bool Parser::check_text(std::string_view text) const
    {
        return peek().text == text;
    }
    
    const Token& Parser::expect(TokenType type, std::string_view message)
    {
        if (!check(type))
        {
            std::ostringstream oss;
            oss << message << " (got '" << peek().text << "' at position " << peek().position << ")";
            throw std::runtime_error(oss.str());
        }
        return advance();
    }
    
    bool Parser::is_at_end() const
    {
        return check(TokenType::END_OF_INPUT);
    }
    
    // Precedence level 0 (lowest): Conditional expressions (if-then-else)
    std::unique_ptr<ASTNode> Parser::parse_conditional()
    {
        // Check for "if" keyword
        if (check(TokenType::KEYWORD) && check_text("if"))
        {
            advance(); // consume "if"
            
            auto node = std::make_unique<ASTNode>(ASTNodeType::CONDITIONAL);
            
            // Parse condition expression
            auto condition = parse_conditional();
            node->children.push_back(std::move(condition));
            
            // Expect "then" keyword
            if (!check(TokenType::KEYWORD) || !check_text("then"))
            {
                std::ostringstream oss;
                oss << "Expected 'then' after if condition at position " << peek().position;
                throw std::runtime_error(oss.str());
            }
            advance(); // consume "then"
            
            // Parse then expression (allow nested conditionals)
            auto then_expr = parse_conditional();
            node->children.push_back(std::move(then_expr));
            
            // Expect "else" keyword
            if (!check(TokenType::KEYWORD) || !check_text("else"))
            {
                std::ostringstream oss;
                oss << "Expected 'else' after then expression at position " << peek().position;
                throw std::runtime_error(oss.str());
            }
            advance(); // consume "else"
            
            // Parse else expression (allow nested conditionals)
            auto else_expr = parse_conditional();
            node->children.push_back(std::move(else_expr));
            
            return node;
        }
        
        // No "if" keyword, fall through to logical OR
        return parse_logical_or();
    }
    
    // Precedence level 1 (lowest): Logical OR
    std::unique_ptr<ASTNode> Parser::parse_logical_or()
    {
        auto left = parse_logical_and();
        
        while (check(TokenType::KEYWORD) && check_text("or"))
        {
            advance(); // consume "or"
            auto right = parse_logical_and();
            
            // Create binary OR node
            auto node = std::make_unique<ASTNode>(ASTNodeType::BINARY_OP, "or");
            node->children.push_back(std::move(left));
            node->children.push_back(std::move(right));
            left = std::move(node);
        }
        
        return left;
    }
    
    // Precedence level 2: Logical AND
    std::unique_ptr<ASTNode> Parser::parse_logical_and()
    {
        auto left = parse_comparison();
        
        while (check(TokenType::KEYWORD) && check_text("and"))
        {
            advance(); // consume "and"
            auto right = parse_comparison();
            
            // Create binary AND node
            auto node = std::make_unique<ASTNode>(ASTNodeType::BINARY_OP, "and");
            node->children.push_back(std::move(left));
            node->children.push_back(std::move(right));
            left = std::move(node);
        }
        
        return left;
    }
    
    // Precedence level 3: Comparison operators
    std::unique_ptr<ASTNode> Parser::parse_comparison()
    {
        auto left = parse_additive();
        
        // Check for 'between', 'in', 'instance of' keywords
        if (check(TokenType::KEYWORD))
        {
            if (check_text("between"))
            {
                advance(); // consume 'between'
                auto lower = parse_additive();
                if (!check(TokenType::KEYWORD) || !check_text("and"))
                {
                    throw std::runtime_error("Expected 'and' in 'between' expression");
                }
                advance(); // consume 'and'
                auto upper = parse_additive();
                
                auto node = std::make_unique<ASTNode>(ASTNodeType::BETWEEN, "between");
                node->children.push_back(std::move(left));
                node->children.push_back(std::move(lower));
                node->children.push_back(std::move(upper));
                return node;
            }
            else if (check_text("instance"))
            {
                advance(); // consume 'instance'
                if (!check(TokenType::KEYWORD) || !check_text("of"))
                {
                    throw std::runtime_error("Expected 'of' after 'instance'");
                }
                advance(); // consume 'of'
                
                // Parse type name - can be multi-word like "date and time" or "years and months duration"
                std::string type_name;
                if (check(TokenType::KEYWORD) || check(TokenType::IDENTIFIER))
                {
                    type_name = std::string(advance().text);
                    // Handle multi-word type names
                    while (check(TokenType::KEYWORD) && check_text("and"))
                    {
                        type_name += " and";
                        advance(); // consume 'and'
                        if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD))
                        {
                            type_name += " " + std::string(advance().text);
                        }
                    }
                    // Handle parameterized types: list<type>, context<...>
                    if (check(TokenType::OPERATOR) && peek().text == "<")
                    {
                        int depth = 1;
                        advance(); // consume <
                        // Note: advance() is a no-op at END_OF_INPUT, so the loop
                        // must terminate on is_at_end() rather than on position_.
                        while (depth > 0 && !is_at_end())
                        {
                            if (check(TokenType::OPERATOR) && peek().text == "<") depth++;
                            else if (check(TokenType::OPERATOR) && peek().text == ">") depth--;
                            if (depth > 0) advance();
                        }
                        if (depth > 0)
                        {
                            throw std::runtime_error("Unterminated parameterized type after 'instance of'");
                        }
                        if (check(TokenType::OPERATOR) && peek().text == ">")
                            advance(); // consume final >
                    }
                }
                else
                {
                    throw std::runtime_error("Expected type name after 'instance of'");
                }
                
                auto node = std::make_unique<ASTNode>(ASTNodeType::INSTANCE_OF, type_name);
                node->children.push_back(std::move(left));
                return node;
            }
            else if (check_text("in"))
            {
                advance(); // consume 'in'
                
                // Parse the right side: list, range, unary test, or positive unary tests
                auto right = parse_in_tests();
                
                auto node = std::make_unique<ASTNode>(ASTNodeType::BINARY_OP, "in");
                node->children.push_back(std::move(left));
                node->children.push_back(std::move(right));
                return node;
            }
        }
        
        while (check(TokenType::OPERATOR))
        {
            const std::string_view oper = peek().text;
            
            // Check if it's a comparison operator
            if (oper == "<" || oper == ">" || oper == "<=" || oper == ">=" || 
                oper == "=" || oper == "==" || oper == "!=")
            {
                advance(); // consume operator
                auto right = parse_additive();
                
                // Normalize == to =
                std::string normalized_op = (oper == "==") ? "=" : std::string(oper);
                
                // Create binary comparison node
                auto node = std::make_unique<ASTNode>(ASTNodeType::BINARY_OP, normalized_op);
                node->children.push_back(std::move(left));
                node->children.push_back(std::move(right));
                left = std::move(node);
            }
            else
            {
                break; // Not a comparison operator
            }
        }
        
        return left;
    }
    
    // Precedence level 4: Addition and subtraction
    std::unique_ptr<ASTNode> Parser::parse_additive()
    {
        auto left = parse_multiplicative();
        
        while (check(TokenType::OPERATOR))
        {
            const std::string_view oper = peek().text;
            
            if (oper == "+" || oper == "-")
            {
                advance(); // consume operator
                auto right = parse_multiplicative();
                
                // Create binary operator node
                auto node = std::make_unique<ASTNode>(ASTNodeType::BINARY_OP, std::string(oper));
                node->children.push_back(std::move(left));
                node->children.push_back(std::move(right));
                left = std::move(node);
            }
            else
            {
                break; // Not an additive operator
            }
        }
        
        return left;
    }
    
    // Precedence level 5: Multiplication and division
    std::unique_ptr<ASTNode> Parser::parse_multiplicative()
    {
        auto left = parse_exponentiation();
        
        while (check(TokenType::OPERATOR))
        {
            const std::string_view oper = peek().text;
            
            if (oper == "*" || oper == "/")
            {
                advance(); // consume operator
                auto right = parse_exponentiation();
                
                // Create binary operator node
                auto node = std::make_unique<ASTNode>(ASTNodeType::BINARY_OP, std::string(oper));
                node->children.push_back(std::move(left));
                node->children.push_back(std::move(right));
                left = std::move(node);
            }
            else
            {
                break; // Not a multiplicative operator
            }
        }
        
        return left;
    }
    
    // Precedence level 6 (highest binary): Exponentiation (right-associative)
    std::unique_ptr<ASTNode> Parser::parse_exponentiation()
    {
        auto left = parse_primary();

        // Postfix chain: filters (expr[...]) and property access (expr.name) may
        // interleave freely, e.g. Applicants[1].Name.
        while (check(TokenType::LBRACKET) || check(TokenType::DOT))
        {
            if (check(TokenType::LBRACKET))
            {
                advance(); // consume '['
                auto filter = parse_conditional();
                expect(TokenType::RBRACKET, "Expected ']' after filter expression");

                auto node = std::make_unique<ASTNode>(ASTNodeType::FILTER_EXPR, "filter");
                node->children.push_back(std::move(left));
                node->children.push_back(std::move(filter));
                left = std::move(node);
            }
            else
            {
                advance(); // consume '.'
                const Token& property = advance();
                auto node = std::make_unique<ASTNode>(ASTNodeType::PROPERTY_ACCESS,
                                                      std::string(property.text));
                node->children.push_back(std::move(left));
                left = std::move(node);
            }
        }
        
        // Right-associative: 2**3**4 = 2**(3**4)
        if (check(TokenType::OPERATOR) && check_text("**"))
        {
            advance(); // consume "**"
            auto right = parse_exponentiation(); // Recursive call for right-associativity
            
            // Create binary exponentiation node
            auto node = std::make_unique<ASTNode>(ASTNodeType::BINARY_OP, "**");
            node->children.push_back(std::move(left));
            node->children.push_back(std::move(right));
            return node;
        }
        
        return left;
    }
    
    // Precedence level 7 (highest): Primary expressions
    std::unique_ptr<ASTNode> Parser::parse_primary()
{
    // parse_primary is the single re-entry point of every recursive-descent
    // cycle, so bounding depth here bounds the whole parser.
    DepthGuard guard(*this);

    // Dispatch to specialized parsing methods
    if (check(TokenType::NUMBER))
    {
        return parse_number_literal();
    }
    
    if (check(TokenType::STRING))
    {
        return parse_string_literal();
    }
    
    if (check(TokenType::KEYWORD))
    {
        return parse_keyword_or_not_function();
    }
    
    if (check(TokenType::IDENTIFIER))
    {
        return parse_identifier_or_function();
    }
    
    if (check(TokenType::LPAREN))
    {
        return parse_parenthesized_expression();
    }
    
    if (check(TokenType::LBRACKET))
    {
        return parse_list_literal();
    }
    
    if (check(TokenType::LBRACE))
    {
        return parse_context_literal();
    }
    
    if (check(TokenType::OPERATOR) && check_text("-"))
    {
        return parse_unary_minus();
    }
    
    // Error: unexpected token
    std::ostringstream oss;
    oss << "Unexpected token '" << peek().text << "' at position " << peek().position;
    throw std::runtime_error(oss.str());
}

std::unique_ptr<ASTNode> Parser::parse_number_literal()
{
    const Token& token = advance();
    return std::make_unique<ASTNode>(ASTNodeType::LITERAL_NUMBER, std::string(token.text));
}

std::unique_ptr<ASTNode> Parser::parse_string_literal()
{
    const Token& token = advance();
    // Remove surrounding quotes for storage
    std::string_view text = token.text;
    if (text.length() >= 2 && text.front() == '"' && text.back() == '"')
    {
        text = text.substr(1, text.length() - 2);
    }

    return std::make_unique<ASTNode>(ASTNodeType::LITERAL_STRING, decode_string_escapes(text));
}

std::unique_ptr<ASTNode> Parser::parse_keyword_or_not_function()
{
    const Token& token = peek();
    
    // Handle true, false, null literals
    if (token.text == "true" || token.text == "false" || token.text == "null")
    {
        advance();
        return std::make_unique<ASTNode>(ASTNodeType::LITERAL_NUMBER, std::string(token.text));
    }
    
    // Special case: "not" can be a function name when followed by '('
    if (token.text == "not" && position_ + 1 < tokens_->size() && 
        (*tokens_)[position_ + 1].type == TokenType::LPAREN)
    {
        advance(); // consume 'not'
        return parse_function_call("not");
    }
    
    // Handle 'for' expression
    if (token.text == "for")
    {
        return parse_for_expression();
    }
    
    // Handle 'some' and 'every' quantified expressions
    if (token.text == "some" || token.text == "every")
    {
        return parse_quantified_expression(token.text);
    }

    // Handle 'if' so conditionals are legal in every expression position
    // (DMN 1.5 section 10.3.2.1), e.g. 1 + (if x then 2 else 3).
    if (token.text == "if")
    {
        return parse_conditional();
    }

    // Other keywords should not appear as primary expressions
    std::ostringstream oss;
    oss << "Unexpected keyword '" << token.text << "' at position " << token.position;
    throw std::runtime_error(oss.str());
}

std::unique_ptr<ASTNode> Parser::parse_identifier_or_function()
{
    const Token& token = advance();

    auto is_word_token = [this]() {
        return check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD);
    };

    auto matches_word = [this, &is_word_token](std::string_view word) {
        return is_word_token() && peek().text == word;
    };
    
    // Check for multi-word function names: "date and time", "years and months duration", "days and time duration"
    if (token.text == "date" || token.text == "years" || token.text == "days")
    {
        size_t saved = position_;
        if (check(TokenType::KEYWORD) && peek().text == "and")
        {
            advance(); // consume "and"
            if (check(TokenType::IDENTIFIER))
            {
                std::string second_word(peek().text);
                advance(); // consume second word
                
                std::string multi_name = std::string(token.text) + " and " + second_word;
                
                // "years and months duration" or "days and time duration"
                if ((multi_name == "years and months" || multi_name == "days and time") && 
                    check(TokenType::IDENTIFIER) && peek().text == "duration")
                {
                    advance(); // consume "duration"
                    multi_name += " duration";
                }
                
                if (check(TokenType::LPAREN))
                {
                    auto node = parse_function_call(multi_name);
                    return parse_postfix(std::move(node));
                }
            }
            position_ = saved; // backtrack
        }
    }
    
    // Check for multi-word function names: "day of year", "day of week", "month of year", "week of year"
    if (token.text == "day" || token.text == "month" || token.text == "week")
    {
        size_t saved = position_;
        if ((check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD)) && peek().text == "of")
        {
            advance(); // consume "of"
            if (check(TokenType::IDENTIFIER))
            {
                std::string third_word(peek().text);
                advance(); // consume third word (year/week)
                
                std::string multi_name = std::string(token.text) + " of " + third_word;
                
                if (check(TokenType::LPAREN))
                {
                    auto node = parse_function_call(multi_name);
                    return parse_postfix(std::move(node));
                }
            }
            position_ = saved; // backtrack
        }
    }

    // Check for common 2-word function names used by FEEL built-ins
    // Examples: "list contains", "insert before", "index of", "distinct values",
    // "substring before", "substring after", "starts with", "ends with",
    // "get value", "get entries", "context put", "context merge",
    // "upper case", "lower case", "round up", "round down"
    {
        size_t saved = position_;
        std::string multi_name;

        if (token.text == "list" && matches_word("contains")) multi_name = "list contains";
        else if (token.text == "list" && matches_word("replace")) multi_name = "list replace";
        else if (token.text == "insert" && matches_word("before")) multi_name = "insert before";
        else if (token.text == "index" && matches_word("of")) multi_name = "index of";
        else if (token.text == "distinct" && matches_word("values")) multi_name = "distinct values";
        else if (token.text == "substring" && matches_word("before")) multi_name = "substring before";
        else if (token.text == "substring" && matches_word("after")) multi_name = "substring after";
        else if (token.text == "starts" && matches_word("with")) multi_name = "starts with";
        else if (token.text == "ends" && matches_word("with")) multi_name = "ends with";
        else if (token.text == "get" && matches_word("value")) multi_name = "get value";
        else if (token.text == "get" && matches_word("entries")) multi_name = "get entries";
        else if (token.text == "context" && matches_word("put")) multi_name = "context put";
        else if (token.text == "context" && matches_word("merge")) multi_name = "context merge";
        else if (token.text == "upper" && matches_word("case")) multi_name = "upper case";
        else if (token.text == "lower" && matches_word("case")) multi_name = "lower case";
        else if (token.text == "round" && matches_word("up")) multi_name = "round up";
        else if (token.text == "round" && matches_word("down")) multi_name = "round down";

        if (!multi_name.empty())
        {
            advance(); // consume second word
            if (check(TokenType::LPAREN))
            {
                auto node = parse_function_call(multi_name);
                return parse_postfix(std::move(node));
            }
            position_ = saved; // backtrack if not actually a function call
        }
    }

    // Check for common 3-word function names used by FEEL built-ins
    // Examples: "round half up", "round half down", "date and time"
    if (token.text == "round")
    {
        size_t saved = position_;
        if (matches_word("half"))
        {
            advance(); // consume "half"
            if (matches_word("up") || matches_word("down"))
            {
                std::string third_word(peek().text);
                advance(); // consume "up" or "down"
                std::string multi_name = "round half " + third_word;
                if (check(TokenType::LPAREN))
                {
                    auto node = parse_function_call(multi_name);
                    return parse_postfix(std::move(node));
                }
            }
        }
        position_ = saved; // backtrack
    }
    
    // Check if this is a function call (followed by left parenthesis)
    if (check(TokenType::LPAREN))
    {
        auto node = parse_function_call(token.text);
        // Check for postfix property access and filter on function call result
        return parse_postfix(std::move(node));
    }
    
    // Not a function call - handle as variable with potential property access
    return parse_variable_with_properties(token.text);
}

std::unique_ptr<ASTNode> Parser::parse_function_call(std::string_view function_name)
{
    advance(); // consume '('
    
    auto func_node = std::make_unique<ASTNode>(ASTNodeType::FUNCTION_CALL, std::string(function_name));
    
    // Parse arguments (if any)
    if (!check(TokenType::RPAREN))
    {
        parse_function_parameters(func_node.get(), function_name);
    }
    
    expect(TokenType::RPAREN, "Expected ')' after function arguments");
    return func_node;
}

void Parser::parse_function_parameters(ASTNode* func_node, std::string_view function_name)
{
    bool has_named_params = false;
    bool has_positional_params = false;
    
    while (true)
    {
        // Try to detect if this is a named parameter by looking ahead
        // Named parameter pattern: identifier : expression
        bool is_named_param = false;
        std::string param_name;
        
        if (check(TokenType::IDENTIFIER))
        {
            // Peek ahead to see if followed by colon
            size_t saved_pos = position_;
            Token ident_token = advance(); // consume identifier
            
            if (check(TokenType::COLON))
            {
                // This is a named parameter
                is_named_param = true;
                param_name = std::string(ident_token.text);
                advance(); // consume ':'
            }
            else
            {
                // Not a named parameter, backtrack
                position_ = saved_pos;
            }
        }
        
        // Validate that we don't mix named and positional parameters
        if (is_named_param)
        {
            has_named_params = true;
            if (has_positional_params)
            {
                std::ostringstream oss;
                oss << "Cannot mix named and positional parameters in function call '" 
                    << function_name << "' at position " << peek().position;
                throw std::runtime_error(oss.str());
            }
        }
        else
        {
            has_positional_params = true;
            if (has_named_params)
            {
                std::ostringstream oss;
                oss << "Cannot mix named and positional parameters in function call '" 
                    << function_name << "' at position " << peek().position;
                throw std::runtime_error(oss.str());
            }
        }
        
        // Parse the value expression
        auto value_expr = parse_conditional();
        
        // Store parameter in the parameters vector
        FunctionParameter param;
        param.name = param_name; // Empty for positional params
        param.valueExpr = std::move(value_expr);
        func_node->parameters.push_back(std::move(param));
        
        // Check for comma (more arguments)
        if (check(TokenType::COMMA))
        {
            advance(); // consume ','
        }
        else
        {
            break; // No more arguments
        }
    }
}

std::unique_ptr<ASTNode> Parser::parse_variable_with_properties(std::string_view var_name)
{
    auto node = std::make_unique<ASTNode>(ASTNodeType::VARIABLE, std::string(var_name));
    
    // Check for property access (.property)
    while (check(TokenType::DOT))
    {
        advance(); // consume '.'
        
        // Expect identifier after dot
        if (!check(TokenType::IDENTIFIER))
        {
            std::ostringstream oss;
            oss << "Expected property name after '.' at position " << peek().position;
            throw std::runtime_error(oss.str());
        }
        
        const Token& prop_token = advance();
        
        // Create property access node
        auto prop_access = std::make_unique<ASTNode>(ASTNodeType::PROPERTY_ACCESS, std::string(prop_token.text));
        prop_access->children.push_back(std::move(node));
        node = std::move(prop_access);
    }
    
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_parenthesized_expression()
{
    advance(); // consume '('
    auto expr = parse_conditional(); // Parse inner expression (start from lowest precedence)
    
    // Check if this is a range: (expr..expr] or (expr..expr)
    if (check(TokenType::DOTDOT)) {
        advance(); // consume ..
        auto end_expr = parse_additive();
        
        std::string range_type = "(";
        if (check(TokenType::RBRACKET)) {
            range_type += "]";
            advance();
        } else if (check(TokenType::RPAREN)) {
            range_type += ")";
            advance();
        } else {
            throw std::runtime_error("Expected ']' or ')' to close range");
        }
        
        auto node = std::make_unique<ASTNode>(ASTNodeType::RANGE, range_type);
        node->children.push_back(std::move(expr));
        node->children.push_back(std::move(end_expr));
        return node;
    }
    
    expect(TokenType::RPAREN, "Expected ')' after expression");
    
    // Check for property access after parenthesized expression
    while (check(TokenType::DOT))
    {
        advance(); // consume '.'
        
        // Expect identifier after dot
        if (!check(TokenType::IDENTIFIER))
        {
            std::ostringstream oss;
            oss << "Expected property name after '.' at position " << peek().position;
            throw std::runtime_error(oss.str());
        }
        
        const Token& prop_token = advance();
        
        // Create property access node
        auto prop_access = std::make_unique<ASTNode>(ASTNodeType::PROPERTY_ACCESS, std::string(prop_token.text));
        prop_access->children.push_back(std::move(expr));
        expr = std::move(prop_access);
    }
    
    return expr;
}

std::unique_ptr<ASTNode> Parser::parse_postfix(std::unique_ptr<ASTNode> node)
{
    // Handle property access (.property) and filter expressions ([filter])
    while (check(TokenType::DOT) || check(TokenType::LBRACKET))
    {
        if (check(TokenType::DOT))
        {
            advance(); // consume '.'
            if (!check(TokenType::IDENTIFIER) && !check(TokenType::KEYWORD))
            {
                return node; // no valid property name
            }
            const Token& prop_token = advance();
            auto prop_access = std::make_unique<ASTNode>(ASTNodeType::PROPERTY_ACCESS, std::string(prop_token.text));
            prop_access->children.push_back(std::move(node));
            node = std::move(prop_access);
        }
        else if (check(TokenType::LBRACKET))
        {
            advance(); // consume '['
            auto filter = parse_conditional();
            if (!check(TokenType::RBRACKET))
                throw std::runtime_error("Expected ']' in filter expression");
            advance(); // consume ']'
            auto filter_node = std::make_unique<ASTNode>(ASTNodeType::FILTER_EXPR, "filter");
            filter_node->children.push_back(std::move(node));
            filter_node->children.push_back(std::move(filter));
            node = std::move(filter_node);
        }
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_list_literal()
{
    advance(); // consume '['
    
    // Check if this is a range: [expr..expr] or [expr..expr)
    // Parse first element, then check for ..
    if (!check(TokenType::RBRACKET))
    {
        auto first = parse_conditional();
        
        // If we see .., this is a range not a list
        if (check(TokenType::DOTDOT)) {
            advance(); // consume ..
            auto end_expr = parse_additive();
            
            std::string range_type = "[";
            if (check(TokenType::RBRACKET)) {
                range_type += "]";
                advance();
            } else if (check(TokenType::RPAREN)) {
                range_type += ")";
                advance();
            } else {
                throw std::runtime_error("Expected ']' or ')' to close range");
            }
            
            auto node = std::make_unique<ASTNode>(ASTNodeType::RANGE, range_type);
            node->children.push_back(std::move(first));
            node->children.push_back(std::move(end_expr));
            return node;
        }
        
        // Not a range - continue as list
        auto list_node = std::make_unique<ASTNode>(ASTNodeType::LITERAL_LIST, "");
        list_node->children.push_back(std::move(first));
        
        while (check(TokenType::COMMA))
        {
            advance(); // consume ','
            // Allow trailing comma
            if (check(TokenType::RBRACKET))
            {
                break;
            }
            list_node->children.push_back(parse_conditional());
        }
        
        expect(TokenType::RBRACKET, "Expected ']' after list elements");
        return list_node;
    }
    
    // Empty list
    auto list_node = std::make_unique<ASTNode>(ASTNodeType::LITERAL_LIST, "");
    expect(TokenType::RBRACKET, "Expected ']' after list elements");
    return list_node;
}

std::unique_ptr<ASTNode> Parser::parse_unary_minus()
{
    advance(); // consume '-'
    auto operand = parse_primary();
    
    // Create unary operator node
    auto node = std::make_unique<ASTNode>(ASTNodeType::UNARY_OP, "-");
    node->children.push_back(std::move(operand));
    return node;
}

[[nodiscard]] std::string Parser::parse_context_key()
{
    // Fast path for the common FEEL case: simple identifier key directly
    // followed by ':'. Keep the generic path for additional-name-symbol keys
    // such as foo+bar.
    if (check(TokenType::IDENTIFIER) &&
        position_ + 1 < tokens_->size() &&
        (*tokens_)[position_ + 1].type == TokenType::COLON)
    {
        return std::string(advance().text);
    }

    if (check(TokenType::STRING))
    {
        std::string_view key = advance().text;
        // Remove quotes if present
        if (key.size() >= 2 && key.front() == '"' && key.back() == '"')
        {
            key = key.substr(1, key.size() - 2);
        }
        return decode_string_escapes(key);
    }

    // Unquoted context keys may include FEEL additional-name-symbols.
    // Consume tokens verbatim until ':' and then trim leading/trailing spaces.
    std::string key;
    key.reserve(32);
    while (!is_at_end() && !check(TokenType::COLON))
    {
        if (check(TokenType::COMMA) || check(TokenType::RBRACE))
        {
            break;
        }
        const Token& token = advance();
        key.append(token.text.data(), token.text.size());
    }

    if (!key.empty())
    {
        size_t begin = 0;
        while (begin < key.size() && std::isspace(static_cast<unsigned char>(key[begin])) != 0)
        {
            ++begin;
        }

        size_t end = key.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(key[end - 1])) != 0)
        {
            --end;
        }

        if (begin < end)
        {
            return key.substr(begin, end - begin);
        }
    }
    
    std::ostringstream oss;
    oss << "Expected identifier or string for context key at position " << peek().position;
    throw std::runtime_error(oss.str());
}

void Parser::parse_context_entry(std::unique_ptr<ASTNode>& context_node)
{
    // Parse key and colon, then value expression
    const std::string key = parse_context_key();
    expect(TokenType::COLON, "Expected ':' after context key");
    
    // Store key-value pair as child nodes
    auto key_node = std::make_unique<ASTNode>(ASTNodeType::LITERAL_STRING, key);
    context_node->children.push_back(std::move(key_node));
    context_node->children.push_back(parse_conditional());
}

std::unique_ptr<ASTNode> Parser::parse_context_literal()
{
    advance(); // consume '{'
    auto context_node = std::make_unique<ASTNode>(ASTNodeType::LITERAL_CONTEXT, "");
    
    if (!check(TokenType::RBRACE))
    {
        while (!check(TokenType::RBRACE))
        {
            parse_context_entry(context_node);
            
            // Check for comma (more entries)
            if (check(TokenType::COMMA))
            {
                advance(); // consume ','
                if (check(TokenType::RBRACE)) break; // Allow trailing comma
            }
            else break;
        }
    }
    
    expect(TokenType::RBRACE, "Expected '}' after context entries");
    return context_node;
}

    nlohmann::json Parser::eval_expression(std::string_view expression, const nlohmann::json& input, const EvaluationContext& eval_ctx)
    {
        // Use the existing Evaluator which already provides this functionality
        return Evaluator::evaluate(expression, input, eval_ctx);
    }

std::unique_ptr<ASTNode> Parser::parse_for_expression()
{
    advance(); // consume 'for'
    
    // for x in expr1 [, y in expr2 ...] return expr
    auto node = std::make_unique<ASTNode>(ASTNodeType::FOR_EXPR, "for");
    
    // Parse iteration bindings: var in list [, var in list ...]
    do
    {
        // Parse variable name
        if (!check(TokenType::IDENTIFIER))
        {
            throw std::runtime_error("Expected variable name in 'for' expression");
        }
        auto var_name = std::string(advance().text);
        
        // Expect 'in'
        if (!check(TokenType::KEYWORD) || !check_text("in"))
        {
            throw std::runtime_error("Expected 'in' after variable name in 'for' expression");
        }
        advance(); // consume 'in'
        
        // Parse the list expression
        auto list_expr = parse_additive();
        
        // Check for bare range: expr..expr (without brackets)
        if (check(TokenType::DOTDOT))
        {
            advance(); // consume ..
            auto end_expr = parse_additive();
            auto range_node = std::make_unique<ASTNode>(ASTNodeType::RANGE, ".."); // bare iteration range
            range_node->children.push_back(std::move(list_expr));
            range_node->children.push_back(std::move(end_expr));
            list_expr = std::move(range_node);
        }
        
        // Store as: VARIABLE(name), list_expr pairs
        auto var_node = std::make_unique<ASTNode>(ASTNodeType::VARIABLE, var_name);
        node->children.push_back(std::move(var_node));
        node->children.push_back(std::move(list_expr));
        
    } while (check(TokenType::COMMA) && (advance(), true)); // consume comma and continue
    
    // Expect 'return'
    if (!check(TokenType::KEYWORD) || !check_text("return"))
    {
        throw std::runtime_error("Expected 'return' in 'for' expression");
    }
    advance(); // consume 'return'
    
    // Parse the return expression
    auto return_expr = parse_conditional();
    node->children.push_back(std::move(return_expr));
    
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_quantified_expression(std::string_view quantifier)
{
    advance(); // consume 'some' or 'every'
    
    // some/every x in list [, y in list ...] satisfies condition
    auto node = std::make_unique<ASTNode>(ASTNodeType::QUANTIFIED_EXPR, std::string(quantifier));
    
    // Parse iteration bindings
    do
    {
        if (!check(TokenType::IDENTIFIER))
        {
            throw std::runtime_error("Expected variable name in quantified expression");
        }
        auto var_name = std::string(advance().text);
        
        if (!check(TokenType::KEYWORD) || !check_text("in"))
        {
            throw std::runtime_error("Expected 'in' after variable name in quantified expression");
        }
        advance(); // consume 'in'
        
        auto list_expr = parse_additive();
        
        auto var_node = std::make_unique<ASTNode>(ASTNodeType::VARIABLE, var_name);
        node->children.push_back(std::move(var_node));
        node->children.push_back(std::move(list_expr));
        
    } while (check(TokenType::COMMA) && (advance(), true));
    
    // Expect 'satisfies'
    if (!check(TokenType::KEYWORD) || !check_text("satisfies"))
    {
        std::ostringstream oss;
        oss << "Expected 'satisfies' in quantified expression at position " << peek().position;
        throw std::runtime_error(oss.str());
    }
    advance(); // consume 'satisfies'
    
    // Parse the condition expression
    auto condition = parse_conditional();
    node->children.push_back(std::move(condition));
    
    return node;
}

// Parse the right-hand side of an 'in' expression
// Can be: range [1..10], (1..10), list [1,2,3], unary test <= 10, or value
std::unique_ptr<ASTNode> Parser::parse_in_tests()
{
    // Check for unary test: operator followed by expression (e.g., <= 10, > 5, = 10, != 10)
    if (check(TokenType::OPERATOR)) {
        const std::string_view op = peek().text;
        if (op == "<" || op == ">" || op == "<=" || op == ">=" || op == "=" || op == "!=") {
            advance(); // consume operator
            auto operand = parse_additive();
            auto node = std::make_unique<ASTNode>(ASTNodeType::UNARY_TEST, std::string(op));
            node->children.push_back(std::move(operand));
            return node;
        }
    }
    
    // Check for range or list starting with [ or (
    if (check(TokenType::LBRACKET) || check(TokenType::LPAREN)) {
        TokenType open = peek().type;
        size_t saved_pos = position_;
        
        advance(); // consume [ or (
        
        // For '(' - could be: range (expr..expr), or positive unary tests list (test, test, ...)
        // For '[' - could be: range [expr..expr], or a list [expr, expr, ...]
        
        // Try parsing first element as a unary test or expression
        auto first = parse_single_unary_test();
        
        if (check(TokenType::DOTDOT)) {
            // This is a range!
            advance(); // consume ..
            auto end_expr = parse_additive();
            
            // Determine closing bracket
            std::string range_type;
            if (open == TokenType::LBRACKET) range_type += "[";
            else range_type += "(";
            
            if (check(TokenType::RBRACKET)) {
                range_type += "]";
                advance();
            } else if (check(TokenType::RPAREN)) {
                range_type += ")";
                advance();
            } else {
                throw std::runtime_error("Expected ']' or ')' to close range");
            }
            
            auto node = std::make_unique<ASTNode>(ASTNodeType::RANGE, range_type);
            node->children.push_back(std::move(first));
            node->children.push_back(std::move(end_expr));
            return node;
        }
        
        if (check(TokenType::COMMA)) {
            // Comma-separated list of positive unary tests: (test1, test2, ...)
            auto list_node = std::make_unique<ASTNode>(ASTNodeType::LITERAL_LIST, "list");
            list_node->children.push_back(std::move(first));
            while (check(TokenType::COMMA)) {
                advance(); // consume comma
                list_node->children.push_back(parse_single_unary_test());
            }
            if (open == TokenType::LBRACKET) {
                if (!check(TokenType::RBRACKET))
                    throw std::runtime_error("Expected ']' to close list");
                advance();
            } else {
                if (!check(TokenType::RPAREN))
                    throw std::runtime_error("Expected ')' to close list");
                advance();
            }
            return list_node;
        }
        
        // Single element in parens: (test) or [value]
        if (open == TokenType::LPAREN && check(TokenType::RPAREN)) {
            advance(); // consume )
            // Wrap single test in a list for consistent evaluation
            auto list_node = std::make_unique<ASTNode>(ASTNodeType::LITERAL_LIST, "list");
            list_node->children.push_back(std::move(first));
            return list_node;
        }
        
        if (open == TokenType::LBRACKET && check(TokenType::RBRACKET)) {
            // Single element list [value]
            advance();
            auto list_node = std::make_unique<ASTNode>(ASTNodeType::LITERAL_LIST, "list");
            list_node->children.push_back(std::move(first));
            return list_node;
        }
        
        // Didn't match expected patterns - backtrack and parse normally
        position_ = saved_pos;
        return parse_additive();
    }
    
    // Check for 'not' keyword: not(tests)
    if (check(TokenType::KEYWORD) && check_text("not")) {
        size_t saved_pos = position_;
        advance(); // consume 'not'
        if (check(TokenType::LPAREN)) {
            advance(); // consume (
            // Parse comma-separated unary tests inside not(...)
            auto list_node = std::make_unique<ASTNode>(ASTNodeType::LITERAL_LIST, "list");
            list_node->children.push_back(parse_single_unary_test());
            while (check(TokenType::COMMA)) {
                advance();
                list_node->children.push_back(parse_single_unary_test());
            }
            if (!check(TokenType::RPAREN))
                throw std::runtime_error("Expected ')' after not(...)");
            advance();
            // Wrap in a NOT unary test
            auto node = std::make_unique<ASTNode>(ASTNodeType::UNARY_TEST, "not");
            node->children.push_back(std::move(list_node));
            return node;
        }
        position_ = saved_pos;
    }
    
    // Default: parse as a normal expression (value or list)
    return parse_additive();
}

std::unique_ptr<ASTNode> Parser::parse_single_unary_test()
{
    // Parse a single positive unary test: operator+expr, range, or plain value
    if (check(TokenType::OPERATOR)) {
        const std::string_view op = peek().text;
        if (op == "<" || op == ">" || op == "<=" || op == ">=" || op == "=" || op == "!=") {
            advance();
            auto operand = parse_additive();
            auto node = std::make_unique<ASTNode>(ASTNodeType::UNARY_TEST, std::string(op));
            node->children.push_back(std::move(operand));
            return node;
        }
    }
    // Check for range starting with [ or (
    if (check(TokenType::LBRACKET) || check(TokenType::LPAREN)) {
        TokenType bracket = peek().type;
        size_t saved = position_;
        advance();
        auto start_expr = parse_additive();
        if (check(TokenType::DOTDOT)) {
            advance();
            auto end_expr = parse_additive();
            std::string range_type;
            range_type += (bracket == TokenType::LBRACKET) ? "[" : "(";
            if (check(TokenType::RBRACKET)) { range_type += "]"; advance(); }
            else if (check(TokenType::RPAREN)) { range_type += ")"; advance(); }
            else throw std::runtime_error("Expected ']' or ')' to close range");
            auto node = std::make_unique<ASTNode>(ASTNodeType::RANGE, range_type);
            node->children.push_back(std::move(start_expr));
            node->children.push_back(std::move(end_expr));
            return node;
        }
        position_ = saved;
    }
    return parse_additive();
}

// Parse a range expression when we know the opening bracket type
// Called from parse_list_literal when we detect .. inside a list
std::unique_ptr<ASTNode> Parser::parse_range(TokenType open_bracket)
{
    auto start_expr = parse_additive();
    
    if (!check(TokenType::DOTDOT)) {
        throw std::runtime_error("Expected '..' in range expression");
    }
    advance(); // consume ..
    
    auto end_expr = parse_additive();
    
    std::string range_type;
    if (open_bracket == TokenType::LBRACKET) range_type += "[";
    else range_type += "(";
    
    if (check(TokenType::RBRACKET)) {
        range_type += "]";
        advance();
    } else if (check(TokenType::RPAREN)) {
        range_type += ")";
        advance();
    } else {
        throw std::runtime_error("Expected ']' or ')' to close range");
    }
    
    auto node = std::make_unique<ASTNode>(ASTNodeType::RANGE, range_type);
    node->children.push_back(std::move(start_expr));
    node->children.push_back(std::move(end_expr));
    return node;
}

} // namespace orion::bre
