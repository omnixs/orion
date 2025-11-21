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

#include <orion/api/spdlog_logger.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace orion::api {

  SpdlogLogger::SpdlogLogger() {
    // Try to get existing "orion" logger, or create a new console logger
    m_logger = spdlog::get("orion");
    if (!m_logger) {
      m_logger = spdlog::stdout_color_mt("orion");
      m_logger->set_level(spdlog::level::info);
    }
  }

  SpdlogLogger::SpdlogLogger(std::shared_ptr<spdlog::logger> logger) 
    : m_logger(std::move(logger)) {
    if (!m_logger) {
      // Fallback to default logger
      m_logger = spdlog::stdout_color_mt("orion_fallback");
    }
  }

  void SpdlogLogger::log(LogLevel level, std::string_view message) {
    m_logger->log(convert_level(level), message);
  }

  void SpdlogLogger::critical(std::string_view message) {
    m_logger->critical(message);
  }

  void SpdlogLogger::error(std::string_view message) {
    m_logger->error(message);
  }

  void SpdlogLogger::warn(std::string_view message) {
    m_logger->warn(message);
  }

  void SpdlogLogger::info(std::string_view message) {
    m_logger->info(message);
  }

  void SpdlogLogger::debug(std::string_view message) {
    m_logger->debug(message);
  }

  void SpdlogLogger::trace(std::string_view message) {
    m_logger->trace(message);
  }

  void SpdlogLogger::flush() {
    m_logger->flush();
  }

  auto SpdlogLogger::get_spdlog_logger() -> std::shared_ptr<spdlog::logger> {
    return m_logger;
  }

  auto SpdlogLogger::convert_level(LogLevel level) -> spdlog::level::level_enum {
    switch (level) {
      case LogLevel::Trace:    return spdlog::level::trace;
      case LogLevel::Debug:    return spdlog::level::debug;
      case LogLevel::Info:     return spdlog::level::info;
      case LogLevel::Warning:  return spdlog::level::warn;
      case LogLevel::Error:    return spdlog::level::err;
      case LogLevel::Critical: return spdlog::level::critical;
      default:                 return spdlog::level::info;
    }
  }

} // namespace orion::api
