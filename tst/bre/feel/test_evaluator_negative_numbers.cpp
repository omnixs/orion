#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>

using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(feel_negative_number_debug)

BOOST_AUTO_TEST_CASE(test_negative_number_parsing_issue) {
    orion::bre::feel::Evaluator evaluator;
    json context = {};
    
    // Test the exact failing expressions from 0105-feel-math
    BOOST_TEST_MESSAGE("\n=== DEBUGGING NEGATIVE NUMBER PARSING ===");
    
    struct TestCase {
        std::string expression;
        double expected;
        std::string description;
    };
    
    std::vector<TestCase> test_cases = {
        {"10 + 20 / -5 - 3", 3.0, "Operator precedence with negative number"},
        {"10+20/-5-3", 3.0, "No spaces - negative number parsing"},
        {"10 + 20/-5 - 3", 3.0, "Mixed spacing - negative number parsing"},
        {"20 / -5", -4.0, "Simple division by negative"},
        {"-5", -5.0, "Simple negative number"},
        {"10 + (-4) - 3", 3.0, "Parenthesized negative"},
        {"10 + (20 / -5) - 3", 3.0, "Fully parenthesized"}
    };
    
    for (const auto& test_case : test_cases) {
        BOOST_TEST_MESSAGE("Testing: " << test_case.expression);
        json result = evaluator.evaluate(test_case.expression, context);
        
        BOOST_TEST_MESSAGE("  Result: " << result);
        BOOST_TEST_MESSAGE("  Expected: " << test_case.expected);
        
        if (result.is_null()) {
            BOOST_TEST_MESSAGE("  ❌ ISSUE: Expression returns null instead of number");
        } else if (result.is_number()) {
            double actual = result.get<double>();
            if (std::abs(actual - test_case.expected) < 0.001) {
                BOOST_TEST_MESSAGE("  ✅ PASS");
            } else {
                BOOST_TEST_MESSAGE("  ❌ FAIL: Got " << actual << ", expected " << test_case.expected);
            }
        } else {
            BOOST_TEST_MESSAGE("  ❌ ISSUE: Expression returns non-numeric result");
        }
        BOOST_TEST_MESSAGE("---");
    }
}

BOOST_AUTO_TEST_SUITE_END()