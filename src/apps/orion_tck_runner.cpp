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

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <rapidxml/rapidxml.hpp>
#include <orion/api/engine.hpp>
#include <orion/api/logger.hpp>
#include <orion/api/spdlog_logger.hpp>
#include <orion/common/xml2json.hpp>
#include "../common/log.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;
using json = nlohmann::json;
using orion::common::ParsedCase;
using orion::common::OutputExpectation;

// Setup spdlog with proper color handling
static void setup_console_logging() {
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::info);
    
    // Also set up the library logger
    auto orion_logger = spdlog::stdout_color_mt("orion");
    orion_logger->set_level(spdlog::level::info);
    auto lib_logger = std::make_shared<orion::api::SpdlogLogger>(orion_logger);
    orion::api::Logger::instance().set_logger(lib_logger);
}

// Reset console colors on exit
static void cleanup_console_logging() {
    // Flush all loggers before shutdown
    spdlog::apply_all([](std::shared_ptr<spdlog::logger> l) { l->flush(); });
    // Reset terminal colors to normal
    std::cout << "\033[0m";
    std::cout.flush();
    spdlog::shutdown();
}

static fs::path find_tck_root()
{
    if (const char* env = std::getenv("ORION_TCK_ROOT"))
    {
        fs::path p(env);
        if (fs::exists(p / "TestCases")) { return fs::canonical(p);
}
    }
    std::vector<fs::path> candidates = {fs::path("dat") / "dmn-tck", fs::path("..") / "dat" / "dmn-tck"};
    for (auto& c : candidates) { if (fs::exists(c / "TestCases")) { return fs::canonical(c);
}
}
    fs::path cur = fs::current_path();
    for (int i = 0; i < 6; ++i)
    {
        fs::path probe = cur / "dat" / "dmn-tck";
        if (fs::exists(probe / "TestCases")) { return fs::canonical(probe);
}
        if (cur.has_parent_path()) { cur = cur.parent_path();
        } else { break;
}
    }
    return {};
}

static std::string read_file(const fs::path& p)
{
    std::ifstream f(p, std::ios::binary);
    if (!f) { throw std::runtime_error("Cannot open " + p.string());
}
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}


/* Commented out - replaced with shared orion::common::parseTestXml
static std::vector<ParsedCase> parse_test_xml(const std::string& xml)
{
    std::vector<ParsedCase> out;
    using namespace rapidxml;
    xml_document<> doc;
    std::string buf = xml;
    buf.push_back('\0');
    try { doc.parse<0>(&buf[0]); }
    catch (...) { return out; }
    auto* root = doc.first_node();
    if (!root) return out;
    auto for_each_case = [&](rapidxml::xml_node<>* tc)
    {
        ParsedCase pc;
        if (auto* a = tc->first_attribute("id")) pc.id = a->value();
        pc.input = json::object();
        // inputNode
        for (auto* in = tc->first_node(); in; in = in->next_sibling())
        {
            std::string nm = in->name();
            if (nm == "inputNode" || nm == "inputData" || nm == "tc:inputNode" || nm == "tc:inputData")
            {
                std::string name;
                if (auto* a = in->first_attribute("name")) name = a->value();
                if (name.empty()) continue;
                
                // Check for nested components first (complex input structure)
                bool hasComponents = false;
                nlohmann::json nestedObject = nlohmann::json::object();
                
                for (auto* comp = in->first_node("component"); comp; comp = comp->next_sibling("component"))
                {
                    hasComponents = true;
                    const char* compName = comp->first_attribute("name") ? comp->first_attribute("name")->value() : nullptr;
                    if (compName)
                    {
                        auto* valNode = comp->first_node("value");
                        if (valNode && valNode->value())
                        {
                            std::string xsiType;
                            if (auto* t = valNode->first_attribute("xsi:type")) xsiType = t->value();
                            nestedObject[compName] = parse_value(valNode->value(), xsiType);
                        }
                    }
                }
                
                // Also try with namespace prefix if no components found
                if (!hasComponents) {
                    for (auto* comp = in->first_node("tc:component"); comp; comp = comp->next_sibling("tc:component"))
                    {
                        hasComponents = true;
                        const char* compName = comp->first_attribute("name") ? comp->first_attribute("name")->value() : nullptr;
                        if (compName)
                        {
                            auto* valNode = comp->first_node("tc:value");
                            if (!valNode) valNode = comp->first_node("value");
                            if (valNode && valNode->value())
                            {
                                std::string xsiType;
                                if (auto* t = valNode->first_attribute("xsi:type")) xsiType = t->value();
                                nestedObject[compName] = parse_value(valNode->value(), xsiType);
                            }
                        }
                    }
                }
                
                if (hasComponents)
                {
                    // Use the nested object structure
                    pc.input[name] = nestedObject;
                }
                else
                {
                    // Handle simple value structure
                    auto* valNode = in->first_node("value");
                    if (valNode && valNode->value())
                    {
                        std::string xsiType;
                        if (auto* t = valNode->first_attribute("xsi:type")) xsiType = t->value();
                        pc.input[name] = parse_value(valNode->value(), xsiType);
                    }
                }
            }
            else if (nm == "resultNode" || nm == "outputNode" || nm == "tc:resultNode" || nm == "tc:outputNode")
            {
                std::string outName;
                if (auto* a = in->first_attribute("name")) outName = a->value();
                std::string outId;
                if (auto* a = in->first_attribute("id")) outId = a->value();
                if (outId.empty()) outId = outName;
                
                // Create one output expectation per resultNode (matching tst_bre methodology)
                OutputExpectation oe;
                oe.name = outName;
                oe.id = outId;
                
                auto* exp = in->first_node("expected");
                if (!exp) exp = in->first_node("tc:expected"); // Try with namespace prefix
                if (exp)
                {
                    // Check for list structure first (hit policy tests with multiple results)
                    auto* listNode = exp->first_node("list");
                    bool hasList = false;
                    if (listNode) {
                        hasList = true;
                        // For list structure, create a single combined expectation
                        nlohmann::json listArray = nlohmann::json::array();
                        for (auto* item = listNode->first_node("item"); item; item = item->next_sibling("item")) {
                            // Check if this item has a direct value (like <item><value>...</value></item>)
                            auto* directValueNode = item->first_node("value");
                            if (directValueNode) {
                                // Direct value item (e.g., string values in collection)
                                std::string xsiType;
                                if (auto* t = directValueNode->first_attribute("xsi:type")) xsiType = t->value();
                                
                                if (auto* nilAttr = directValueNode->first_attribute("xsi:nil")) {
                                    std::string nilValue = nilAttr->value();
                                    if (nilValue == "true") {
                                        listArray.push_back(nullptr);
                                    } else {
                                        listArray.push_back(directValueNode->value() ? parse_value(directValueNode->value(), xsiType) : nullptr);
                                    }
                                } else {
                                    listArray.push_back(directValueNode->value() ? parse_value(directValueNode->value(), xsiType) : nullptr);
                                }
                            } else {
                                // Component-based item (like <item><component name="..."><value>...</value></component></item>)
                                nlohmann::json itemObj = nlohmann::json::object();
                                // Process components within each list item
                                for (auto* comp = item->first_node("component"); comp; comp = comp->next_sibling("component")) {
                                    const char* compName = comp->first_attribute("name") ? comp->first_attribute("name")->value() : nullptr;
                                    if (compName) {
                                        auto* valNode = comp->first_node("value");
                                        if (valNode) {
                                            std::string xsiType;
                                            if (auto* t = valNode->first_attribute("xsi:type")) xsiType = t->value();
                                            
                                            if (auto* nilAttr = valNode->first_attribute("xsi:nil")) {
                                                std::string nilValue = nilAttr->value();
                                                if (nilValue == "true") {
                                                    itemObj[compName] = nullptr;
                                                } else {
                                                    itemObj[compName] = valNode->value() ? parse_value(valNode->value(), xsiType) : nullptr;
                                                }
                                            } else {
                                                itemObj[compName] = valNode->value() ? parse_value(valNode->value(), xsiType) : nullptr;
                                            }
                                        }
                                    }
                                }
                                if (!itemObj.empty()) listArray.push_back(itemObj);
                            }
                        }
                        oe.expected = listArray.dump();
                    }
                    
                    // Check for direct components (non-list structure)
                    else if (exp->first_node("component"))
                    {
                        // For component structure, create a single combined object expectation
                        nlohmann::json componentObj = nlohmann::json::object();
                        for (auto* comp = exp->first_node("component"); comp; comp = comp->next_sibling("component"))
                        {
                            const char* compName = comp->first_attribute("name") ? comp->first_attribute("name")->value() : nullptr;
                            if (compName)
                            {
                                auto* valNode = comp->first_node("value");
                                if (valNode)
                                {
                                    std::string xsiType;
                                    if (auto* t = valNode->first_attribute("xsi:type")) xsiType = t->value();
                                    
                                    // Check for xsi:nil="true" (explicit null)
                                    if (auto* nilAttr = valNode->first_attribute("xsi:nil")) {
                                        std::string nilValue = nilAttr->value();
                                        if (nilValue == "true") {
                                            componentObj[compName] = nullptr;
                                        } else if (valNode->value() == nullptr) {
                                            if (xsiType.find("string") != std::string::npos) {
                                                componentObj[compName] = "";  // Empty string
                                            } else {
                                                componentObj[compName] = nullptr;   // Null value
                                            }
                                        } else {
                                            componentObj[compName] = parse_value(valNode->value(), xsiType);
                                        }
                                    } else {
                                        // Handle normal values when no xsi:nil attribute
                                        if (valNode->value() == nullptr) {
                                            if (xsiType.find("string") != std::string::npos) {
                                                componentObj[compName] = "";  // Empty string
                                            } else {
                                                componentObj[compName] = nullptr;   // Null value
                                            }
                                        } else {
                                            componentObj[compName] = parse_value(valNode->value(), xsiType);
                                        }
                                    }
                                }
                            }
                        }
                        oe.expected = componentObj.dump();
                    }
                    
                    // If no components or list, handle simple value structure
                    else
                    {
                        auto* val = exp->first_node("value");
                        if (!val) val = exp->first_node("tc:value"); // Try with namespace prefix
                        if (val) 
                        {
                            std::string xsiType;
                            if (auto* t = val->first_attribute("xsi:type")) xsiType = t->value();
                            
                            // Check for xsi:nil="true" (explicit null)
                            if (auto* nilAttr = val->first_attribute("xsi:nil")) {
                                std::string nilValue = nilAttr->value();
                                if (nilValue == "true") {
                                    oe.expected = "null";
                                } else {
                                    // Handle normal values
                                    if (val->value() == nullptr) {
                                        // No value content - this could be empty string or null
                                        // Check if xsiType indicates string
                                        if (xsiType.find("string") != std::string::npos) {
                                            oe.expected = "\"\"";  // Empty string
                                        } else {
                                            oe.expected = "null";   // Null value
                                        }
                                    } else {
                                        oe.expected = parse_value(val->value(), xsiType).dump();
                                    }
                                }
                            } else {
                                // Handle normal values when no xsi:nil attribute
                                if (val->value() == nullptr) {
                                    // No value content - this could be empty string or null
                                    // Check if xsiType indicates string
                                    if (xsiType.find("string") != std::string::npos) {
                                        oe.expected = "\"\"";  // Empty string
                                    } else {
                                        oe.expected = "null";   // Null value
                                    }
                                } else {
                                    oe.expected = parse_value(val->value(), xsiType).dump();
                                }
                            }
                        }
                    }
                }
                if (!oe.expected.empty()) pc.outputs.push_back(oe);
            }
        }
        if (pc.outputs.size() == 1) pc.outputs[0].id = pc.id.empty() ? pc.outputs[0].id : pc.id;
        // align with single-output convention
        if (!pc.outputs.empty()) out.push_back(std::move(pc));
    };
    if (std::string(root->name()) == "testCases" || std::string(root->name()) == "tc:testCases")
    {
        for (auto* tc = root->first_node("testCase"); tc; tc = tc->next_sibling("testCase")) for_each_case(tc);
        if (!out.size()) {
            // Try with namespace prefix
            for (auto* tc = root->first_node("tc:testCase"); tc; tc = tc->next_sibling("tc:testCase")) for_each_case(tc);
        }
    }
    else if (std::string(root->name()) == "testCase" || std::string(root->name()) == "tc:testCase") { for_each_case(root); }
    return out;
}
*/ // End of commented out local parse_test_xml function

static void ensure_parent(const fs::path& p)
{
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
}

struct Config
{
    std::string version = "0.1.0";
    fs::path root;
    std::string testFilter;
    bool verbose = false;
    bool stopOnFailure = false;
};

struct TestStats
{
    std::size_t total_outputs = 0;
    std::size_t ok = 0;
    std::size_t fail = 0;
    std::size_t total_cases = 0;
    std::size_t passed_cases = 0;
    std::size_t failed_cases = 0;
    std::size_t total_features = 0;
    std::size_t passed_features = 0;
};

struct DirInfo {
    fs::path dir;
    fs::path dmn;
    std::vector<fs::path> xmls;
};

static Config parse_command_line(int argc, char** argv)
{
    Config config;
    std::string rootArg;
    
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "--version" && i + 1 < argc) { config.version = argv[++i];
        } else if (a == "--root" && i + 1 < argc) { rootArg = argv[++i];
        } else if (a == "--test" && i + 1 < argc) { config.testFilter = argv[++i];
        } else if (a == "--verbose") { config.verbose = true;
        } else if (a == "--stop-on-failure" || a == "--stop" || a == "-s") { config.stopOnFailure = true;
        } else if (a == "--help" || a == "-h") {
            spdlog::info("Usage: {} [options]", argv[0]);
            spdlog::info("Options:");
            spdlog::info("  --root <path>         TCK root directory (default: auto-detect)");
            spdlog::info("  --version <version>   Engine version (default: 0.1.0)");
            spdlog::info("  --test <pattern>      Only run tests matching pattern (e.g., 0105-feel-math)");
            spdlog::info("  --verbose             Enable verbose debug output");
            spdlog::info("  --stop-on-failure     Stop testing on first failure");
            spdlog::info("  --help                Show this help");
            exit(0);
        }
    }
    
    config.root = rootArg.empty() ? find_tck_root() : fs::path(rootArg);
    if (config.root.empty()) { throw std::runtime_error("TCK root not found");
}
    
    return config;
}

static std::vector<DirInfo> discover_test_directories(const fs::path& base_path, std::string_view testFilter)
{
    std::vector<DirInfo> dirs;
    
    if (!fs::exists(base_path)) { return dirs;
}
    
    for (auto& e : fs::recursive_directory_iterator(base_path))
    {
        if (!e.is_directory()) { continue;
}
        
        DirInfo di;
        di.dir = e.path();
        
        // Apply test filter if specified
        if (!testFilter.empty()) {
            std::string dirPath = di.dir.string();
            if (dirPath.find(testFilter) == std::string::npos) {
                continue; // Skip this directory if it doesn't match the filter
            }
        }
        
        // Scan for DMN and test XML files
        for (auto& f : fs::directory_iterator(di.dir))
        {
            if (!f.is_regular_file()) { continue;
}
            
            auto ext = f.path().extension().string();
            if (ext == ".dmn") { 
                if (di.dmn.empty()) { di.dmn = f.path(); 
}
            }
            else if (ext == ".xml" && f.path().filename().string().find("-test-") != std::string::npos) {
                di.xmls.push_back(f.path());
            }
        }
        
        if (!di.dmn.empty() && !di.xmls.empty()) {
            dirs.push_back(std::move(di));
        }
    }
    
    return dirs;
}

static std::vector<DirInfo> get_standard_test_cases(const Config& config)
{
    fs::path base = config.root / "TestCases";
    if (!fs::exists(base)) { throw std::runtime_error("TestCases directory missing");
}
    
    return discover_test_directories(base, config.testFilter);
}

static std::vector<DirInfo> get_extra_test_cases(const Config& config)
{
    fs::path extra_dir = "dat/tst/dmn-tck-extra";
    return discover_test_directories(extra_dir, config.testFilter);
}

// Helper: Extract output value from actual result JSON
static std::string extract_output_value(
    const json& actual,
    const std::string& outputId,
    const std::string& outputName,
    const std::string& expected)
{
    if (!actual.is_object())
    {
        return "";
    }
    
    // Handle component-based outputs (like Approval_Status, Approval_Rate)
    if (outputId.find("_") != std::string::npos)
    {
        size_t underscorePos = outputId.find("_");
        std::string decisionName = outputId.substr(0, underscorePos);
        std::string componentName = outputId.substr(underscorePos + 1);
        
        auto decisionIt = actual.find(decisionName);
        if (decisionIt != actual.end() && decisionIt->is_object())
        {
            auto componentIt = decisionIt->find(componentName);
            if (componentIt == decisionIt->end() && decisionIt->contains(decisionName))
            {
                // Try deeper nesting
                auto nestedDecision = (*decisionIt)[decisionName];
                if (nestedDecision.is_object())
                {
                    componentIt = nestedDecision.find(componentName);
                    if (componentIt != nestedDecision.end())
                    {
                        return componentIt->dump();
                    }
                }
            }
            else if (componentIt != decisionIt->end())
            {
                return componentIt->dump();
            }
        }
        return "";
    }
    
    // Regular output
    auto it = actual.find(outputName);
    if (it == actual.end())
    {
        return "";
    }
    
    try
    {
        nlohmann::json expectedObj = nlohmann::json::parse(expected);
        
        if (expectedObj.is_array() && it->is_array())
        {
            return it->dump();
        }
        else if (expectedObj.is_object() && it->is_object())
        {
            if (it->contains(outputName) && (*it)[outputName].is_object())
            {
                return (*it)[outputName].dump();
            }
            return it->dump();
        }
        
        return it->dump();
    }
    catch (...)
    {
        return it->dump();
    }
}

// Helper: Compare expected vs actual values with numeric tolerance
static bool compare_values(std::string_view expected, std::string_view actual)
{
    try
    {
        double expected_num = std::stod(std::string(expected));
        std::string numeric_str(actual);
        
        if (actual.front() == '"' && actual.back() == '"' && actual.length() > 2)
        {
            numeric_str = actual.substr(1, actual.length() - 2);
        }
        
        double actual_num = std::stod(numeric_str);
        double tolerance = std::max(1e-10, std::abs(expected_num) * 1e-10);
        return std::abs(expected_num - actual_num) <= tolerance;
    }
    catch (...)
    {
        if (expected == "\"\"" && actual == "null")
        {
            return false;
        }
        if (expected == "null" && actual == "null")
        {
            return true;
        }
        return actual == expected;
    }
}

// Helper: Write single test result to CSV
static void write_csv_result(
    std::ofstream& csv,
    const std::string& test_dir,
    const std::string& test_case_id,
    const std::string& result_node_id,
    bool success,
    const std::string& detail)
{
    csv << '"' << test_dir << '"' << ","
        << '"' << test_case_id << '"' << ","
        << '"' << result_node_id << '"' << ",";
    
    if (success)
    {
        csv << "\"SUCCESS\",\"\"\n";
    }
    else
    {
        std::string escaped = detail;
        for (char& ch : escaped)
        {
            if (ch == '"')
            {
                ch = '\'';
            }
        }
        csv << "\"ERROR\",\"" << escaped << "\"\n";
    }
}

static bool execute_single_test_case(
    const std::string& dmn_xml, 
    const ParsedCase& test_case,
    const std::string& test_dir,
    const std::string& test_case_id,
    const Config& config,
    std::ofstream& csv,
    TestStats& stats)
{
    bool case_passed = true;
    std::string input_json = test_case.input.dump();
    
    if (config.verbose) {
        spdlog::debug("[DEBUG] Test: {}/{} case={}", test_dir, test_case_id, test_case.id);
        spdlog::debug("[DEBUG] Input: {}", input_json);
    }
    
    std::string result;
    json actual;
    bool evalOk = true;
    std::string errMsg;
    
    try {
        // Use proper BusinessRulesEngine API
        orion::api::BusinessRulesEngine engine;
        std::string error;
        if (!engine.load_dmn_model(dmn_xml, error)) {
            throw std::runtime_error("Failed to load DMN model: " + error);
        }
        result = engine.evaluate(input_json, {});
        actual = json::parse(result);
        if (config.verbose) {
            spdlog::debug("[DEBUG] Raw result: {}", result);
            spdlog::debug("[DEBUG] Parsed result: {}", actual.dump(2));
        }
    } catch (const std::exception& ex) {
        evalOk = false;
        errMsg = ex.what();
        if (config.verbose) {
            spdlog::debug("[DEBUG] Exception: {}", errMsg);
        }
    }
    
    for (auto& outExp : test_case.outputs)
    {
        ++stats.total_outputs;
        bool success = false;
        std::string detail;
        std::string got;
        
        if (evalOk && actual.is_object())
        {
            got = extract_output_value(actual, outExp.id, outExp.name, outExp.expected);
            
            if (!got.empty())
            {
                success = compare_values(outExp.expected, got);
                if (!success)
                {
                    detail = "FAILURE: '" + outExp.id + "' expected='" + outExp.expected + "' but found='" + got + "'";
                }
            }
            else
            {
                detail = "FAILURE: '" + outExp.id + "' expected='" + outExp.expected + "' but missing output";
            }
        }
        else if (!evalOk)
        {
            detail = "FAILURE: '" + outExp.id + "' exception='" + errMsg + "'";
        }
        
        if (config.verbose)
        {
            spdlog::debug("{} {}/{} result_node={} output={} expected={} actual={}", 
                (success ? "[OK]" : "[FAIL]"), test_dir, test_case_id, test_case.id, outExp.name,
                outExp.expected, (got.empty() ? "<none>" : got));
        }
        
        std::string result_node_id = test_case.id.empty() ? "001" : test_case.id;
        write_csv_result(csv, test_dir, test_case_id, result_node_id, success, detail);
        
        if (success)
        {
            ++stats.ok;
        }
        else
        {
            ++stats.fail;
            case_passed = false;
            
            if (config.stopOnFailure)
            {
                csv.flush();
                spdlog::error("Stopped on first failure: {}", detail);
                spdlog::info("Progress: total={} success={} fail={}", stats.total_outputs, stats.ok, stats.fail);
                exit(1);
            }
        }
    }
    
    return case_passed;
}

static TestStats execute_test_directory_set(
    const std::vector<DirInfo>& dirs,
    const fs::path& base_path, 
    const Config& config,
    std::ofstream& csv,
    const std::string& label = "")
{
    TestStats stats;
    
    for (auto& di : dirs) {
        std::string dmn_xml;
        try { 
            dmn_xml = read_file(di.dmn); 
        }
        catch (...) { 
            continue; 
        }
        
        bool feature_passed = true;
        std::size_t feature_cases_passed = 0;
        std::size_t feature_total_cases = 0;
        
        for (auto& xf : di.xmls) {
            std::string xml;
            try { 
                xml = read_file(xf); 
            }
            catch (...) { 
                continue; 
            }
            
            auto cases = orion::common::parse_test_xml(xml);
            if (cases.empty()) { continue;
}
            
            std::string rel = fs::relative(di.dir, base_path).generic_string();
            std::string testBase = xf.filename().string();
            if (auto pos = testBase.rfind('.'); pos != std::string::npos) {
                testBase = testBase.substr(0, pos);
            }
            
            // Separate test directory path from test case ID for standard CSV format
            std::string test_dir = label.empty() ? rel : ("[" + label + "] " + rel);
            std::string test_case_id = testBase;
            
            for (auto& c : cases) {
                // Count individual outputs as test cases (to match tst_bre methodology)
                int outputs_before = stats.total_outputs;
                int ok_before = stats.ok;
                
                bool case_passed = execute_single_test_case(dmn_xml, c, test_dir, test_case_id, config, csv, stats);
                
                // Update total test cases based on outputs processed
                int outputs_processed = stats.total_outputs - outputs_before;
                int outputs_passed = stats.ok - ok_before;
                
                stats.total_cases += outputs_processed;
                feature_total_cases += outputs_processed;
                stats.passed_cases += outputs_passed;
                stats.failed_cases += (outputs_processed - outputs_passed);
                feature_cases_passed += outputs_passed;
                
                if (!case_passed) {
                    feature_passed = false;
                }
            }
        }
        
        ++stats.total_features;
        if (feature_passed && feature_total_cases > 0) {
            ++stats.passed_features;
        }
        
        // Progress reporting similar to tst_bre format
        if (!config.verbose) {
            std::string feature_name = di.dir.filename().string();
            double percentage = feature_total_cases > 0 ? (feature_cases_passed * 100.0) / feature_total_cases : 0.0;
            
            if (feature_passed && feature_total_cases > 0) {
                spdlog::info("[TEST] Running {}: {}/{} passed ({:.1f}%)", 
                    feature_name, feature_cases_passed, feature_total_cases, percentage);
            } else {
                spdlog::warn("[TEST] Running {}: {}/{} passed ({:.1f}%)", 
                    feature_name, feature_cases_passed, feature_total_cases, percentage);
            }
        }
    }
    
    return stats;
}

static void write_results_files(const Config& config, const TestStats& main_stats [[maybe_unused]], const TestStats& extra_stats [[maybe_unused]])
{
    fs::path outDir = config.root / "TestResults" / "Orion" / config.version;
    ensure_parent(outDir / "dummy");
    fs::path propPath = outDir / "tck_results.properties";
    
    std::ofstream prop(propPath, std::ios::binary | std::ios::trunc);
    if (!prop) { throw std::runtime_error("Cannot write " + propPath.string());
}
    
    time_t now = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    
    prop << "#" << buf << "\n";
    prop << "product.name=Orion DMN Engine\n";
    prop << "product.version=" << config.version << "\n";
    prop << "vendor.name=Orion Project\n";
    prop << "vendor.url=https://example.org/orion\n";
    prop << "product.url=https://example.org/orion\n";
    prop << "product.comment=Orion experimental DMN evaluation (partial literal + decision table support)\n";
    prop << "last.update=" << buf << "\n";
    prop << "instructions.url=https://github.com/dmn-tck/tck\n";
}

static void print_summary(const TestStats& main_stats, const TestStats& extra_stats, const Config& config)
{
    // Print old-style summary for compatibility
    spdlog::info("Finished: test_cases={} passed={} failed={} (outputs: total={} success={} fail={})", 
        main_stats.total_cases, main_stats.passed_cases, main_stats.failed_cases,
        main_stats.total_outputs, main_stats.ok, main_stats.fail);
    
    // Print comprehensive summary in tst_bre style
    std::size_t total_cases = main_stats.total_cases + extra_stats.total_cases;
    std::size_t total_passed = main_stats.passed_cases + extra_stats.passed_cases;
    double overall_success = total_cases > 0 ? (total_passed * 100.0) / total_cases : 0.0;
    
    spdlog::info("DMN TCK Comprehensive Summary: {}/{} individual test cases passed ({:.1f}% success rate)", 
        total_passed, total_cases, overall_success);
    
    // Determine if this was Level-2, Level-3, or both based on test filter
    bool is_level2_only = config.testFilter.find("compliance-level-2") != std::string::npos;
    bool is_level3_only = config.testFilter.find("compliance-level-3") != std::string::npos;
    
    if (main_stats.total_cases > 0) {
        double feature_success = main_stats.total_features > 0 ? (main_stats.passed_features * 100.0) / main_stats.total_features : 0.0;
        double case_success = main_stats.total_cases > 0 ? (main_stats.passed_cases * 100.0) / main_stats.total_cases : 0.0;
        
        if (is_level2_only) {
            spdlog::info("  Level-2 Features: {}/{} feature tests passed ({:.1f}% success rate)", 
                main_stats.passed_features, main_stats.total_features, feature_success);
            spdlog::info("  Level-2 Cases: {}/{} individual test cases passed ({:.1f}% success rate)", 
                main_stats.passed_cases, main_stats.total_cases, case_success);
        } else if (is_level3_only) {
            spdlog::info("  Level-3 Features: {}/{} feature tests passed ({:.1f}% success rate)", 
                main_stats.passed_features, main_stats.total_features, feature_success);
            spdlog::info("  Level-3 Cases: {}/{} individual test cases passed ({:.1f}% success rate)", 
                main_stats.passed_cases, main_stats.total_cases, case_success);
        } else {
            // Mixed or all tests - assume Level-2 for main stats for now
            spdlog::info("  Standard Features: {}/{} feature tests passed ({:.1f}% success rate)", 
                main_stats.passed_features, main_stats.total_features, feature_success);
            spdlog::info("  Standard Cases: {}/{} individual test cases passed ({:.1f}% success rate)", 
                main_stats.passed_cases, main_stats.total_cases, case_success);
        }
    }
    
    // Extra cases summary  
    if (extra_stats.total_cases > 0) {
        double extra_success = extra_stats.total_cases > 0 ? (extra_stats.passed_cases * 100.0) / extra_stats.total_cases : 0.0;
        spdlog::info("  dmn-tck-extra Cases: {}/{} individual test cases passed ({:.1f}% success rate)", 
            extra_stats.passed_cases, extra_stats.total_cases, extra_success);
    }
    
    // Final result - fix the incorrect "All tests passed" logic
    bool all_passed = (main_stats.failed_cases == 0 && extra_stats.failed_cases == 0 && 
                       main_stats.total_cases > 0); // Must have at least some tests to pass
    if (all_passed) {
        spdlog::info("All DMN TCK tests passed successfully!");
    } else {
        spdlog::info("DMN TCK tests completed with {} failures.", 
            (main_stats.failed_cases + extra_stats.failed_cases));
    }
}

int main(int argc, char** argv)
{
    // Setup proper spdlog console logging with colors
    setup_console_logging();

    // Ensure console colors are reset on exit
    std::atexit(cleanup_console_logging);

    try
    {
        auto log = orion::common::init_hourly_logger("orion_tck_runner");
        log->info("TCK Runner started");
        
        Config config = parse_command_line(argc, argv);
        
        // Enable debug logging if verbose mode is requested
        if (config.verbose) {
            spdlog::set_level(spdlog::level::debug);
        }
        
        // Setup output files
        fs::path outDir = config.root / "TestResults" / "Orion" / config.version;
        ensure_parent(outDir / "dummy");
        fs::path csvPath = outDir / "tck_results.csv";
        
        std::ofstream csv(csvPath, std::ios::binary | std::ios::trunc);
        if (!csv) { throw std::runtime_error("Cannot write " + csvPath.string());
}
        
        // Discover and execute standard TCK test cases
        auto standard_dirs = get_standard_test_cases(config);
        fs::path standard_base = config.root / "TestCases";
        TestStats main_stats = execute_test_directory_set(standard_dirs, standard_base, config, csv);
        
        // Discover and execute extra test cases
        auto extra_dirs = get_extra_test_cases(config);
        fs::path extra_base = fs::path("dat/tst/dmn-tck-extra");
        TestStats extra_stats = execute_test_directory_set(extra_dirs, extra_base, config, csv, "EXTRA");
        
        csv.flush();
        
        // Write results files
        write_results_files(config, main_stats, extra_stats);
        
        // Print summary
        print_summary(main_stats, extra_stats, config);
        
        std::size_t total_passed = main_stats.passed_cases + extra_stats.passed_cases;
        std::size_t total_cases = main_stats.total_cases + extra_stats.total_cases;
        
        log->info("TCK Runner completed: {}/{} test cases passed", total_passed, total_cases);
        spdlog::info("TCK Runner completed: {}/{} test cases passed", total_passed, total_cases);
        log->flush();
        
        return (main_stats.fail || extra_stats.fail) ? 1 : 0;
    }
    catch (const std::exception& ex)
    {
        spdlog::error("Error: {}", ex.what());
        return 2;
    }
}
