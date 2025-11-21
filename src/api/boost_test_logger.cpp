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

#include <orion/api/boost_test_logger.hpp>
#include <boost/test/unit_test.hpp>
#include <string>

namespace orion::api {

  BoostTestLogger::BoostTestLogger() {
    // Map Boost.Test log_level to ILogger LogLevel
    // Read from Boost.Test's log level setting
    // Default to Info (equivalent to Boost.Test's 'message' level)
    auto log_level = boost::unit_test::log_level();

    // Map Boost.Test log levels to our LogLevel
    // boost::unit_test::log_level values:
    //   invalid_log_level = -1,
    //   log_successful_tests = 0,  // "all"
    //   log_test_units = 1,        // "test_suite"  
    //   log_messages = 2,          // "message" (default)
    //   log_warnings = 3,          // "warning"
    //   log_all_errors = 4,        // "error"
    //   log_cpp_exception_errors = 5,
    //   log_system_errors = 6,
    //   log_fatal_errors = 7,
    //   log_nothing = 8            // "nothing"

    if (log_level <= boost::unit_test::log_test_units) {
      // "all" or "test_suite" - show everything including trace/debug
      min_level_ = LogLevel::Trace;
    } else if (log_level == boost::unit_test::log_messages) {
      // "message" (default) - show info and above
      min_level_ = LogLevel::Info;
    } else if (log_level == boost::unit_test::log_warnings) {
      // "warning" - show warnings and above
      min_level_ = LogLevel::Warning;
    } else if (log_level <= boost::unit_test::log_fatal_errors) {
      // "error" levels - show errors and critical only
      min_level_ = LogLevel::Error;
    } else {
      // "nothing" or invalid - only show critical
      min_level_ = LogLevel::Critical;
    }
  }

  void BoostTestLogger::set_min_level(LogLevel level) {
    min_level_ = level;
  }

  auto BoostTestLogger::should_log(LogLevel level) const -> bool {
    // Log if the message level is >= minimum level
    // Lower enum values = more severe = higher priority
    return static_cast<int>(level) <= static_cast<int>(min_level_);
  }

  void BoostTestLogger::log(LogLevel level, std::string_view message) {
    if (!should_log(level)) {
      return;
    }
    std::string formatted = std::string("[") + level_to_string(level) + "] " + std::string(message);
    BOOST_TEST_MESSAGE(formatted);
  }

  void BoostTestLogger::critical(std::string_view message) {
    if (!should_log(LogLevel::Critical)) {
      return;
    }
    std::string formatted = std::string("[CRITICAL] ") + std::string(message);
    BOOST_TEST_MESSAGE(formatted);
  }

  void BoostTestLogger::error(std::string_view message) {
    if (!should_log(LogLevel::Error)) {
      return;
    }
    std::string formatted = std::string("[ERROR] ") + std::string(message);
    BOOST_TEST_MESSAGE(formatted);
  }

  void BoostTestLogger::warn(std::string_view message) {
    if (!should_log(LogLevel::Warning)) {
      return;
    }
    std::string formatted = std::string("[WARN] ") + std::string(message);
    BOOST_TEST_MESSAGE(formatted);
  }

  void BoostTestLogger::info(std::string_view message) {
    if (!should_log(LogLevel::Info)) {
      return;
    }
    std::string formatted = std::string("[INFO] ") + std::string(message);
    BOOST_TEST_MESSAGE(formatted);
  }

  void BoostTestLogger::debug(std::string_view message) {
    if (!should_log(LogLevel::Debug)) {
      return;
    }
    std::string formatted = std::string("[DEBUG] ") + std::string(message);
    BOOST_TEST_MESSAGE(formatted);
  }

  void BoostTestLogger::trace(std::string_view message) {
    if (!should_log(LogLevel::Trace)) {
      return;
    }
    std::string formatted = std::string("[TRACE] ") + std::string(message);
    BOOST_TEST_MESSAGE(formatted);
  }

  void BoostTestLogger::flush() {
    // Boost.Test doesn't require flushing
  }

  auto BoostTestLogger::level_to_string(LogLevel level) -> const char* {
    switch (level) {
      case LogLevel::Trace:    return "TRACE";
      case LogLevel::Debug:    return "DEBUG";
      case LogLevel::Info:     return "INFO";
      case LogLevel::Warning:  return "WARN";
      case LogLevel::Error:    return "ERROR";
      case LogLevel::Critical: return "CRITICAL";
      default:                 return "UNKNOWN";
    }
  }

} // namespace orion::api
