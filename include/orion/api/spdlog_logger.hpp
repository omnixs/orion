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
#include <spdlog/logger.h>
#include <memory>

namespace orion::api {

  /**
   * @brief spdlog-based logger implementation for applications
   */
  class SpdlogLogger : public ILogger {
  public:
    SpdlogLogger();
    explicit SpdlogLogger(std::shared_ptr<spdlog::logger> logger);
    ~SpdlogLogger() override = default;

    void log(LogLevel level, std::string_view message) override;
    void critical(std::string_view message) override;
    void error(std::string_view message) override;
    void warn(std::string_view message) override;
    void info(std::string_view message) override;
    void debug(std::string_view message) override;
    void trace(std::string_view message) override;
    void flush() override;

    /**
     * @brief Get the underlying spdlog logger for advanced configuration
     */
    auto get_spdlog_logger() -> std::shared_ptr<spdlog::logger>;

  private:
    std::shared_ptr<spdlog::logger> m_logger;
    
    static auto convert_level(LogLevel level) -> spdlog::level::level_enum;
  };

} // namespace orion::api
