# ORION Coding Standards

## Table of Contents
- [General Principles](#general-principles)
- [C++ Language Standards](#cpp-language-standards)
- [DMN 1.5 Compliance](#dmn-15-compliance)
- [Error Handling](#error-handling)
- [Memory Management](#memory-management)
- [Documentation](#documentation)
- [Testing](#testing)
- [Performance](#performance)
- [Code Organization](#code-organization)
- [Business Rules Engine (BRE) Specific](#business-rules-engine-bre-specific)
- [Dependencies](#dependencies)
- [Code Review Process](#code-review-process)

## General Principles

### Core Values
- **DMN 1.5 Specification Compliance** - All decision logic must conform to OMG DMN 1.5 standard
- **Generic Solutions Only** - No hardcoded values, test-specific code, or assumptions about specific domains
- **Modern C++** - Use C++20/23 features, const-correct code, RAII principles
- **Production Quality** - Robust error handling, comprehensive testing, performance optimization

### Design Philosophy
```cpp
// ✅ GOOD: Generic, reusable, DMN-compliant
class BusinessRulesEngine {
    nlohmann::json evaluate(std::string_view dmn_xml, const nlohmann::json& context);
};

// ❌ BAD: Hardcoded, domain-specific, brittle
class LoanProcessor {
    double calculatePayment(double amount = 600000, double rate = 0.0375);
};
```

## C++ Language Standards

### Language Version
- **Target**: C++23 where available, C++20 minimum
- **Compiler**: MSVC 2022, GCC 11+, Clang 14+
- **Features**: Concepts, ranges, coroutines (where applicable)

### Naming Conventions (Enforced by clang-tidy)

The project uses `.clang-tidy` configuration to enforce consistent naming:

```yaml
# From .clang-tidy
CheckOptions:
  - key: readability-identifier-naming.ClassCase
    value: CamelCase              # Classes: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: lower_case             # Functions/methods: snake_case
  - key: readability-identifier-naming.VariableCase
    value: lower_case             # Variables: snake_case
```

**All code must follow these conventions** - they are automatically checked by clang-tidy.

### Code Style

**Naming Conventions** (enforced by `.clang-tidy`):

```cpp
// ✅ Classes: CamelCase
namespace orion::bre {
    class DecisionTable { };
    class BusinessKnowledgeModel { };
    class ContractViolation { };
}

// ✅ Functions and methods: lower_case (snake_case)
class BKMManager {
    std::map<std::string, std::unique_ptr<BusinessKnowledgeModel>> bkms_;
    
public:
    void add_bkm(std::unique_ptr<BusinessKnowledgeModel> bkm);
    [[nodiscard]] bool has_bkm(std::string_view bkm_name) const noexcept;
    [[nodiscard]] const BusinessKnowledgeModel* get_bkm(std::string_view bkm_name) const;
};

// ✅ Variables: lower_case (snake_case)
void process_decision(std::string_view dmn_xml) {
    std::string error_message;
    bool load_success = load_bkm_from_dmn(dmn_xml, error_message);
    const auto* bkm_instance = get_bkm("MyBKM");
}

// ✅ Private members: trailing underscore
class Engine {
private:
    std::string model_name_;
    std::map<std::string, Decision> decisions_;
    BKMManager bkm_manager_;
};

// ✅ Constants: ALL_CAPS
static constexpr int MAX_RULES = 1000;
static constexpr int MONTHS_PER_YEAR = 12;

// ✅ Use attributes
[[nodiscard]] bool is_valid() const noexcept;
[[nodiscard]] nlohmann::json evaluate(const nlohmann::json& context) const;
```

**Indentation:**
- **ALWAYS use spaces, NEVER use tabs**
- **Indent with 4 spaces per level**
- **Rationale**: Spaces ensure consistent rendering across all editors and tools
- **Enforcement**: All source files have been converted to use spaces

```cpp
// ✅ GOOD: 4 spaces per indentation level
class Example {
public:
    void method() {
        if (condition) {
            do_something();
        }
    }
};

// ❌ BAD: Using tabs (will cause issues with tooling)
class Example {
→   void method() {  // Tab character - DO NOT USE
→   →   do_something();
    }
};
```

**Modern C++ Features:**
```cpp
// ✅ Trailing return types for complex returns
auto parse_decision(std::string_view xml) -> std::optional<Decision>;

// ✅ Use std::string_view for non-owning string parameters
auto tokenize(std::string_view input) -> std::vector<std::string>;
bool is_valid_identifier(std::string_view name) noexcept;

// ✅ Structured bindings
for (const auto& [key, value] : context.items()) {
    process_value(value);
}

// ✅ Init statements in conditionals
if (auto result = evaluate_expression(expr); result.has_value()) {
    return *result;
}
```

### Const Correctness
```cpp
// ✅ GOOD: Const-correct design
class Evaluator {
public:
    [[nodiscard]] nlohmann::json evaluate(std::string_view expression, 
                                          const nlohmann::json& context) const;
private:
    mutable std::mutex cache_mutex_;  // Only mutable when necessary
};

// ✅ GOOD: Use string_view for read-only string parameters
class Parser {
public:
    [[nodiscard]] bool is_keyword(std::string_view token) const;
    [[nodiscard]] std::optional<double> parse_number(std::string_view str) const;
};

// ❌ BAD: Missing const qualifiers
class BadEvaluator {
    nlohmann::json evaluate(std::string expression, nlohmann::json context);  // Should be string_view, const ref, and const method
};
```

## DMN 1.5 Compliance

### Mandatory Requirements
- **All decision logic** must conform to DMN 1.5 specification
- **FEEL expressions** must use standard FEEL syntax and semantics
- **Hit policies** must implement all DMN-defined policies (FIRST, UNIQUE, PRIORITY, ANY, COLLECT, etc.)
- **Data types** must support DMN type system (number, string, boolean, date, duration, etc.)

### Implementation Standards
```cpp
// ✅ GOOD: DMN-compliant structure
struct DecisionTable {
    HitPolicy hit_policy{HitPolicy::FIRST};
    CollectAggregation aggregation{CollectAggregation::NONE};
    std::vector<InputClause> inputs;
    std::vector<OutputClause> outputs;
    std::vector<Rule> rules;
    
    [[nodiscard]] nlohmann::json evaluate(const nlohmann::json& context) const;
};

// ✅ GOOD: Standard FEEL evaluation
[[nodiscard]] nlohmann::json evaluate_feel_expression(std::string_view expression,
                                        const nlohmann::json& context);
```

## Error Handling

### Exception Hierarchy
```cpp
// ✅ Current exception types
namespace orion::bre {
    class ContractViolation : public std::logic_error {
        // Programming errors that should never happen in correct code
        // Includes source location (file, line, function) for debugging
    };
}

// Note: Business logic errors currently use std::runtime_error
// Plan: Migrate to std::expected<T, Error> for expected failures

// ✅ Usage with proper error context
void parse_business_knowledge_model(std::string_view xml) {
    if (xml.empty()) [[unlikely]] {
        throw std::runtime_error("DMN XML cannot be empty");
    }
    
    try {
        // Parsing logic
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse BKM: " + std::string(e.what()));
    }
}
```

### Contract Programming

**Purpose**: Use `ContractViolation` for programming errors ONLY (bugs in our code). Use `std::expected` for business logic errors (expected failures).

#### Programming Errors (Precondition Violations)
```cpp
// ✅ Use THROW_CONTRACT_VIOLATION for programming errors
#include <orion/bre/contract_violation.hpp>

void add_bkm(std::unique_ptr<BusinessKnowledgeModel> bkm) {
    // Precondition checks - these indicate bugs if violated
    if (bkm == nullptr) [[unlikely]] {
        THROW_CONTRACT_VIOLATION("BKM cannot be null");
    }
    if (bkm->name.empty()) [[unlikely]] {
        THROW_CONTRACT_VIOLATION("BKM name cannot be empty");
    }
    
    std::string name = bkm->name;
    bkms_[name] = std::move(bkm);
}
```

#### Business Logic Errors (Expected Failures)
```cpp
// ✅ Future: Use std::expected for business logic errors
// Note: Currently using std::runtime_error, planned migration to std::expected
#include <expected>

// Planned error types (not yet implemented):
enum class DmnError {
    BkmNotFound,
    DecisionNotFound,
    InvalidDocument
};

// Future implementation:
std::expected<const BusinessKnowledgeModel*, DmnError> 
get_bkm(std::string_view name) {
    // Precondition check (programming error)
    if (name.empty()) [[unlikely]] {
        THROW_CONTRACT_VIOLATION("BKM name cannot be empty");
    }
    
    // Business logic check (expected error)
    if (!has_bkm(name)) {
        return std::unexpected(DmnError::BkmNotFound);
    }
    
    return bkms_[std::string(name)].get();
}

// Current implementation uses:
const BusinessKnowledgeModel* get_bkm(std::string_view name) const {
    if (!has_bkm(name)) {
        return nullptr;  // Or throws std::runtime_error
    }
    auto it = bkms_.find(std::string(name));
    return it != bkms_.end() ? it->second.get() : nullptr;
}
```

#### When to Use Each
- **ContractViolation**: Null pointers, empty required strings, invalid state, out of bounds
  - These are bugs - should never happen in correct code
  - Throws exception with source location (file:line:function)
  
- **std::runtime_error** (current): Business logic errors like not found, parse errors, evaluation errors
  - Currently used for expected failures
  - Plan: Migrate to `std::expected<T, Error>` for better error handling
  
- **std::expected** (future): Expected failures that should be handled gracefully
  - Not found errors → return error value instead of exception
  - Parse errors → return error value for graceful recovery
  - Invalid input → return error value for user feedback

## Memory Management

### Smart Pointers
```cpp
// ✅ GOOD: Use smart pointers for ownership management
class BKMManager {
    std::map<std::string, std::unique_ptr<BusinessKnowledgeModel>> bkms_;
public:
    void add_bkm(std::unique_ptr<BusinessKnowledgeModel> bkm);
    const BusinessKnowledgeModel* get_bkm(std::string_view name) const;
};

// ❌ BAD: Raw pointers for ownership
class BadManager {
    std::map<std::string, BusinessKnowledgeModel*> bkms_;  // Memory leak risk
};
```

### Resource Management (RAII)
```cpp
// ✅ GOOD: RAII for automatic resource cleanup
class DMNParser {
    struct XmlDocumentDeleter {
        void operator()(rapidxml::xml_document<>* doc) const {
            delete doc;
        }
    };
    
    using XmlDocumentPtr = std::unique_ptr<rapidxml::xml_document<>, XmlDocumentDeleter>;
    
    [[nodiscard]] XmlDocumentPtr parse_xml(std::string_view xml) {
        auto doc = std::make_unique<rapidxml::xml_document<>>();
        // Parse and return - automatic cleanup on exception
        return doc;
    }
};
```

## Documentation

### API Documentation
```cpp
/**
 * @brief Evaluates a DMN decision table against input context
 * 
 * Implements DMN 1.5 specification for decision table evaluation with
 * support for all hit policies and data types.
 * 
 * @param context JSON object containing input data for evaluation
 * @return JSON result according to hit policy and aggregation rules
 * 
 * @throws std::runtime_error if evaluation fails due to invalid input
 * @throws ContractViolation if table structure is invalid (programming error)
 * 
 * @see DMN 1.5 Specification Section 8.1 "Decision Table Evaluation"
 * @see https://www.omg.org/spec/DMN/
 * 
 * @example
 * ```cpp
 * std::string_view dmn_xml = load_dmn_content();
 * DecisionTable table = parse_decision_table(dmn_xml);
 * nlohmann::json context = {{"Age", 25}, {"Income", 50000}};
 * nlohmann::json result = table.evaluate(context);
 * ```
 */
nlohmann::json DecisionTable::evaluate(const nlohmann::json& context) const;
```

### Code Comments
```cpp
// ✅ GOOD: Explain WHY, not WHAT
// DMN 1.5 requires case-insensitive property access for FEEL expressions
std::string lower_name = to_lower_case(property_name);  // property_name is string_view

// ✅ GOOD: Reference specifications
// According to DMN 1.5 Section 10.3.2.9, null values use three-valued logic
if (left_val.is_null() || right_val.is_null()) {
    return nlohmann::json{};  // null propagation
}

// ❌ BAD: Obvious comments
i++;  // Increment i
```

## Testing

### Unit Test Standards
```cpp
// ✅ GOOD: DMN specification-based testing
BOOST_AUTO_TEST_CASE(decision_table_unique_hit_policy) {
    // Arrange: DMN 1.5 compliant decision table
    DecisionTable table = create_unique_hit_policy_table();
    nlohmann::json context = {{"input1", "value1"}};
    
    // Act: Evaluate according to DMN specification
    nlohmann::json result = table.evaluate(context);
    
    // Assert: Verify DMN-compliant behavior
    BOOST_CHECK(result.is_object());
    BOOST_CHECK_EQUAL(result["output1"], "expected_value");
}

// ❌ BAD: Hardcoded test values in production code
bool is_loan_decision(std::string_view name) {
    return name == "MonthlyPayment";  // Don't hardcode test case names!
}
```

### Integration Testing
- **DMN TCK Compliance**: All tests must pass official DMN Test Compatibility Kit
- **Real DMN Models**: Test with actual DMN files, not synthetic data
- **Cross-platform**: Verify behavior on Windows, Linux, macOS

## Performance

### Optimization Guidelines
```cpp
// ✅ GOOD: Use string_view for read-only string operations
[[nodiscard]] bool is_operator(std::string_view token) noexcept {
    // No allocation, no copy - just a view into existing string
    return token == "+" || token == "-" || token == "*" || token == "/";
}

// ✅ GOOD: Efficient string parsing with string_view
[[nodiscard]] std::vector<std::string_view> tokenize(std::string_view input) {
    std::vector<std::string_view> tokens;
    size_t start = 0;
    while (start < input.size()) {
        size_t end = input.find(' ', start);
        if (end == std::string_view::npos) end = input.size();
        tokens.push_back(input.substr(start, end - start));
        start = end + 1;
    }
    return tokens;
}

// ✅ GOOD: Efficient JSON handling
void process_large_context(const nlohmann::json& context) {
    // Use const references to avoid copying
    for (const auto& [key, value] : context.items()) {
        if (value.is_object()) {
            process_nested_object(value);  // Pass by const reference
        }
    }
}

// ✅ GOOD: Caching for expensive operations
class FeelExpressionCache {
    mutable std::unordered_map<std::string, CompiledExpression> cache_;
    mutable std::shared_mutex cache_mutex_;
    
public:
    [[nodiscard]] nlohmann::json evaluate(std::string_view expr, const nlohmann::json& context) const {
        std::shared_lock lock(cache_mutex_);
        std::string key(expr);  // Convert to string for map lookup
        if (auto it = cache_.find(key); it != cache_.end()) {
            return it->second.evaluate(context);
        }
        // Compile and cache with key
    }
};
```

### Memory Efficiency
- **Avoid unnecessary copying** of JSON objects
- **Use std::string_view** for read-only string operations (no allocation, no copy)
  - Function parameters that don't need ownership: `void parse(std::string_view input)`
  - String comparisons and searches: `bool contains(std::string_view pattern)`
  - Tokenization and substring operations without copies
  - ⚠️ **Warning**: Never return `string_view` to local variables or temporaries
- **Reserve container capacity** when size is known
- **Prefer stack allocation** for small, short-lived objects

## Code Organization

### File Structure
```
orion/
├── include/orion/
│   ├── api/
│   │   ├── engine.hpp             # Main BRE interface
│   │   ├── logger.hpp             # Logger interface
│   │   └── dmn_enums.hpp          # DMN type definitions
│   ├── bre/
│   │   ├── ast_node.hpp           # FEEL AST definitions
│   │   ├── dmn_model.hpp          # Core DMN structures
│   │   ├── dmn_parser.hpp         # DMN XML parser
│   │   ├── contract_violation.hpp # Contract programming support
│   │   └── feel/                  # FEEL expression engine
│   │       ├── evaluator.hpp      # FEEL expression evaluation
│   │       ├── parser.hpp         # FEEL parser
│   │       └── expr.hpp           # FEEL expression types
│   └── common/
│       └── xml2json.hpp           # XML utilities
├── src/
│   ├── api/
│   │   └── engine.cpp             # BRE engine implementation
│   ├── bre/
│   │   ├── ast_node.cpp           # AST implementation
│   │   ├── dmn_parser.cpp         # DMN XML parsing
│   │   ├── dmn_model.cpp          # DMN model implementation
│   │   └── feel/                  # FEEL implementation
│   │       ├── evaluator.cpp      # FEEL evaluator
│   │       ├── expr.cpp           # FEEL expressions
│   │       └── parser.cpp         # FEEL parser
│   └── apps/
│       ├── orion_app.cpp          # CLI application
│       └── orion_tck_runner.cpp   # TCK test runner
└── tst/
    ├── bre/
    │   ├── test_main.cpp          # Test entry point
    │   ├── feel/                  # FEEL expression tests
    │   │   ├── test_parser.cpp
    │   │   └── test_evaluator_*.cpp
    │   └── tck/                   # TCK compliance tests
    │       └── test_tck_*.cpp
    └── common/
        └── test_xml2json.cpp      # Utility tests
```

### Namespace Organization
```cpp
// ✅ GOOD: Logical namespace hierarchy
namespace orion {
    namespace api {
        // Public API for consumers
        class BusinessRulesEngine;
        class ILogger;
    }
    
    namespace bre {
        // Core BRE functionality
        class DecisionTable;
        class DmnModel;
        
        namespace feel {
            // FEEL-specific functionality
            class Evaluator;
            class Parser;
        }
    }
    
    namespace common {
        // Common utilities
        [[nodiscard]] nlohmann::json xml_to_json(std::string_view xml);
    }
}
```

## Business Rules Engine (BRE) Specific

### ❌ FORBIDDEN: Hardcoded Test Values

Never include hardcoded values from test cases in production BRE code:

```cpp
// ❌ DON'T DO THIS
if (expression.find("PMT(") != string::npos)
if (context.contains("fee"))
double loan_amount = 600000;  
string decision_name = "MonthlyPayment";
constexpr double INTEREST_RATE = 0.0375;  // Test-specific value

// ✅ DO THIS INSTEAD  
if (available_bkms.count(func_name) > 0)
if (context.contains(variable_name))
double value = resolve_from_context(property_name, context);
string decision_name = parse_decision_name_from_xml(dmn_xml);
constexpr int MONTHS_PER_YEAR = 12;  // Generic constant
```

### Generic Patterns for BRE

1. **Function Detection**: Use `available_bkms.count(func_name)` instead of checking specific names
2. **Property Access**: Use `detail::resolve_argument()` for any property path  
3. **Arithmetic**: Use `detail::eval_math_expression()` for any mathematical expression
4. **Context Lookup**: Use generic key iteration instead of specific key names
5. **Debug Conditions**: Use pattern-based detection, not hardcoded names

```cpp
// ✅ GOOD: Generic debug condition
bool debug_output = (expression.find("(") != string::npos && 
                    context.is_object() && 
                    context.size() > 1);

// ❌ BAD: Hardcoded debug condition  
bool debug_output = (expression.find("PMT") != string::npos && 
                    context.contains("Loan"));
```

### BRE Code Review Checklist

- [ ] No function names from test cases (PMT, Payment, etc.)
- [ ] No property names from test cases (fee, amount, rate, term, etc.)  
- [ ] No numeric values from test cases (600000, 0.0375, 360, etc.)
- [ ] No string literals from test cases ("Hello World", "John Doe", etc.)
- [ ] Uses generic parsing and evaluation functions
- [ ] Works with any DMN model structure
- [ ] Proper DMN 1.5 compliance
- [ ] No magic numbers (use named constants)

## Dependencies

### Approved Libraries
- **nlohmann/json**: JSON parsing and manipulation
- **spdlog**: Logging framework
- **Boost.Test**: Unit testing framework
- **RapidXML**: XML parsing for DMN files

### Dependency Guidelines
```cpp
// ✅ GOOD: Minimal, focused includes
#include <nlohmann/json.hpp>
#include <orion/bre/contract_violation.hpp>

// ❌ BAD: Unnecessary dependencies
#include <boost/algorithm/string.hpp>  // Use std::string methods instead
```

### Adding New Dependencies
- **Justify necessity**: Explain why existing solutions are insufficient
- **Evaluate license compatibility**: Must be compatible with Apache 2.0
- **Consider maintenance burden**: Prefer well-maintained, stable libraries
- **Performance impact**: Benchmark against alternatives

## Code Review Process

### Pre-commit Checks
1. **Build successfully** on all target platforms
2. **All tests pass** including DMN TCK compliance tests
3. **clang-tidy passes** with no warnings (naming conventions, code quality)
4. **No hardcoded values** detected by scanner
5. **Documentation updated** for API changes
6. **Performance benchmarks** pass (if applicable)

### Review Checklist
- [ ] DMN 1.5 specification compliance
- [ ] No hardcoded test values or domain assumptions
- [ ] Proper error handling and contract validation
- [ ] Modern C++ best practices followed
- [ ] clang-tidy warnings addressed (naming, code quality)
- [ ] Comprehensive test coverage
- [ ] Clear, helpful documentation
- [ ] Performance considerations addressed

### Automation
```bash
# Run before committing
clang-tidy -p build src/**/*.cpp     # Check code quality and naming
.\scripts\scan-hardcoded-values.ps1  # Check for hardcoded values
cmake --build build --target test    # Run all tests
cmake --build build --target tst_bre # Run BRE-specific tests
```

**Using compile_commands.json for clang-tidy:**
```bash
# Generate compile_commands.json
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Run clang-tidy on all source files
clang-tidy -p build $(find src -name "*.cpp")

# Run clang-tidy on specific file
clang-tidy -p build src/bre/bkm_manager.cpp

# Fix issues automatically (use with caution)
clang-tidy -p build --fix-errors src/bre/bkm_manager.cpp
```

## Enforcement

- **clang-tidy**: Enforces naming conventions (CamelCase classes, snake_case functions/variables)
- **Automated scanning**: `.\scripts\scan-hardcoded-values.ps1` catches hardcoded values
- **CI/CD pipeline**: All checks must pass before merge (build, tests, clang-tidy, code quality)
- **Code review**: Mandatory review by BRE team member
- **Documentation**: API changes require documentation updates

---

*This document is a living standard - update it as the ORION project evolves while maintaining DMN 1.5 compliance and generic, reusable design principles.*