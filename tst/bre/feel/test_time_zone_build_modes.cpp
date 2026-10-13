/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

/**
 * @file test_time_zone_build_modes.cpp
 * @brief Behaviour of the library with and without the std::chrono time zone database
 *
 * The CMake option ORION_ENABLE_TZDB selects the variant; ORION_TZDB_ENABLED (exported by
 * orion_lib) tells which one this test binary links against. CI builds and runs both.
 */

#include <boost/test/unit_test.hpp>
#include <orion/api/engine.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/types.hpp>
#include <nlohmann/json.hpp>
#include "test_helpers.hpp"

#include <regex>
#include <string>

using orion::bre::feel::Evaluator;
using orion::bre::feel::test::get_test_eval_ctx;
using json = nlohmann::json;

namespace {
    json eval(const std::string& expression)
    {
        return Evaluator::evaluate(expression, json::object(), get_test_eval_ctx());
    }
}

BOOST_AUTO_TEST_SUITE(time_zone_build_modes)

// ---------- Behaviour that must be identical in both variants ----------

BOOST_AUTO_TEST_CASE(runtime_query_matches_build_variant)
{
    BOOST_TEST(orion::api::named_time_zones_supported() == (ORION_TZDB_ENABLED != 0));
}

BOOST_AUTO_TEST_CASE(fixed_offsets_do_not_need_the_time_zone_database)
{
    BOOST_TEST(eval(R"(@"2021-01-02T10:10:10+01:00" - @"2021-01-01T10:10:10Z")") == json("PT23H"));
    BOOST_TEST(eval(R"(@"2021-01-02T10:10:10Z" - @"2021-01-01T10:10:10Z")") == json("P1D"));
}

BOOST_AUTO_TEST_CASE(named_zone_values_still_parse)
{
    const auto parsed = orion::bre::feel::parse_datetime("2021-01-01T10:10:10@Europe/Paris");
    BOOST_REQUIRE(parsed.has_value());
    BOOST_TEST(parsed->has_tz);
    BOOST_TEST(parsed->time.h == 10);
    BOOST_TEST(eval(R"(string((@"2021-01-01T10:10:10@Europe/Paris").timezone))") == json("Europe/Paris"));
}

BOOST_AUTO_TEST_CASE(now_and_today_return_iso_values)
{
    const json now = eval("now()");
    const json today = eval("today()");
    BOOST_REQUIRE(now.is_string());
    BOOST_REQUIRE(today.is_string());
    BOOST_TEST(std::regex_match(now.get<std::string>(), std::regex(R"(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})")),
               "now() = " << now);
    BOOST_TEST(std::regex_match(today.get<std::string>(), std::regex(R"(\d{4}-\d{2}-\d{2})")),
               "today() = " << today);
}

#if ORION_TZDB_ENABLED

// ---------- std::chrono time zone database available ----------

BOOST_AUTO_TEST_CASE(named_zones_use_zone_rules)
{
    const auto winter = orion::bre::feel::named_timezone_offset_seconds(
        "Europe/Paris", orion::bre::feel::Date{2021, 1, 1}, orion::bre::feel::Time{10, 10, 10});
    BOOST_REQUIRE(winter.has_value());
    BOOST_TEST(*winter == 3600);

    // Europe/Paris is UTC+1 and Asia/Dhaka UTC+6 on these dates.
    BOOST_TEST(eval(R"(@"2021-01-02T10:10:10@Europe/Paris" - @"2021-01-01T10:10:10@Asia/Dhaka")") == json("P1DT5H"));
    BOOST_TEST(eval(R"(@"2021-01-02T10:10:10+01:00" - @"2021-01-01T10:10:10@Europe/Paris")") == json("P1D"));
}

#else

// ---------- Built without std::chrono (ORION_ENABLE_TZDB=OFF) ----------

BOOST_AUTO_TEST_CASE(named_zone_offsets_are_unknown)
{
    const auto offset = orion::bre::feel::named_timezone_offset_seconds(
        "Europe/Paris", orion::bre::feel::Date{2021, 1, 1}, orion::bre::feel::Time{10, 10, 10});
    BOOST_TEST(!offset.has_value());

    const auto parsed = orion::bre::feel::parse_datetime("2021-01-01T10:10:10@Europe/Paris");
    BOOST_REQUIRE(parsed.has_value());
    BOOST_TEST(parsed->tz_offset_seconds == 0);
}

BOOST_AUTO_TEST_CASE(named_zones_are_treated_as_utc_offset_zero)
{
    // Documented limitation: without the database both zones resolve to offset 0.
    BOOST_TEST(eval(R"(@"2021-01-02T10:10:10@Europe/Paris" - @"2021-01-01T10:10:10@Asia/Dhaka")") == json("P1D"));
    BOOST_TEST(eval(R"(@"2021-01-02T10:10:10+01:00" - @"2021-01-01T10:10:10@Europe/Paris")") == json("PT23H"));
}

#endif

BOOST_AUTO_TEST_SUITE_END()
