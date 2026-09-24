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
#include <iostream>
#include <filesystem>
#include <chrono>
#include <orion/api/engine.hpp>
#include <orion/api/logger.hpp>
#include <orion/api/spdlog_logger.hpp>
#include "../common/log.hpp"

int main(int argc, char** argv)
{
    using namespace std;
    using orion::common::init_hourly_logger;

    string model, data;
    size_t iterations = 1;
    size_t warmup_iterations = 0;
    bool timing = false;
    bool quiet = false;
    for (int i = 1; i < argc; ++i)
    {
        string a = argv[i];
        if ((a == "-m" || a == "--model") && i + 1 < argc) { model = argv[++i];
        } else if ((a == "-d" || a == "--data") && i + 1 < argc) { data = argv[++i];
        } else if ((a == "-n" || a == "--iterations") && i + 1 < argc) {
            iterations = stoull(argv[++i]);
        } else if (a == "--warmup" && i + 1 < argc) {
            warmup_iterations = stoull(argv[++i]);
        } else if (a == "--timing") {
            timing = true;
        } else if (a == "--quiet") {
            quiet = true;
        } else { 
            cerr << "Unknown argument: " << a << '\n';
            return 2;
        }
    }
    if (model.empty() || data.empty() || iterations == 0)
    {
        cerr << "Usage: orion-bre -m <model.dmn.xml> -d <data.json> "
                "[-n <iterations>] [--warmup <iterations>] [--timing] [--quiet]\n";
        return 2;
    }

    shared_ptr<spdlog::logger> spdlog_instance;
    if (!quiet)
    {
        spdlog_instance = init_hourly_logger("orion_app");
        auto logger = make_shared<orion::api::SpdlogLogger>(spdlog_instance);
        orion::api::Logger::instance().set_logger(logger);
    }
    else
    {
        orion::api::Logger::instance().set_logger(nullptr);
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

        const auto construction_start = chrono::steady_clock::now();
        orion::api::BusinessRulesEngine engine;
        const auto construction_end = chrono::steady_clock::now();

        const auto load_start = chrono::steady_clock::now();
        auto result = engine.load_dmn_model(dmn_xml);
        const auto load_end = chrono::steady_clock::now();
        if (!result) {
            cerr << "Failed to load DMN model: " << result.error() << '\n';
            return 1;
        }

        const auto input = nlohmann::json::parse(data_json);
        nlohmann::json out;
        for (size_t i = 0; i < warmup_iterations; ++i)
        {
            out = engine.evaluate(input);
        }

        const auto evaluation_start = chrono::steady_clock::now();
        for (size_t i = 0; i < iterations; ++i)
        {
            out = engine.evaluate(input);
        }
        const auto evaluation_end = chrono::steady_clock::now();

        if (timing)
        {
            const chrono::duration<double, milli> construction_time = construction_end - construction_start;
            const chrono::duration<double, milli> load_time = load_end - load_start;
            const chrono::duration<double, milli> evaluation_time = evaluation_end - evaluation_start;
            nlohmann::json metrics{
                {"engineConstructionMs", construction_time.count()},
                {"modelLoadMs", load_time.count()},
                {"warmupIterations", warmup_iterations},
                {"evaluationIterations", iterations},
                {"evaluationTotalMs", evaluation_time.count()},
                {"evaluationMeanUs", evaluation_time.count() * 1000.0 / static_cast<double>(iterations)},
                {"evaluationsPerSecond", static_cast<double>(iterations) * 1000.0 / evaluation_time.count()}
            };
            cout << metrics.dump() << '\n';
        }

        if (!quiet)
        {
            spdlog_instance->info("Result: {}", out.dump());
            cout << out.dump() << endl;
        }
        return 0;
    }
    catch (const exception& e)
    {
        spdlog_instance->error("Error: {}", e.what());
        return 1;
    }
}
