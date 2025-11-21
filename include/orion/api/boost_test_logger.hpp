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

#pragma once

#include <orion/api/logger.hpp>
#include <boost/test/unit_test.hpp>

namespace orion::api {

  /**
   * @brief Boost.Test-based logger implementation for unit tests
   * 
   * Outputs log messages to BOOST_TEST_MESSAGE for visibility in test results.
   * Respects Boost.Test's --log_level parameter for filtering.
   */
  class BoostTestLogger : public ILogger {
  public:
    BoostTestLogger();
    ~BoostTestLogger() override = default;

    void log(LogLevel level, std::string_view message) override;
    void critical(std::string_view message) override;
    void error(std::string_view message) override;
    void warn(std::string_view message) override;
    void info(std::string_view message) override;
    void debug(std::string_view message) override;
    void trace(std::string_view message) override;
    void flush() override;

    /**
     * @brief Set the minimum log level for filtering
     * @param level The minimum level to log (messages below this level are filtered)
     */
    void set_min_level(LogLevel level);

  private:
    static auto level_to_string(LogLevel level) -> const char*;
    auto should_log(LogLevel level) const -> bool;

    LogLevel min_level_{LogLevel::Info}; // Default to Info (Boost.Test's 'message' level)
  };

} // namespace orion::api
