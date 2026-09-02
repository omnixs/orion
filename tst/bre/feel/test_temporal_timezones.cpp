/*
 * ORION Optimized Rule Integration & Operations Native
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: 2025 ORION contributors
 */

/**
 * @file test_temporal_timezones.cpp
 * @brief Tests for named time zone resolution and temporal offset/timezone properties
 */

#include <boost/test/unit_test.hpp>
#include <orion/bre/feel/evaluator.hpp>
#include <orion/bre/feel/types.hpp>
#include <nlohmann/json.hpp>
#include "test_helpers.hpp"

using orion::bre::feel::Evaluator;
using orion::bre::feel::test::get_test_eval_ctx;
using json = nlohmann::json;

BOOST_AUTO_TEST_SUITE(FeelTemporalTimezoneSuite)

BOOST_AUTO_TEST_CASE(named_timezone_offset_unknown_zone_is_nullopt)
{
    const auto offset = orion::bre::feel::named_timezone_offset_seconds(
        "Not/AZone", orion::bre::feel::Date{2017, 8, 10}, orion::bre::feel::Time{10, 20, 0});

    BOOST_CHECK(!offset.has_value());
}

BOOST_AUTO_TEST_CASE(named_timezone_offset_uses_zone_rules_when_available)
{
    // Europe/Paris is UTC+2 in August (CEST) and UTC+1 in January (CET).
    const auto summer = orion::bre::feel::named_timezone_offset_seconds(
        "Europe/Paris", orion::bre::feel::Date{2017, 8, 10}, orion::bre::feel::Time{10, 20, 0});
    const auto winter = orion::bre::feel::named_timezone_offset_seconds(
        "Europe/Paris", orion::bre::feel::Date{2017, 1, 10}, orion::bre::feel::Time{10, 20, 0});

    // Toolchains without a time zone database report no offset; both must then agree.
    if (summer.has_value())
    {
        BOOST_REQUIRE(winter.has_value());
        BOOST_CHECK_EQUAL(*summer, 2 * 3600);
        BOOST_CHECK_EQUAL(*winter, 1 * 3600);
    }
    else
    {
        BOOST_CHECK(!winter.has_value());
    }
}

BOOST_AUTO_TEST_CASE(named_timezone_offset_rejects_out_of_range_year)
{
    const auto offset = orion::bre::feel::named_timezone_offset_seconds(
        "Europe/Paris", orion::bre::feel::Date{999999999, 12, 31}, orion::bre::feel::Time{23, 59, 59});

    BOOST_CHECK(!offset.has_value());
}

BOOST_AUTO_TEST_CASE(datetime_with_named_zone_parses)
{
    const auto parsed = orion::bre::feel::parse_datetime("2017-08-10T10:20:00@Europe/Paris");

    BOOST_REQUIRE(parsed.has_value());
    BOOST_CHECK(parsed->has_tz);
    BOOST_CHECK_EQUAL(parsed->time.h, 10);
}

BOOST_AUTO_TEST_CASE(datetime_with_empty_named_zone_is_invalid)
{
    BOOST_CHECK(!orion::bre::feel::parse_datetime("2017-08-10T10:20:00@").has_value());
}

BOOST_AUTO_TEST_CASE(time_offset_property_returns_duration)
{
    // DMN 1.5: `time offset` is a days-and-time duration, not the raw lexical offset.
    auto result = Evaluator::evaluate("time(\"10:20:00+02:00\").time offset", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(result.get<std::string>(), "PT2H");

    auto utc = Evaluator::evaluate("time(\"10:20:00Z\").time offset", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(utc.get<std::string>(), "PT0H");
}

BOOST_AUTO_TEST_CASE(timezone_property_returns_named_zone_only)
{
    auto named = Evaluator::evaluate("time(\"10:20:00@Europe/Paris\").timezone", {}, get_test_eval_ctx());
    BOOST_CHECK_EQUAL(named.get<std::string>(), "Europe/Paris");

    // A numeric offset has no IANA zone name.
    auto offset_only = Evaluator::evaluate("time(\"10:20:00+02:00\").timezone", {}, get_test_eval_ctx());
    BOOST_CHECK(offset_only.is_null());
}

BOOST_AUTO_TEST_SUITE_END()
