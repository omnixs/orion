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

static orion::api::HitPolicy parse_hp(std::string_view s)
{
    if (s == "FIRST") { return orion::api::HitPolicy::FIRST;
}
    if (s == "UNIQUE") { return orion::api::HitPolicy::UNIQUE;
}
    if (s.rfind("COLLECT", 0) == 0) { return orion::api::HitPolicy::COLLECT;
}
    return orion::api::HitPolicy::FIRST;
}

static orion::api::CollectAggregation parse_agg(std::string_view s)
{
    if (s == "COLLECT:SUM") { return orion::api::CollectAggregation::SUM;
}
    if (s == "COLLECT:COUNT") { return orion::api::CollectAggregation::COUNT;
}
    return orion::api::CollectAggregation::NONE;
}

int main(int argc, char** argv)
{
    using namespace std;
    using orion::common::init_hourly_logger;
    
    // Initialize spdlog for the application and set it as the library's logger
    auto spdlog_instance = init_hourly_logger("orion_app");
    auto logger = std::make_shared<orion::api::SpdlogLogger>(spdlog_instance);
    orion::api::Logger::instance().set_logger(logger);
    
    string model, data;
    string hp;
    for (int i = 1; i < argc; ++i)
    {
        string a = argv[i];
        if ((a == "-m" || a == "--model") && i + 1 < argc) { model = argv[++i];
        } else if ((a == "-d" || a == "--data") && i + 1 < argc) { data = argv[++i];
        } else if (a == "--hit-policy" && i + 1 < argc) { hp = argv[++i];
        } else { 
            spdlog_instance->error("Unknown argument: {}", a);
        }
    }
    if (model.empty() || data.empty())
    {
        spdlog_instance->info("Usage: orion-bre -m <model.dmn.xml> -d <data.json> [--hit-policy FIRST|UNIQUE|COLLECT|COLLECT:SUM|COLLECT:COUNT]");
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

        orion::api::EvalOptions opt{};
        if (!hp.empty())
        {
            opt.overrideHitPolicy = true;
            opt.hitPolicyOverride = parse_hp(hp);
            opt.collectAgg = parse_agg(hp);
        }

        // Use proper BusinessRulesEngine API
        orion::api::BusinessRulesEngine engine;
        std::string error;
        if (!engine.load_dmn_model(dmn_xml, error)) {
            spdlog_instance->error("Failed to load DMN model: {}", error);
            return 1;
        }
        
        auto out = engine.evaluate(data_json, opt);
        spdlog_instance->info("Result: {}", out);
        return 0;
    }
    catch (const exception& e)
    {
        spdlog_instance->error("Error: {}", e.what());
        return 1;
    }
}
