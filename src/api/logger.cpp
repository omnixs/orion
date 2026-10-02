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

  namespace {
    constinit NullLogger null_logger;
    constinit Logger global_logger;
  } // namespace

  auto Logger::instance() noexcept -> Logger& {
    return global_logger;
  }

  void Logger::set_logger(std::shared_ptr<ILogger> logger_impl) {
    if (logger_impl) {
      logger_impl_.store(std::move(logger_impl), std::memory_order_release);
    }
  }

  auto Logger::get_logger() const -> std::shared_ptr<ILogger> {
    if (auto impl = logger_impl_.load(std::memory_order_acquire)) {
      return impl;
    }
    // Non-owning alias: no allocation.
    return {std::shared_ptr<ILogger>{}, &null_logger};
  }

  void Logger::log(LogLevel level, std::string_view message) const {
    if (const auto impl = logger_impl_.load(std::memory_order_acquire)) {
      impl->log(level, message);
    }
  }

  void Logger::critical(std::string_view message) const {
    if (const auto impl = logger_impl_.load(std::memory_order_acquire)) {
      impl->critical(message);
    }
  }

  void Logger::error(std::string_view message) const {
    if (const auto impl = logger_impl_.load(std::memory_order_acquire)) {
      impl->error(message);
    }
  }

  void Logger::warn(std::string_view message) const {
    if (const auto impl = logger_impl_.load(std::memory_order_acquire)) {
      impl->warn(message);
    }
  }

  void Logger::info(std::string_view message) const {
    if (const auto impl = logger_impl_.load(std::memory_order_acquire)) {
      impl->info(message);
    }
  }

  void Logger::debug(std::string_view message) const {
    if (const auto impl = logger_impl_.load(std::memory_order_acquire)) {
      impl->debug(message);
    }
  }

  void Logger::trace(std::string_view message) const {
    if (const auto impl = logger_impl_.load(std::memory_order_acquire)) {
      impl->trace(message);
    }
  }

  void Logger::flush() const {
    if (const auto impl = logger_impl_.load(std::memory_order_acquire)) {
      impl->flush();
    }
  }

} // namespace orion::api
