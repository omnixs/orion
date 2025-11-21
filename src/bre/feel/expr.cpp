#include <orion/bre/feel/expr.hpp>
#include <orion/bre/feel/parser.hpp>

#include <nlohmann/json.hpp>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <cmath>
#include <limits>
#include <regex>
#include <algorithm>
#include <variant>
#include "util_internal.hpp"

// FEEL evaluator: lexer + AST + evaluator (subset for DMN 1.5 literal expressions)
// Supports: numbers, strings, names (with spaces / dots), + - * / **, unary + -, comparisons, logical and/or/not, parentheses,
// ranges [a..b], IN lists, BETWEEN, three-valued logic with null propagation, function calls (matches, all, any).

namespace orion::bre::feel {
    using json = nlohmann::json;

namespace {  // Anonymous namespace to prevent collision with main parser

    // ---------------- Value model ----------------
    using Decimal = long double;

    struct Range
    {
        Decimal start{0};
        Decimal end{0};
        bool inc_start{true};
        bool inc_end{true};
    };

    struct Value;

    struct Value
    {
        using List = std::vector<Value>;
        std::variant<std::nullptr_t, bool, Decimal, std::string, Range, List> v;

        Value(): v(nullptr)
        {
        }

        Value(std::nullptr_t): v(nullptr)
        {
    }

    Value(bool bool_val): v(bool_val)
    {
    }

    Value(Decimal decimal): v(decimal)
    {
    }

    Value(double dbl): v(Decimal(dbl))
    {
    }

    Value(const std::string& str): v(str)
    {
    }

    Value(std::string&& str): v(std::move(str))
    {
    }

    Value(const char* str): v(std::string(str))
    {
    }

    Value(const Range& rng): v(rng)
    {
    }

    Value(List lst): v(std::move(lst))
    {
    }

    [[nodiscard]] bool is_null() const { return std::holds_alternative<std::nullptr_t>(v); }
    [[nodiscard]] bool is_bool() const { return std::holds_alternative<bool>(v); }
    [[nodiscard]] bool is_num() const { return std::holds_alternative<Decimal>(v); }
    [[nodiscard]] bool is_str() const { return std::holds_alternative<std::string>(v); }
    [[nodiscard]] bool is_range() const { return std::holds_alternative<Range>(v); }
    [[nodiscard]] bool is_list() const { return std::holds_alternative<List>(v); }
    [[nodiscard]] Decimal num() const { return std::get<Decimal>(v); }
    [[nodiscard]] const std::string& str() const { return std::get<std::string>(v); }
    [[nodiscard]] bool bval() const { return std::get<bool>(v); }
    [[nodiscard]] const Range& range() const { return std::get<Range>(v); }
    [[nodiscard]] const List& list() const { return std::get<List>(v); }
};

// ---------------- Lexer ----------------
enum class TokKind : std::uint8_t
{
    END, INVALID, NUMBER, STRING, NAME, PLUS, MINUS, STAR, SLASH, POW, LPAREN, RPAREN, LBRACK, RBRACK, COMMA,
    DOTDOT, EQ, NEQ, LT, LE, GT, GE, AND, OR, NOT, IN, BETWEEN, AND_SYM, UNKNOWN, DOT
};  struct Token
    {
        TokKind k{TokKind::END};
        std::string text;
        long double num{0};
    };

    struct Lexer
    {
        std::string src;
    size_t i{0};

    Lexer(std::string_view source): src(source)
    {
    }       char peek() const { return i < src.size() ? src[i] : '\0'; }
        char get() { return i < src.size() ? src[i++] : '\0'; }
        static bool is_ident_start(char c) { return (std::isalpha((unsigned char)c) != 0) || c == '_'; }  // Explicit bool conversion
        static bool is_ident_char(char c) { return (std::isalnum((unsigned char)c) != 0) || c == '_' || c == ' ' || c == '.'; }  // Explicit bool conversion
        void skip_ws() { while (std::isspace((unsigned char)peek()) != 0) { get();  // Explicit bool conversion
}}

        // Helper: Parse number token (including scientific notation)
        Token parse_number_token()
        {
            Token t;
            std::string buf;
            bool dot = false;
            while ((std::isdigit((unsigned char)peek()) != 0) || peek() == '.')  // Explicit bool conversion
            {
                if (peek() == '.') {
                    if (dot) { break; }
                    dot = true;
                }
                buf.push_back(get());
            }
            if (peek() == 'e' || peek() == 'E')
            {
                buf.push_back(get());
                if (peek() == '+' || peek() == '-') { buf.push_back(get()); }
                while (std::isdigit((unsigned char)peek()) != 0) { buf.push_back(get()); }  // Explicit bool conversion
            }
            try {
                t.num = std::stold(buf);
                t.k = TokKind::NUMBER;
                t.text = buf;
            }
            catch (...) {
                t.k = TokKind::INVALID;
            }
            return t;
        }

        // Helper: Parse string token (handles escaping)
        Token parse_string_token(char quote)
        {
            Token t;
            get();  // consume quote
            std::string buf;
            while (peek() != '\0' && peek() != quote)  // Explicit char comparison
            {
                if (peek() == '\\') {
                    buf.push_back(get());
                    if (peek() != '\0') { buf.push_back(get()); }  // Explicit char comparison
                }
                else { buf.push_back(get()); }
            }
            if (peek() == quote) { get(); }
            t.k = TokKind::STRING;
            t.text = buf;
            return t;
        }

        // Helper: Parse identifier or keyword token
        Token parse_ident_or_keyword()
        {
            Token t;
            std::string buf;
            buf.push_back(get());
            while (is_ident_char(peek())) { buf.push_back(get()); }
            while (!buf.empty() && buf.back() == ' ') { buf.pop_back(); }
            
            std::string low;
            low.reserve(buf.size());
            for (char ch : buf) { low.push_back((char)std::tolower((unsigned char)ch)); }
            
            if (low == "and") { t.k = TokKind::AND; t.text = buf; }
            else if (low == "or") { t.k = TokKind::OR; t.text = buf; }
            else if (low == "not") { t.k = TokKind::NOT; t.text = buf; }
            else if (low == "in") { t.k = TokKind::IN; t.text = buf; }
            else if (low == "between") { t.k = TokKind::BETWEEN; t.text = buf; }
            else { t.k = TokKind::NAME; t.text = buf; }
            return t;
        }

        Token next()
        {
            skip_ws();
            Token t;
            char c = peek();
            if (c == '\0')  // Explicit char comparison
            {
                t.k = TokKind::END;
                return t;
            }
            // numbers
            if ((std::isdigit((unsigned char)c) != 0) || (c == '.' && (std::isdigit(  // Explicit bool conversion
                (unsigned char)(i + 1 < src.size() ? src[i + 1] : '0')) != 0)))
            {
                return parse_number_token();
            }
            // string
            if (c == '"' || c == '\'')
            {
                return parse_string_token(c);
            }
            // identifiers / keywords
            if (is_ident_start(c))
            {
                return parse_ident_or_keyword();
            }
            // two-char tokens
            if (c == '*' && i + 1 < src.size() && src[i + 1] == '*')
            {
                i += 2;
                t.k = TokKind::POW;
                t.text = "**";
                return t;
            }
            if (c == '.' && i + 1 < src.size() && src[i + 1] == '.')
            {
                i += 2;
                t.k = TokKind::DOTDOT;
                t.text = "..";
                return t;
            }
            if (c == '!' && i + 1 < src.size() && src[i + 1] == '=')
            {
                i += 2;
                t.k = TokKind::NEQ;
                t.text = "!=";
                return t;
            }
            if (c == '<' && i + 1 < src.size() && src[i + 1] == '=')
            {
                i += 2;
                t.k = TokKind::LE;
                t.text = "<=";
                return t;
            }
            if (c == '>' && i + 1 < src.size() && src[i + 1] == '=')
            {
                i += 2;
                t.k = TokKind::GE;
                t.text = ">=";
                return t;
            }
            // single-char
            get();
            switch (c)
            {
            case '+': t.k = TokKind::PLUS;
                break;
            case '-': t.k = TokKind::MINUS;
                break;
            case '*': t.k = TokKind::STAR;
                break;
            case '/': t.k = TokKind::SLASH;
                break;
            case '(': t.k = TokKind::LPAREN;
                break;
            case ')': t.k = TokKind::RPAREN;
                break;
            case '[': t.k = TokKind::LBRACK;
                break;
            case ']': t.k = TokKind::RBRACK;
                break;
            case ',': t.k = TokKind::COMMA;
                break;
            case '=': t.k = TokKind::EQ;
                break;
            case '<': t.k = TokKind::LT;
                break;
            case '>': t.k = TokKind::GT;
                break;
            default: break;
            }
            if (t.k != TokKind::END && t.k != TokKind::INVALID && t.k != TokKind::UNKNOWN) { return t;
}
            t.k = TokKind::UNKNOWN;
            t.text = std::string(1, c);
            return t;
        }
    };

    // ---------------- AST ----------------
    struct Expr
    {
        virtual ~Expr() = default;
    };

    using ExprPtr = std::unique_ptr<Expr>;

    struct ENull : Expr
    {
    };

    struct EBool : Expr
    {
        bool v;

        EBool(bool b): v(b)
        {
        }
    };

    struct ENum : Expr
    {
        Decimal v;

        ENum(Decimal d): v(d)
        {
        }
    };

    struct EStr : Expr
    {
        std::string v;

        EStr(std::string s): v(std::move(s))
        {
        }
    };

    struct EName : Expr
    {
        std::string name;

        explicit EName(std::string n): name(std::move(n))
        {
        }
    };

    struct EUnary : Expr
    {
        std::string op;
        ExprPtr rhs;

        EUnary(std::string o, ExprPtr r): op(std::move(o)), rhs(std::move(r))
        {
        }
    };

    struct EBinary : Expr
    {
        std::string op;
        ExprPtr lhs, rhs;

        EBinary(std::string o, ExprPtr a, ExprPtr b): op(std::move(o)), lhs(std::move(a)), rhs(std::move(b))
        {
        }
    };

    struct EIn : Expr
    {
        ExprPtr item;
        std::vector<ExprPtr> list;

        EIn(ExprPtr it, std::vector<ExprPtr> ls): item(std::move(it)), list(std::move(ls))
        {
        }
    };

    struct ERange : Expr
    {
        ExprPtr a;
        ExprPtr b;
        bool incA;
        bool incB;

        ERange(ExprPtr x, ExprPtr y, bool ia, bool ib): a(std::move(x)), b(std::move(y)), incA(ia), incB(ib)
        {
        }
    };

    struct EBetween : Expr
    {
        ExprPtr val;
        ExprPtr lo;
        ExprPtr hi;

        EBetween(ExprPtr v, ExprPtr l, ExprPtr h): val(std::move(v)), lo(std::move(l)), hi(std::move(h))
        {
        }
    };

    struct EList : Expr
    {
        std::vector<ExprPtr> items;

        explicit EList(std::vector<ExprPtr> v): items(std::move(v))
        {
        }
    };

    struct ECall : Expr
    {
        std::string name;
        std::vector<ExprPtr> args;

        ECall(std::string n, std::vector<ExprPtr> a): name(std::move(n)), args(std::move(a))
        {
        }
    };

    // ---------------- Parser ----------------
    struct Parser
    {
        Lexer lex;
        Token cur;
        const json& ctx;
        bool ok{true};
        std::string err;
        Parser(std::string_view s, const json& c): lex(s), ctx(c) { cur = lex.next(); }

        bool accept(TokKind k)
        {
            if (cur.k == k)
            {
                cur = lex.next();
                return true;
            }
            return false;
        }

        bool expect(TokKind k)
        {
            if (!accept(k))
            {
                ok = false;
                err = "expected token";
                return false;
            }
            return true;
        }

        ExprPtr parse()
        {
            auto e = parse_or();
            if (cur.k != TokKind::END)
            {
                ok = false;
                err = "trailing tokens";
            }
            return e;
        }

        ExprPtr parse_or()
        {
            auto e = parse_and();
            while (cur.k == TokKind::OR)
            {
                cur = lex.next();
                auto r = parse_and();
                e = std::make_unique<EBinary>("or", std::move(e), std::move(r));
            }
            return e;
        }

        ExprPtr parse_and()
        {
            auto e = parse_comparison();
            while (cur.k == TokKind::AND)
            {
                cur = lex.next();
                auto r = parse_comparison();
                e = std::make_unique<EBinary>("and", std::move(e), std::move(r));
            }
            return e;
        }

        ExprPtr parse_comparison()
        {
            auto e = parse_add();
            if (cur.k == TokKind::BETWEEN)
            {
                cur = lex.next();
                auto lo = parse_add();
                if (cur.k == TokKind::AND)
                {
                    cur = lex.next();
                    auto hi = parse_add();
                    return std::make_unique<EBetween>(std::move(e), std::move(lo), std::move(hi));
                }
                else
                {
                    ok = false;
                    err = "expected AND in between";
                }
            }
            if (cur.k == TokKind::IN)
            {
                cur = lex.next();
                if (!expect(TokKind::LPAREN)) { return e;
}
                std::vector<ExprPtr> items;
                if (cur.k != TokKind::RPAREN)
                {
                    for (;;)
                    {
                        items.push_back(parse_or());
                        if (cur.k == TokKind::COMMA)
                        {
                            cur = lex.next();
                            continue;
                        }
                        break;
                    }
                }
                expect(TokKind::RPAREN);
                return std::make_unique<EIn>(std::move(e), std::move(items));
            }
            if (cur.k == TokKind::EQ || cur.k == TokKind::NEQ || cur.k == TokKind::LT || cur.k == TokKind::LE || cur.k
                == TokKind::GT || cur.k == TokKind::GE)
            {
                std::string op = cur.text;
                cur = lex.next();
                auto r = parse_add();
                return std::make_unique<EBinary>(op, std::move(e), std::move(r));
            }
            return e;
        }

        ExprPtr parse_add()
        {
            auto e = parse_mul();
            while (cur.k == TokKind::PLUS || cur.k == TokKind::MINUS)
            {
                std::string op = cur.k == TokKind::PLUS ? "+" : "-";
                cur = lex.next();
                auto r = parse_mul();
                e = std::make_unique<EBinary>(op, std::move(e), std::move(r));
            }
            return e;
        }

        ExprPtr parse_mul()
        {
            auto e = parse_pow();
            while (cur.k == TokKind::STAR || cur.k == TokKind::SLASH)
            {
                std::string op = cur.k == TokKind::STAR ? "*" : "/";
                cur = lex.next();
                auto r = parse_pow();
                e = std::make_unique<EBinary>(op, std::move(e), std::move(r));
            }
            return e;
        }

        ExprPtr parse_pow()
        {
            auto e = parse_unary();
            if (cur.k == TokKind::POW)
            {
                cur = lex.next();
                auto rhs = parse_pow();
                e = std::make_unique<EBinary>("**", std::move(e), std::move(rhs));
            }
            return e;
        }

        ExprPtr parse_unary()
        {
            if (cur.k == TokKind::NOT)
            {
                cur = lex.next();
                auto r = parse_unary();
                return std::make_unique<EUnary>("not", std::move(r));
            }
            if (cur.k == TokKind::PLUS)
            {
                cur = lex.next();
                return parse_unary();
            }
            if (cur.k == TokKind::MINUS)
            {
                cur = lex.next();
                auto r = parse_unary();
                return std::make_unique<EUnary>("neg", std::move(r));
            }
            return parse_primary();
        }

        // Helper: Parse name expression (identifier, boolean literal, or function call)
        ExprPtr parse_name_expr()
        {
            std::string name = cur.text;
            while (!name.empty() && name.front() == ' ') { name.erase(name.begin()); }
            cur = lex.next();
            
            std::string low;
            low.reserve(name.size());
            for (char ch : name) { low.push_back((char)std::tolower((unsigned char)ch)); }
            
            if (low == "true") { return std::make_unique<EBool>(true); }
            if (low == "false") { return std::make_unique<EBool>(false); }
            
            if (cur.k == TokKind::LPAREN)
            {
                cur = lex.next();
                std::vector<ExprPtr> args;
                if (cur.k != TokKind::RPAREN)
                {
                    for (;;)
                    {
                        args.push_back(parse_or());
                        if (cur.k == TokKind::COMMA)
                        {
                            cur = lex.next();
                            continue;
                        }
                        break;
                    }
                }
                expect(TokKind::RPAREN);
                return std::make_unique<ECall>(name, std::move(args));
            }
            return std::make_unique<EName>(name);
        }

        // Helper: Parse list or range expression
        ExprPtr parse_list_or_range()
        {
            bool inc_start = true;
            cur = lex.next();
            if (cur.k == TokKind::RBRACK)
            {
                cur = lex.next();
                return std::make_unique<EList>(std::vector<ExprPtr>{});
            }
            auto first = parse_or();
            if (cur.k == TokKind::DOTDOT)
            {
                cur = lex.next();
                auto second = parse_or();
                bool inc_end = false;
                if (cur.k == TokKind::RBRACK)
                {
                    inc_end = true;
                    cur = lex.next();
                }
                else { expect(TokKind::RPAREN); }
                return std::make_unique<ERange>(std::move(first), std::move(second), inc_start, inc_end);
            }
            std::vector<ExprPtr> items;
            items.push_back(std::move(first));
            while (cur.k == TokKind::COMMA)
            {
                cur = lex.next();
                items.push_back(parse_or());
            }
            expect(TokKind::RBRACK);
            return std::make_unique<EList>(std::move(items));
        }

        ExprPtr parse_primary()
        {
            if (cur.k == TokKind::NUMBER)
            {
                auto v = cur.num;
                cur = lex.next();
                return std::make_unique<ENum>(v);
            }
            if (cur.k == TokKind::STRING)
            {
                auto s = cur.text;
                cur = lex.next();
                return std::make_unique<EStr>(std::move(s));
            }
            if (cur.k == TokKind::NAME)
            {
                return parse_name_expr();
            }
            if (cur.k == TokKind::LPAREN)
            {
                cur = lex.next();
                auto e = parse_or();
                expect(TokKind::RPAREN);
                return e;
            }
            if (cur.k == TokKind::LBRACK)
            {
                return parse_list_or_range();
            }
            ok = false;
            err = "unexpected token";
            return std::make_unique<ENull>();
        }
    };

    // ---------------- Evaluation helpers ----------------
    static Value make_null() { return Value(); }

    static Value add_num(const Value& a, const Value& b)
    {
        if (a.is_num() && b.is_num()) { return Value(a.num() + b.num());
}
        if (a.is_str() && b.is_str()) { return Value(a.str() + b.str());
}
        if (a.is_str() && b.is_num()) { return Value(a.str() + std::to_string((double)b.num())); }
        if (a.is_num() && b.is_str()) { return Value(std::to_string((double)a.num()) + b.str()); }
        return make_null();
    }

    static Value sub_num(const Value& a, const Value& b)
    {
        if (a.is_num() && b.is_num()) { return Value(a.num() - b.num());
}
        return make_null();
    }

    static Value mul_num(const Value& a, const Value& b)
    {
        if (a.is_num() && b.is_num()) { return Value(a.num() * b.num());
}
        return make_null();
    }

    static Value div_num(const Value& a, const Value& b)
    {
        if (a.is_num() && b.is_num())
        {
            if (b.num() == 0) { return make_null();
}
            return Value(a.num() / b.num());
        }
        return make_null();
    }

    static Value pow_num(const Value& a, const Value& b)
    {
        if (a.is_num() && b.is_num()) { return Value((Decimal)std::pow((long double)a.num(), (long double)b.num()));
}
        return make_null();
    }

    // Three-valued logic: null propagates unless determinable.
    static Value logic_and(const Value& a, const Value& b)
    {
        if (a.is_bool() && b.is_bool()) { return Value(a.bval() && b.bval());
}
        if (a.is_bool() && !a.bval()) { return Value(false);
}
        if (b.is_bool() && !b.bval()) { return Value(false);
}
        if (a.is_null() || b.is_null()) { return make_null();
}
        return make_null();
    }

    static Value logic_or(const Value& a, const Value& b)
    {
        if (a.is_bool() && b.is_bool()) { return Value(a.bval() || b.bval());
}
        if (a.is_bool() && a.bval()) { return Value(true);
}
        if (b.is_bool() && b.bval()) { return Value(true);
}
        if (a.is_null() || b.is_null()) { return make_null();
}
        return make_null();
    }

    static Value logic_not(const Value& v)
    {
        if (v.is_bool()) { return Value(!v.bval());
}
        if (v.is_null()) { return make_null();
}
        return make_null();
    }

    static int cmp_values(const Value& a, const Value& b)
    {
        if (a.is_num() && b.is_num())
        {
            if (a.num() < b.num()) { return -1;
}
            if (a.num() > b.num()) { return 1;
}
            return 0;
        }
        if (a.is_str() && b.is_str())
        {
            if (a.str() < b.str()) { return -1;
}
            if (a.str() > b.str()) { return 1;
}
            return 0;
        }
        return std::numeric_limits<int>::max();
    }

    static Value cmp_op(const std::string& op, const Value& a, const Value& b)
    {
        int c = cmp_values(a, b);
        if (c == std::numeric_limits<int>::max()) { return make_null();
}
        if (op == "=") { return Value(c == 0);
}
        if (op == "!=") { return Value(c != 0);
}
        if (op == "<") { return Value(c < 0);
}
        if (op == "<=") { return Value(c <= 0);
}
        if (op == ">") { return Value(c > 0);
}
        if (op == ">=") { return Value(c >= 0);
}
        return make_null();
    }

    static Value eval_expr(const Expr* e, const json& ctx);
    static Value eval_single_arg_math(const std::string& func_name, const std::vector<Value>& args);
    static Value eval_two_arg_math(const std::string& func_name, const std::vector<Value>& args);
    static Value eval_all_function(const std::vector<Value>& values);
    static Value eval_any_function(const std::vector<Value>& values);
    static Value eval_boolean_string_functions(const std::string& func_name, const std::vector<Value>& args);

    // Helper: Trim leading and trailing whitespace from string
    static std::string trim_whitespace(const std::string& str)
    {
        std::string trimmed = str;
        while (!trimmed.empty() && trimmed.front() == ' ') {
            trimmed.erase(trimmed.begin());
        }
        while (!trimmed.empty() && trimmed.back() == ' ') {
            trimmed.pop_back();
        }
        return trimmed;
    }

    // Helper: Convert JSON value to FEEL Value type
    static Value json_to_value(const json& j)
    {
        if (j.is_boolean()) {
            return Value(j.get<bool>());
        }
        if (j.is_number()) {
            return Value(static_cast<Decimal>(j.get<double>()));
        }
        if (j.is_string()) {
            return Value(j.get<std::string>());
        }
        return make_null();
    }

    // Helper: Navigate to property in JSON object, handling dots
    static const json* navigate_json_path(const json& root, const std::string& path)
    {
        const json* node = &root;
        size_t start = 0;
        
        while (start < path.size())
        {
            size_t dot = path.find('.', start);
            std::string part = path.substr(start, 
                dot == std::string::npos ? std::string::npos : dot - start);
            
            if (!node->is_object()) {
                return nullptr;
            }
            
            auto it = node->find(part);
            if (it == node->end()) {
                return nullptr;
            }
            
            // If this is the final part, return it
            if (dot == std::string::npos) {
                return &*it;
            }
            
            // Otherwise, continue navigating
            node = &*it;
            start = dot + 1;
        }
        
        return nullptr;
    }

    // Resolve variable name in context (handles property access with dots)
    static Value resolve_name(const std::string& name, const json& ctx)
    {
        std::string trimmed = trim_whitespace(name);
        const json* result = navigate_json_path(ctx, trimmed);
        
        if (result != nullptr) {
            return json_to_value(*result);
        }
        
        return make_null();
    }

static Value eval_range(const ERange* r, const json& ctx)
{
    Value a = eval_expr(r->a.get(), ctx);
    Value b = eval_expr(r->b.get(), ctx);
    if (a.is_num() && b.is_num())
    {
        Range rng{a.num(), b.num(), r->incA, r->incB};
        return Value(rng);
    }
    return make_null();
}   static Value eval_in(const EIn* in, const json& ctx)
    {
        Value item = eval_expr(in->item.get(), ctx);
        if (item.is_null()) { return make_null();
}
        bool any = false;
        for (auto& ep : in->list)
        {
            Value v = eval_expr(ep.get(), ctx);
            if (item.is_num() && v.is_num())
            {
                if (item.num() == v.num())
                {
                    any = true;
                    break;
                }
            }
            else if (item.is_str() && v.is_str())
            {
                if (item.str() == v.str())
                {
                    any = true;
                    break;
                }
            }
            else if (v.is_range() && item.is_num())
            {
                auto& r = v.range();
                bool left = r.inc_start ? item.num() >= r.start : item.num() > r.start;
                bool right = r.inc_end ? item.num() <= r.end : item.num() < r.end;
                if (left && right)
                {
                    any = true;
                    break;
                }
            }
        }
        return Value(any);
    }

    static Value eval_between(const EBetween* be, const json& ctx)
    {
        Value val = eval_expr(be->val.get(), ctx);
        Value lo = eval_expr(be->lo.get(), ctx);
        Value hi = eval_expr(be->hi.get(), ctx);
        if (val.is_num() && lo.is_num() && hi.is_num()) { return Value(val.num() >= lo.num() && val.num() <= hi.num());
}
        return make_null();
    }

    static Value eval_call(const ECall* call, const json& ctx)
    {
        std::string low;
        for (char c : call->name) { low.push_back((char)std::tolower((unsigned char)c));
}
        std::vector<Value> args;
        args.reserve(call->args.size());
        for (auto& a : call->args) { args.push_back(eval_expr(a.get(), ctx));
}
        // Boolean/string functions (matches, all, any)
        if (low == "matches" || low == "all" || low == "any")
        {
            return eval_boolean_string_functions(low, args);
        }
        
        // Math functions (single-argument)
        if (low == "abs" || low == "sqrt" || low == "floor" || 
            low == "ceiling" || low == "exp" || low == "log")
        {
            return eval_single_arg_math(low, args);
        }
        
        // Two-argument math functions (modulo and rounding)
        if (low == "modulo" || low == "decimal" || low == "round" || low == "round up" || 
            low == "round down" || low == "round half up" || low == "round half down")
        {
            return eval_two_arg_math(low, args);
        }
        
        return make_null();
    }

    // Helper: Evaluate 'all' function logic
    static Value eval_all_function(const std::vector<Value>& values)
    {
        bool any_null = false;
        bool all_true = true;
        bool any_false = false;
        for (auto& v : values)
        {
            if (v.is_null()) {
                any_null = true;
            } else if (v.is_bool()) {
                if (!v.bval()) {
                    all_true = false;
                    any_false = true;
                }
            } else {
                any_null = true;
            }
        }
        if (any_null && !any_false) { return make_null(); }
        return Value(all_true);
    }

    // Helper: Evaluate 'any' function logic
    static Value eval_any_function(const std::vector<Value>& values)
    {
        bool any_null = false;
        bool any_true = false;
        for (auto& v : values)
        {
            if (v.is_null()) {
                any_null = true;
            } else if (v.is_bool() && v.bval()) {
                any_true = true;
            }
        }
        if (any_true) { return Value(true); }
        if (any_null) { return make_null(); }
        return Value(false);
    }

    // Helper: Evaluate boolean/string functions (matches, all, any)
    static Value eval_boolean_string_functions(const std::string& func_name, const std::vector<Value>& args)
    {
        if (func_name == "matches")
        {
            if (args.size() == 2 && args[0].is_str() && args[1].is_str())
            {
                try
                {
                    std::regex re(args[1].str(), std::regex::ECMAScript);
                    bool m = std::regex_match(args[0].str(), re);
                    return Value(m);
                }
                catch (...) { return make_null(); }
        }
        return make_null();
    }
    
    if (func_name == "all")
    {
        if (args.size() == 1 && args[0].is_list()) {
            return eval_all_function(args[0].list());
        }
        return eval_all_function(args);
    }
    
    if (func_name == "any")
    {
        if (args.size() == 1 && args[0].is_list()) {
            return eval_any_function(args[0].list());
        }
        return eval_any_function(args);
    }
    
    return make_null();  // Unknown function
}   // Helper: Evaluate two-argument math functions (rounding/scaling/modulo functions)
    static Value eval_two_arg_math(const std::string& func_name, const std::vector<Value>& args)
    {
        // Common validation: two numeric arguments
        if (args.size() != 2) { return make_null(); }
        if (args[0].is_null() || args[1].is_null()) { return make_null(); }
        if (!args[0].is_num() || !args[1].is_num()) { return make_null(); }
        
        auto value = static_cast<double>(args[0].num());  // Explicit cast to silence narrowing warning
        
        // Modulo function (different logic)
        if (func_name == "modulo") {
            auto divisor = static_cast<double>(args[1].num());  // Explicit cast
            if (divisor == 0.0) { return make_null(); }
            // DMN spec: modulo(dividend, divisor) = dividend - divisor * floor(dividend / divisor)
            double result = value - divisor * std::floor(value / divisor);
            return Value(result);
        }
        
    // Rounding functions (common pattern)
    int scale = static_cast<int>(args[1].num());
    double multiplier = std::pow(10.0, scale);
    double scaled = value * multiplier;
    double rounded = 0.0;  // Initialize to avoid uninitialized variable warning        // Function-specific rounding logic
        if (func_name == "decimal" || func_name == "round") {
            rounded = std::nearbyint(scaled);  // half-even rounding
        }
        else if (func_name == "round up") {
            rounded = (value >= 0.0) ? std::ceil(scaled) : std::floor(scaled);
        }
        else if (func_name == "round down") {
            rounded = (value >= 0.0) ? std::floor(scaled) : std::ceil(scaled);
        }
        else if (func_name == "round half up") {
            rounded = (value >= 0.0) ? std::floor(scaled + 0.5) : std::ceil(scaled - 0.5);
        }
        else if (func_name == "round half down") {
            rounded = (value >= 0.0) ? std::ceil(scaled - 0.5) : std::floor(scaled + 0.5);
        }
        else {
            return make_null();  // Unknown function
        }
        
        return Value(rounded / multiplier);
    }

    // Helper: Evaluate single-argument math functions
    static Value eval_single_arg_math(const std::string& func_name, const std::vector<Value>& args)
    {
        // Validate arguments
        if (args.size() != 1) {
            return make_null();
        }
        if (args[0].is_null()) {
            return make_null();
        }
        if (!args[0].is_num()) {
            return make_null();
        }

        auto val = static_cast<double>(args[0].num());  // Explicit cast to silence narrowing warning

        // Dispatch to specific math function
        if (func_name == "abs") {
            return Value(std::abs(val));
        }
        if (func_name == "sqrt") {
            if (val < 0.0) {
                return make_null();
            }
            return Value(std::sqrt(val));
        }
        if (func_name == "floor") {
            return Value(std::floor(val));
        }
        if (func_name == "ceiling") {
            return Value(std::ceil(val));
        }
        if (func_name == "exp") {
            return Value(std::exp(val));
        }
        if (func_name == "log") {
            if (val <= 0.0) {
                return make_null();
            }
            return Value(std::log(val));
        }

        // Unknown function
        return make_null();
    }

    // Helper: Evaluate unary expression (negation, logical not)
    static Value eval_unary_expr(const EUnary* unary, const json& ctx)
    {
        Value result = eval_expr(unary->rhs.get(), ctx);
        
        if (unary->op == "neg")
        {
            if (result.is_num()) {
                return Value(-result.num());
            }
            return make_null();
        }
        
        if (unary->op == "not") {
            return logic_not(result);
        }
        
        return make_null();
    }

    // Helper: Evaluate binary expression (arithmetic, logical, comparison)
    static Value eval_binary_expr(const EBinary* binary, const json& ctx)
    {
        Value left = eval_expr(binary->lhs.get(), ctx);
        Value right = eval_expr(binary->rhs.get(), ctx);
        
        // Arithmetic operators
        if (binary->op == "+") { return add_num(left, right); }
        if (binary->op == "-") { return sub_num(left, right); }
        if (binary->op == "*") { return mul_num(left, right); }
        if (binary->op == "/") { return div_num(left, right); }
        if (binary->op == "**") { return pow_num(left, right); }
        
        // Logical operators
        if (binary->op == "and") { return logic_and(left, right); }
        if (binary->op == "or") { return logic_or(left, right); }
        
        // Comparison operators (fallback)
        return cmp_op(binary->op, left, right);
    }

    // Helper: Evaluate list expression
    static Value eval_list_expr(const EList* list, const json& ctx)
    {
        Value::List result;
        for (const auto& item : list->items) {
            result.push_back(eval_expr(item.get(), ctx));
        }
        return Value(std::move(result));
    }

    // Main expression evaluator (dispatcher to type-specific handlers)
    static Value eval_expr(const Expr* e, const json& ctx)
    {
        // Null literal
        if (dynamic_cast<const ENull*>(e) != nullptr) {  // Explicit pointer comparison
            return make_null();
        }   // Boolean literal
    if (const auto* expr = dynamic_cast<const EBool*>(e)) {  // Add const auto* for pointer type
        return Value(expr->v);
    }
    
    // Number literal
    if (const auto* expr = dynamic_cast<const ENum*>(e)) {  // Add const auto* for pointer type
        return Value(expr->v);
    }
    
    // String literal
    if (const auto* expr = dynamic_cast<const EStr*>(e)) {  // Add const auto* for pointer type
        return Value(expr->v);
    }
    
    // Variable name
    if (const auto* expr = dynamic_cast<const EName*>(e)) {  // Add const auto* for pointer type
        return resolve_name(expr->name, ctx);
    }
    
    // Unary expression
    if (const auto* expr = dynamic_cast<const EUnary*>(e)) {  // Add const auto* for pointer type
        return eval_unary_expr(expr, ctx);
    }
    
    // Binary expression
    if (const auto* expr = dynamic_cast<const EBinary*>(e)) {  // Add const auto* for pointer type
        return eval_binary_expr(expr, ctx);
    }
    
    // In expression
    if (const auto* expr = dynamic_cast<const EIn*>(e)) {  // Add const auto* for pointer type
        return eval_in(expr, ctx);
    }
    
    // Range expression
    if (const auto* expr = dynamic_cast<const ERange*>(e)) {  // Add const auto* for pointer type
        return eval_range(expr, ctx);
    }
    
    // Between expression
    if (const auto* expr = dynamic_cast<const EBetween*>(e)) {  // Add const auto* for pointer type
        return eval_between(expr, ctx);
    }
    
    // List expression
    if (const auto* expr = dynamic_cast<const EList*>(e)) {  // Add const auto* for pointer type
        return eval_list_expr(expr, ctx);
    }
    
    // Function call
    if (const auto* expr = dynamic_cast<const ECall*>(e)) {  // Add const auto* for pointer type
        return eval_call(expr, ctx);
    }       return make_null();
    }

} // end anonymous namespace

    bool eval_feel_literal(std::string_view expr, const json& ctx, json& out, std::string& err)
    {
        try {
            // Use main parser for consistent parsing behavior
            out = orion::bre::feel::Parser::eval_expression(expr, ctx);
            return true;
        }
        catch (const std::exception& e) {
            err = e.what();
            return false;
        }
    }
}
