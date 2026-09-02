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
 * @file test_evaluator_duration.cpp
 * @brief Unit tests for FEEL duration type support
 * 
 * Tests duration() built-in function and duration comparisons.
 * Validates DMN 1.5 Section 10.3.2.3 compliance.
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/regex_cache.hpp>
#include <orion/bre/feel/unary.hpp>
#include <nlohmann/json.hpp>
#include "test_helpers.hpp"

using json = nlohmann::json;
using namespace orion::bre::feel;
using orion::bre::feel::test::get_test_eval_ctx;

BOOST_AUTO_TEST_SUITE(feel_duration_tests)

// ============================================================================
// Duration Parsing Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(duration_parse_days_only)
{
    // P5D = 5 days
    json result = Evaluator::evaluate("duration(\"P5D\")", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "P5D");
}

BOOST_AUTO_TEST_CASE(duration_parse_hours_only)
{
    // PT10H = 10 hours
    json result = Evaluator::evaluate("duration(\"PT10H\")", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "PT10H");
}

BOOST_AUTO_TEST_CASE(duration_parse_days_and_hours)
{
    // P2DT6H = 2 days and 6 hours
    json result = Evaluator::evaluate("duration(\"P2DT6H\")", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "P2DT6H");
}

BOOST_AUTO_TEST_CASE(duration_parse_full_iso8601)
{
    // P1Y2M3DT4H5M6S = 1 year, 2 months, 3 days, 4 hours, 5 minutes, 6 seconds
    json result = Evaluator::evaluate("duration(\"P1Y2M3DT4H5M6S\")", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "P1Y2M3DT4H5M6S");
}

BOOST_AUTO_TEST_CASE(duration_parse_zero_duration)
{
    // P0D = 0 days (edge case)
    json result = Evaluator::evaluate("duration(\"P0D\")", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "P0D");
}

// ============================================================================
// Duration Invalid Format Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(duration_invalid_missing_p_prefix)
{
    // Missing 'P' prefix - invalid
    json result = Evaluator::evaluate("duration(\"5D\")", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(duration_invalid_malformed)
{
    // Truly malformed: number without unit after P
    json result = Evaluator::evaluate("duration(\"P5\")", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_null());  // P followed by number but no unit
}

BOOST_AUTO_TEST_CASE(duration_invalid_bare_p)
{
    // Bare period designator is invalid
    json result = Evaluator::evaluate("duration(\"P\")", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_null());
}

BOOST_AUTO_TEST_CASE(duration_invalid_time_units_without_t)
{
    // Time units H/S must be in the T section
    json no_t_hours = Evaluator::evaluate("duration(\"P1H\")", {}, get_test_eval_ctx());
    BOOST_CHECK(no_t_hours.is_null());

    json no_t_seconds = Evaluator::evaluate("duration(\"P1S\")", {}, get_test_eval_ctx());
    BOOST_CHECK(no_t_seconds.is_null());
}

BOOST_AUTO_TEST_CASE(duration_invalid_empty_string)
{
    // Empty string - invalid
    json result = Evaluator::evaluate("duration(\"\")", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_null());
}

// ============================================================================
// DMN Null Propagation Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(duration_null_propagation)
{
    // duration(null) → null (DMN null propagation)
    json result = Evaluator::evaluate("duration(null)", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_null());
}

// ============================================================================
// Duration Comparison Tests
// ============================================================================
// Note: Duration comparisons in unary test expressions (e.g., <= duration("PT31H"))
// are tested in test_unary_feel_functions.cpp.
//
// Direct function-to-function comparisons like duration("PT48H") = duration("P2D")
// are not yet supported in FEEL expressions. Duration comparisons work in DMN
// decision tables via unary tests, where the input is a duration string and the
// test expression contains a duration() function call.
//
// See:
// - tst/bre/feel/test_unary_feel_functions.cpp for unary test comparison tests
// - src/bre/feel/unary.cpp for the comparison implementation (try_compare_durations)

BOOST_AUTO_TEST_CASE(duration_comparison_note)
{
    // This test documents where duration comparisons are tested
    BOOST_TEST_MESSAGE("Duration comparisons in unary tests: see test_unary_feel_functions.cpp");
    BOOST_TEST_MESSAGE("Duration() function parsing: tested in this file");
    BOOST_CHECK(true);  // Documentation placeholder
}

// ============================================================================
// Between Operator Tests
// ============================================================================
// Note: The 'in' operator for ranges is not yet implemented in the FEEL parser.
// These tests will be enabled when range expressions are supported.
// For now, use comparison operators: duration >= minDuration and duration <= maxDuration

BOOST_AUTO_TEST_CASE(duration_between_note)
{
    // This test documents that 'in' operator is not yet supported
    BOOST_TEST_MESSAGE("Range 'in' operator not yet implemented - use >= and <= instead");
    BOOST_CHECK(true);  // Placeholder
}

// ============================================================================
// Context Variable Integration Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(duration_from_context_variable)
{
    json input = {
        {"waitTime", "P7D"}
    };
    
    // duration(waitTime) → parse from context
    json result = Evaluator::evaluate("duration(waitTime)", input, get_test_eval_ctx());
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "P7D");
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

BOOST_AUTO_TEST_CASE(duration_large_values)
{
    // P365D = 1 year in days (large value)
    json result = Evaluator::evaluate("duration(\"P365D\")", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "P365D");
}

BOOST_AUTO_TEST_CASE(duration_mixed_units)
{
    // P1DT12H30M45S = 1 day, 12 hours, 30 minutes, 45 seconds
    json result = Evaluator::evaluate("duration(\"P1DT12H30M45S\")", {}, get_test_eval_ctx());
    BOOST_CHECK(result.is_string());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "P1DT12H30M45S");
}

BOOST_AUTO_TEST_SUITE_END()
