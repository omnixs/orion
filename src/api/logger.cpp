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

#include <orion/api/logger.hpp>

namespace orion::api {

  /**
   * @brief Null logger implementation (discards all log messages)
   */
  class NullLogger : public ILogger {
  public:
    void log(LogLevel /*level*/, std::string_view /*message*/) override {}
    void critical(std::string_view /*message*/) override {}
    void error(std::string_view /*message*/) override {}
    void warn(std::string_view /*message*/) override {}
    void info(std::string_view /*message*/) override {}
    void debug(std::string_view /*message*/) override {}
    void trace(std::string_view /*message*/) override {}
    void flush() override {}
  };

  class Logger::Impl {
  public:
    Impl() : logger_impl(std::make_shared<NullLogger>()) {}

    std::shared_ptr<ILogger> logger_impl;
  };

  Logger::Logger() : pimpl(std::make_unique<Impl>()) {}

  Logger::~Logger() = default;

  void Logger::set_logger(std::shared_ptr<ILogger> logger_impl) {
    if (logger_impl) {
      pimpl->logger_impl = logger_impl;
    }
  }

  auto Logger::get_logger() const -> std::shared_ptr<ILogger> {
    return pimpl->logger_impl;
  }

  void Logger::log(LogLevel level, std::string_view message) const {
    pimpl->logger_impl->log(level, message);
  }

  void Logger::critical(std::string_view message) const {
    pimpl->logger_impl->critical(message);
  }

  void Logger::error(std::string_view message) const {
    pimpl->logger_impl->error(message);
  }

  void Logger::warn(std::string_view message) const {
    pimpl->logger_impl->warn(message);
  }

  void Logger::info(std::string_view message) const {
    pimpl->logger_impl->info(message);
  }

  void Logger::debug(std::string_view message) const {
    pimpl->logger_impl->debug(message);
  }

  void Logger::trace(std::string_view message) const {
    pimpl->logger_impl->trace(message);
  }

  void Logger::flush() const {
    pimpl->logger_impl->flush();
  }

} // namespace orion::api
