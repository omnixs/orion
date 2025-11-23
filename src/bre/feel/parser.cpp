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
#include <sstream>

namespace orion::bre::feel {
    std::unique_ptr<ASTNode> Parser::parse(const std::vector<Token>& tokens)
    {
        tokens_ = &tokens;
        position_ = 0;
        
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
            auto condition = parse_logical_or();
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
        
        while (check(TokenType::OPERATOR))
        {
            const std::string& oper = peek().text;
            
            // Check if it's a comparison operator
            if (oper == "<" || oper == ">" || oper == "<=" || oper == ">=" || 
                oper == "=" || oper == "==" || oper == "!=")
            {
                advance(); // consume operator
                auto right = parse_additive();
                
                // Normalize == to =
                std::string normalized_op = (oper == "==") ? "=" : oper;
                
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
            const std::string& oper = peek().text;
            
            if (oper == "+" || oper == "-")
            {
                advance(); // consume operator
                auto right = parse_multiplicative();
                
                // Create binary operator node
                auto node = std::make_unique<ASTNode>(ASTNodeType::BINARY_OP, oper);
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
            const std::string& oper = peek().text;
            
            if (oper == "*" || oper == "/")
            {
                advance(); // consume operator
                auto right = parse_exponentiation();
                
                // Create binary operator node
                auto node = std::make_unique<ASTNode>(ASTNodeType::BINARY_OP, oper);
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
    return std::make_unique<ASTNode>(ASTNodeType::LITERAL_NUMBER, token.text);
}

std::unique_ptr<ASTNode> Parser::parse_string_literal()
{
    const Token& token = advance();
    // Remove surrounding quotes for storage
    std::string text = token.text;
    if (text.length() >= 2 && text.front() == '"' && text.back() == '"')
    {
        text = text.substr(1, text.length() - 2);
    }
    return std::make_unique<ASTNode>(ASTNodeType::LITERAL_STRING, text);
}

std::unique_ptr<ASTNode> Parser::parse_keyword_or_not_function()
{
    const Token& token = peek();
    
    // Handle true, false, null literals
    if (token.text == "true" || token.text == "false" || token.text == "null")
    {
        advance();
        return std::make_unique<ASTNode>(ASTNodeType::LITERAL_NUMBER, token.text);
    }
    
    // Special case: "not" can be a function name when followed by '('
    if (token.text == "not" && position_ + 1 < tokens_->size() && 
        (*tokens_)[position_ + 1].type == TokenType::LPAREN)
    {
        advance(); // consume 'not'
        return parse_function_call("not");
    }
    
    // Other keywords should not appear as primary expressions
    std::ostringstream oss;
    oss << "Unexpected keyword '" << token.text << "' at position " << token.position;
    throw std::runtime_error(oss.str());
}

std::unique_ptr<ASTNode> Parser::parse_identifier_or_function()
{
    const Token& token = advance();
    
    // Check if this is a function call (followed by left parenthesis)
    if (check(TokenType::LPAREN))
    {
        return parse_function_call(token.text);
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
                param_name = ident_token.text;
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
        auto value_expr = parse_logical_or();
        
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
        auto prop_access = std::make_unique<ASTNode>(ASTNodeType::PROPERTY_ACCESS, prop_token.text);
        prop_access->children.push_back(std::move(node));
        node = std::move(prop_access);
    }
    
    return node;
}

std::unique_ptr<ASTNode> Parser::parse_parenthesized_expression()
{
    advance(); // consume '('
    auto expr = parse_logical_or(); // Parse inner expression (start from lowest precedence)
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
        auto prop_access = std::make_unique<ASTNode>(ASTNodeType::PROPERTY_ACCESS, prop_token.text);
        prop_access->children.push_back(std::move(expr));
        expr = std::move(prop_access);
    }
    
    return expr;
}

std::unique_ptr<ASTNode> Parser::parse_list_literal()
{
    advance(); // consume '['
    
    auto list_node = std::make_unique<ASTNode>(ASTNodeType::LITERAL_LIST, "");
    
    // Parse list elements (if any)
    if (!check(TokenType::RBRACKET))
    {
        while (true)
        {
            list_node->children.push_back(parse_logical_or());
            
            // Check for comma (more elements)
            if (check(TokenType::COMMA))
            {
                advance(); // consume ','
                // Allow trailing comma
                if (check(TokenType::RBRACKET))
                {
                    break;
                }
            }
            else
            {
                break; // No more elements
            }
        }
    }
    
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

    nlohmann::json Parser::eval_expression(std::string_view expression, const nlohmann::json& context)
    {
        // Use the existing Evaluator which already provides this functionality
        return Evaluator::evaluate(expression, context);
    }
} // namespace orion::bre
