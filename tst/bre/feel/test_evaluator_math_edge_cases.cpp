#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace orion::bre;

BOOST_AUTO_TEST_SUITE(math_edge_cases_debug)

BOOST_AUTO_TEST_CASE(test_complex_math_expressions) {
    orion::bre::feel::Evaluator evaluator;
    json context = json::object();

    BOOST_TEST_MESSAGE("Testing complex math expressions from 0105-feel-math that likely fail:");
    
    auto test_expr = [&](const std::string& expr, double expected, const std::string& description) {
        try {
            json result = evaluator.evaluate(expr, context);
            BOOST_TEST_MESSAGE("Expression: " << expr << " (" << description << ")");
            BOOST_TEST_MESSAGE("Result: " << result.dump() << " (expected: " << expected << ")");
            if (result.is_number()) {
                double actual = result.get<double>();
                if (std::abs(actual - expected) < 0.01) {
                    BOOST_TEST_MESSAGE("✅ PASS");
                } else {
                    BOOST_TEST_MESSAGE("❌ FAIL - got " << actual << ", expected " << expected);
                }
            } else {
                BOOST_TEST_MESSAGE("❌ FAIL - not a number");
            }
            BOOST_TEST_MESSAGE("---");
        } catch (const std::exception& e) {
            BOOST_TEST_MESSAGE("Expression: " << expr << " (" << description << ")");
            BOOST_TEST_MESSAGE("❌ ERROR: " << e.what());
            BOOST_TEST_MESSAGE("---");
        }
    };

    // Test complex expressions that might be failing
    test_expr("10 + 20 / -5 - 3", 3.0, "Operator precedence with negative");
    test_expr("10 + 20 / (-5 - 3)", 7.5, "Parentheses with arithmetic");
    test_expr("1.2*10**3", 1200.0, "Decimal with exponentiation");
    
    // Also test spacing variations that might cause issues
    test_expr("10+20/-5-3", 3.0, "No spaces");
    test_expr("10 + 20/-5 - 3", 3.0, "Mixed spacing");
}

BOOST_AUTO_TEST_CASE(test_null_arithmetic_edge_cases) {
    orion::bre::feel::Evaluator evaluator;
    json context = json::object();
    
    BOOST_TEST_MESSAGE("Testing null arithmetic edge cases:");
    
    auto test_null_expr = [&](const std::string& expr, const std::string& description) {
        try {
            json result = evaluator.evaluate(expr, context);
            BOOST_TEST_MESSAGE("Expression: " << expr << " (" << description << ")");
            BOOST_TEST_MESSAGE("Result: " << result.dump());
            if (result.is_null()) {
                BOOST_TEST_MESSAGE("✅ PASS - correctly returns null");
            } else {
                BOOST_TEST_MESSAGE("❌ FAIL - should return null");
            }
            BOOST_TEST_MESSAGE("---");
        } catch (const std::exception& e) {
            BOOST_TEST_MESSAGE("Expression: " << expr << " (" << description << ")");
            BOOST_TEST_MESSAGE("❌ ERROR: " << e.what());
            BOOST_TEST_MESSAGE("---");
        }
    };
    
    // Test null with different spacing
    test_null_expr("10 - null", "Subtraction with null");
    test_null_expr("null - 10", "Null minus number");
    test_null_expr("10 * null", "Multiplication with null");
    test_null_expr("null * 10", "Null times number");
    test_null_expr("10 / null", "Division by null");
    test_null_expr("null / 10", "Null divided by number");
}

BOOST_AUTO_TEST_SUITE_END()