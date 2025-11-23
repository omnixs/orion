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

#include <fstream>
#include <sstream>
#include <filesystem>
#include <orion/api/engine.hpp>
#include <orion/api/logger.hpp>
#include <orion/api/spdlog_logger.hpp>
#include "../common/log.hpp"

int main(int argc, char** argv)
{
    using namespace std;
    using orion::common::init_hourly_logger;
    
    // Initialize spdlog for the application and set it as the library's logger
    auto spdlog_instance = init_hourly_logger("orion_app");
    auto logger = std::make_shared<orion::api::SpdlogLogger>(spdlog_instance);
    orion::api::Logger::instance().set_logger(logger);
    
    string model, data;
    for (int i = 1; i < argc; ++i)
    {
        string a = argv[i];
        if ((a == "-m" || a == "--model") && i + 1 < argc) { model = argv[++i];
        } else if ((a == "-d" || a == "--data") && i + 1 < argc) { data = argv[++i];
        } else { 
            spdlog_instance->error("Unknown argument: {}", a);
        }
    }
    if (model.empty() || data.empty())
    {
        spdlog_instance->info("Usage: orion-bre -m <model.dmn.xml> -d <data.json>");
        return 2;
    }
    try
    {
        ifstream fm(model);
        if (!fm) { throw runtime_error("Cannot open model");
}
        stringstream mm;
        mm << fm.rdbuf();
        string dmn_xml = mm.str();
        ifstream fd(data);
        if (!fd) { throw runtime_error("Cannot open data");
}
        stringstream dd;
        dd << fd.rdbuf();
        string data_json = dd.str();

        // Use proper BusinessRulesEngine API
        orion::api::BusinessRulesEngine engine;
        std::string error;
        if (!engine.load_dmn_model(dmn_xml, error)) {
            spdlog_instance->error("Failed to load DMN model: {}", error);
            return 1;
        }
        
        auto out = engine.evaluate(data_json);
        spdlog_instance->info("Result: {}", out);
        return 0;
    }
    catch (const exception& e)
    {
        spdlog_instance->error("Error: {}", e.what());
        return 1;
    }
}
