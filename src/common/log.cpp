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

#include "log.hpp"
#include <spdlog/sinks/hourly_file_sink.h>
#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;

namespace orion::common
{
std::shared_ptr<spdlog::logger> init_hourly_logger(const std::string& name)
{
static std::mutex mtx;
std::scoped_lock lock(mtx);

if (auto existing = spdlog::get(name)) {
    return existing;
}

fs::path logdir = fs::path("dat") / "log"; // repository-relative
std::error_code error_code;
fs::create_directories(logdir, error_code);

// hourly_file_sink_mt automatically appends timestamp, just provide base name
auto base_filename = logdir / (name + ".log");
auto file_sink = std::make_shared<spdlog::sinks::hourly_file_sink_mt>(base_filename.string(), false);

auto logger = std::make_shared<spdlog::logger>(name, file_sink);
logger->set_level(spdlog::level::info);
logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

spdlog::register_logger(logger);
return logger;
}
}