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

#include <format>
#include <memory>
#include <string>
#include <string_view>

namespace orion::api {

  enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
  };

  /**
   * @brief Abstract logger interface for the Orion BRE library
   * 
   * This interface allows the library to be independent of specific logging implementations.
   * Applications can provide their own logger implementation (e.g., spdlog, Boost.Log, etc.)
   */
  class ILogger {
  public:
    virtual ~ILogger() = default;
    
    virtual void log(LogLevel level, std::string_view message) = 0;
    virtual void critical(std::string_view message) = 0;
    virtual void error(std::string_view message) = 0;
    virtual void warn(std::string_view message) = 0;
    virtual void info(std::string_view message) = 0;
    virtual void debug(std::string_view message) = 0;
    virtual void trace(std::string_view message) = 0;
    virtual void flush() = 0;
  };

  /**
   * @brief Logger singleton with Pimpl pattern
   * 
   * This class provides a global logger instance that can be configured by the application.
   * The library uses this logger, and applications inject their specific implementation.
   */
  class Logger {
  public:
    static auto instance() -> Logger& {
      static Logger logger;
      return logger;
    }

    Logger();
    ~Logger();

    // Non-copyable, non-moveable (singleton)
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    /**
     * @brief Set the logger implementation
     * @param logger_impl The concrete logger implementation (e.g., SpdlogLogger, BoostTestLogger)
     */
    void set_logger(std::shared_ptr<ILogger> logger_impl);

    /**
     * @brief Get the current logger implementation
     */
    auto get_logger() const -> std::shared_ptr<ILogger>;

    void log(LogLevel level, std::string_view message) const;
    void critical(std::string_view message) const;
    void error(std::string_view message) const;
    void warn(std::string_view message) const;
    void info(std::string_view message) const;
    void debug(std::string_view message) const;
    void trace(std::string_view message) const;
    void flush() const;

  private:
    class Impl;
    std::unique_ptr<Impl> pimpl;
  };

  // Convenience functions for formatted logging
  template<typename... Args>
  auto log(LogLevel level, std::string_view message, Args&&... args) -> void {
    Logger::instance().log(level, std::vformat(message, std::make_format_args(args...)));
  }

  template<typename... Args>
  auto critical(std::string_view message, Args&&... args) -> void {
    Logger::instance().critical(std::vformat(message, std::make_format_args(args...)));
  }

  template<typename... Args>
  auto error(std::string_view message, Args&&... args) -> void {
    Logger::instance().error(std::vformat(message, std::make_format_args(args...)));
  }

  template<typename... Args>
  auto warn(std::string_view message, Args&&... args) -> void {
    Logger::instance().warn(std::vformat(message, std::make_format_args(args...)));
  }

  template<typename... Args>
  auto info(std::string_view message, Args&&... args) -> void {
    Logger::instance().info(std::vformat(message, std::make_format_args(args...)));
  }

  template<typename... Args>
  auto debug(std::string_view message, Args&&... args) -> void {
    Logger::instance().debug(std::vformat(message, std::make_format_args(args...)));
  }

  template<typename... Args>
  auto trace(std::string_view message, Args&&... args) -> void {
    Logger::instance().trace(std::vformat(message, std::make_format_args(args...)));
  }

} // namespace orion::api
