/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/function_registry.hpp>

using namespace orion::bre::feel;

BOOST_AUTO_TEST_SUITE(function_registry)

BOOST_AUTO_TEST_CASE(test_singleton_instance) {
    auto& registry1 = FunctionRegistry::instance();
    auto& registry2 = FunctionRegistry::instance();
    
    // Should return same instance
    BOOST_TEST(&registry1 == &registry2);
}

BOOST_AUTO_TEST_CASE(test_get_abs_function) {
    auto sig = FunctionRegistry::instance().get_signature("abs");
    
    BOOST_TEST(sig.has_value());
    BOOST_TEST(sig->name == "abs");
    BOOST_TEST(sig->parameters.size() == 1);
    BOOST_TEST(sig->parameters[0].name == "n");
    BOOST_TEST(!sig->parameters[0].optional);
    BOOST_TEST(!sig->variadic);
}

BOOST_AUTO_TEST_CASE(test_get_sqrt_function) {
    auto sig = FunctionRegistry::instance().get_signature("sqrt");
    
    BOOST_TEST(sig.has_value());
    BOOST_TEST(sig->parameters.size() == 1);
    BOOST_TEST(sig->parameters[0].name == "number");
}

BOOST_AUTO_TEST_CASE(test_get_decimal_function) {
    auto sig = FunctionRegistry::instance().get_signature("decimal");
    
    BOOST_TEST(sig.has_value());
    BOOST_TEST(sig->parameters.size() == 2);
    BOOST_TEST(sig->parameters[0].name == "n");
    BOOST_TEST(sig->parameters[1].name == "scale");
    BOOST_TEST(!sig->parameters[0].optional);
    BOOST_TEST(!sig->parameters[1].optional);
}

BOOST_AUTO_TEST_CASE(test_get_modulo_function) {
    auto sig = FunctionRegistry::instance().get_signature("modulo");
    
    BOOST_TEST(sig.has_value());
    BOOST_TEST(sig->parameters.size() == 2);
    BOOST_TEST(sig->parameters[0].name == "dividend");
    BOOST_TEST(sig->parameters[1].name == "divisor");
}

BOOST_AUTO_TEST_CASE(test_get_round_functions) {
    auto round_sig = FunctionRegistry::instance().get_signature("round");
    BOOST_TEST(round_sig.has_value());
    BOOST_TEST(round_sig->parameters.size() == 2);
    
    auto round_up_sig = FunctionRegistry::instance().get_signature("round up");
    BOOST_TEST(round_up_sig.has_value());
    BOOST_TEST(round_up_sig->parameters[0].name == "n");
    BOOST_TEST(round_up_sig->parameters[1].name == "scale");
    
    auto round_down_sig = FunctionRegistry::instance().get_signature("round down");
    BOOST_TEST(round_down_sig.has_value());
    
    auto round_half_up_sig = FunctionRegistry::instance().get_signature("round half up");
    BOOST_TEST(round_half_up_sig.has_value());
    
    auto round_half_down_sig = FunctionRegistry::instance().get_signature("round half down");
    BOOST_TEST(round_half_down_sig.has_value());
}

BOOST_AUTO_TEST_CASE(test_get_substring_optional_parameter) {
    auto sig = FunctionRegistry::instance().get_signature("substring");
    
    BOOST_TEST(sig.has_value());
    BOOST_TEST(sig->parameters.size() == 3);
    BOOST_TEST(sig->parameters[0].name == "string");
    BOOST_TEST(!sig->parameters[0].optional);
    BOOST_TEST(sig->parameters[1].name == "start position");
    BOOST_TEST(!sig->parameters[1].optional);
    BOOST_TEST(sig->parameters[2].name == "length");
    BOOST_TEST(sig->parameters[2].optional);  // length is optional
}

BOOST_AUTO_TEST_CASE(test_get_variadic_append) {
    auto sig = FunctionRegistry::instance().get_signature("append");
    
    BOOST_TEST(sig.has_value());
    BOOST_TEST(sig->parameters.size() == 1);  // First parameter 'list'
    BOOST_TEST(sig->parameters[0].name == "list");
    BOOST_TEST(sig->variadic);  // Accepts additional items
}

BOOST_AUTO_TEST_CASE(test_get_boolean_functions) {
    auto not_sig = FunctionRegistry::instance().get_signature("not");
    BOOST_TEST(not_sig.has_value());
    BOOST_TEST(not_sig->parameters[0].name == "negand");
    
    auto all_sig = FunctionRegistry::instance().get_signature("all");
    BOOST_TEST(all_sig.has_value());
    BOOST_TEST(all_sig->parameters[0].name == "list");
    
    auto any_sig = FunctionRegistry::instance().get_signature("any");
    BOOST_TEST(any_sig.has_value());
}

BOOST_AUTO_TEST_CASE(test_get_unknown_function) {
    auto sig = FunctionRegistry::instance().get_signature("unknown_function");
    
    BOOST_TEST(!sig.has_value());
}

BOOST_AUTO_TEST_CASE(test_case_sensitive_lookup) {
    auto sig1 = FunctionRegistry::instance().get_signature("abs");
    auto sig2 = FunctionRegistry::instance().get_signature("ABS");
    
    BOOST_TEST(sig1.has_value());
    BOOST_TEST(!sig2.has_value());  // Should be case-sensitive
}

BOOST_AUTO_TEST_CASE(test_multi_word_function_names) {
    auto sig1 = FunctionRegistry::instance().get_signature("round up");
    BOOST_TEST(sig1.has_value());
    
    auto sig2 = FunctionRegistry::instance().get_signature("string length");
    BOOST_TEST(sig2.has_value());
    
    auto sig3 = FunctionRegistry::instance().get_signature("substring before");
    BOOST_TEST(sig3.has_value());
}

BOOST_AUTO_TEST_SUITE_END()
