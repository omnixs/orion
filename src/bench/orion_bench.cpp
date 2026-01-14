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

/*
 * ORION Business Rules Engine - Benchmark Suite
 * Performance benchmarks for DMN TestExamples
 */

#include <benchmark/benchmark.h>
#include <orion/api/engine.hpp>
#include <orion/api/logger.hpp>
#include <orion/api/spdlog_logger.hpp>
#include <string>
#include <stdexcept>

using namespace orion::api;

// ============================================================================
// TestExample 1: calc-discount/A.1.dmn - UNIQUE Hit Policy
// Simple pricing based on age and priority service
// ============================================================================

static const char* kCalcDiscountA1_DMN = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<definitions namespace="http://onedecision.io/examples/" name="Calculate Price Decision Model" id="A.1" xmlns="https://www.omg.org/spec/DMN/20230324/DMN15.xsd">
    <description>Implements the pricing model</description>
    <inputData name="age">
        <variable typeRef="ns2:number" name="age" xmlns:ns2="https://www.omg.org/spec/DMN/20230324/FEEL/">
            <description>The age of the applicant</description>
        </variable>
    </inputData>
    <inputData name="priority">
        <variable typeRef="ns2:boolean" name="priority" xmlns:ns2="https://www.omg.org/spec/DMN/20230324/FEEL/">
            <description>Whether priority service was requested</description>
        </variable>
    </inputData>
    <decision name="Calculate Price Decision" id="calcPrice_d">
        <description>Determine price based on age of applicant and whether priority service requested</description>
        <variable typeRef="ns2:number" name="price" xmlns:ns2="https://www.omg.org/spec/DMN/20230324/FEEL/">
            <description>Price to charge customer</description>
        </variable>
        <decisionTable hitPolicy="UNIQUE" preferredOrientation="Rule-as-Row">
            <input>
                <inputExpression>
                    <text>age</text>
                </inputExpression>
            </input>
            <input>
                <inputExpression>
                    <text>priority</text>
                </inputExpression>
            </input>
            <output name="price"/>
            <rule>
                <inputEntry>
                    <text>&lt;2</text>
                </inputEntry>
                <inputEntry>
                    <text>false</text>
                </inputEntry>
                <outputEntry>
                    <text>0</text>
                </outputEntry>
            </rule>
            <rule>
                <inputEntry>
                    <text>&lt;2</text>
                </inputEntry>
                <inputEntry>
                    <text>true</text>
                </inputEntry>
                <outputEntry>
                    <text>10</text>
                </outputEntry>
            </rule>
            <rule>
                <inputEntry>
                    <text>[3..16]</text>
                </inputEntry>
                <inputEntry>
                    <text>false</text>
                </inputEntry>
                <outputEntry>
                    <text>20</text>
                </outputEntry>
            </rule>
            <rule>
                <inputEntry>
                    <text>[3..16]</text>
                </inputEntry>
                <inputEntry>
                    <text>true</text>
                </inputEntry>
                <outputEntry>
                    <text>30</text>
                </outputEntry>
            </rule>
            <rule>
                <inputEntry>
                    <text>&gt;=16</text>
                </inputEntry>
                <inputEntry>
                    <text>false</text>
                </inputEntry>
                <outputEntry>
                    <text>40</text>
                </outputEntry>
            </rule>
            <rule>
                <inputEntry>
                    <text>&gt;=16</text>
                </inputEntry>
                <inputEntry>
                    <text>true</text>
                </inputEntry>
                <outputEntry>
                    <text>50</text>
                </outputEntry>
            </rule>
        </decisionTable>
    </decision>
</definitions>)";

static const char* kCalcDiscountA1_Input1 = R"({"age": 1, "priority": false})";
static const char* kCalcDiscountA1_Input2 = R"({"age": 1, "priority": true})";
static const char* kCalcDiscountA1_Input3 = R"({"age": 10, "priority": false})";
static const char* kCalcDiscountA1_Input4 = R"({"age": 10, "priority": true})";
static const char* kCalcDiscountA1_Input5 = R"({"age": 25, "priority": false})";
static const char* kCalcDiscountA1_Input6 = R"({"age": 25, "priority": true})";

// ============================================================================
// TestExample 2: calc-discount/A.2.dmn - COLLECT+SUM Hit Policy
// Additive pricing with aggregation
// ============================================================================

static const char* kCalcDiscountA2_DMN = R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<definitions namespace="http://onedecision.io/examples/" name="Calculate Price Decision Model" id="A.2" xmlns="https://www.omg.org/spec/DMN/20230324/DMN15.xsd">
    <description>Implements the pricing model</description>
    <inputData name="age">
        <variable typeRef="ns2:number" name="age" xmlns:ns2="https://www.omg.org/spec/DMN/20230324/FEEL/">
            <description>The age of the applicant</description>
        </variable>
    </inputData>
    <inputData name="priority">
        <variable typeRef="ns2:boolean" name="priority" xmlns:ns2="https://www.omg.org/spec/DMN/20230324/FEEL/">
            <description>Whether priority service was requested</description>
        </variable>
    </inputData>
    <decision name="Calculate Price Decision" id="calcPrice_d">
        <description>Determine price based on age of applicant and whether priority service requested</description>
        <decisionTable hitPolicy="COLLECT" aggregation="SUM" preferredOrientation="Rule-as-Row">
            <input>
                <inputExpression>
                    <text>age</text>
                </inputExpression>
            </input>
            <input>
                <inputExpression>
                    <text>priority</text>
                </inputExpression>
            </input>
            <output name="price"/>
            <rule>
                <inputEntry>
                    <text>&lt;2</text>
                </inputEntry>
                <inputEntry>
                    <text>-</text>
                </inputEntry>
                <outputEntry>
                    <text>0</text>
                </outputEntry>
            </rule>
            <rule>
                <inputEntry>
                    <text>[3..16]</text>
                </inputEntry>
                <inputEntry>
                    <text>-</text>
                </inputEntry>
                <outputEntry>
                    <text>20</text>
                </outputEntry>
            </rule>
            <rule>
                <inputEntry>
                    <text>&gt;=16</text>
                </inputEntry>
                <inputEntry>
                    <text>-</text>
                </inputEntry>
                <outputEntry>
                    <text>40</text>
                </outputEntry>
            </rule>
            <rule>
                <inputEntry>
                    <text>-</text>
                </inputEntry>
                <inputEntry>
                    <text>true</text>
                </inputEntry>
                <outputEntry>
                    <text>10</text>
                </outputEntry>
            </rule>
        </decisionTable>
    </decision>
</definitions>)";

static const char* kCalcDiscountA2_Input = R"({"age": 19, "priority": true})";

// ============================================================================
// TestExample 3: order-discount/order-discount.dmn - Volume Discount
// Simple range-based discount calculation
// ============================================================================

static const char* kOrderDiscount_DMN = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/DMN15.xsd" id="definitions" name="definitions" namespace="http://camunda.org/schema/1.0/dmn" xmlns:feel="https://www.omg.org/spec/DMN/20230324/FEEL/">
  <decision id="order-discount" name="Order Discount">
    <decisionTable id="decisionTable">
      <input id="input1" label="Amount">
        <inputExpression id="inputExpression1" typeRef="feel:number">        <text>amount</text>
</inputExpression>
      </input>
      <output id="output1" label="Discount" name="discount" typeRef="feel:number" />
      <rule id="row-23632031-1">
        <inputEntry id="UnaryTests_0b4auz4">        <text><![CDATA[< 500]]></text>
</inputEntry>
        <outputEntry id="LiteralExpression_0ksutcc">        <text>0</text>
</outputEntry>
      </rule>
      <rule id="row-23632031-2">
        <inputEntry id="UnaryTests_0h34apy">        <text>[500..999]</text>
</inputEntry>
        <outputEntry id="LiteralExpression_0bqqi2f">        <text>2</text>
</outputEntry>
      </rule>
      <rule id="row-23632031-3">
        <inputEntry id="UnaryTests_1kb7b4b">        <text>[1000..1999]</text>
</inputEntry>
        <outputEntry id="LiteralExpression_0e0xoae">        <text>3</text>
</outputEntry>
      </rule>
      <rule id="row-23632031-4">
        <inputEntry id="UnaryTests_0nva7ft">        <text>[2000..4999]</text>
</inputEntry>
        <outputEntry id="LiteralExpression_1xvzhwq">        <text>5</text>
</outputEntry>
      </rule>
      <rule id="row-23632031-5">
        <inputEntry id="UnaryTests_0vrfj63">        <text><![CDATA[>= 5000]]></text>
</inputEntry>
        <outputEntry id="LiteralExpression_16gkb8h">        <text>8</text>
</outputEntry>
      </rule>
    </decisionTable>
  </decision>
</definitions>)";

static const char* kOrderDiscount_Input1 = R"({"amount": 250})";
static const char* kOrderDiscount_Input2 = R"({"amount": 750})";
static const char* kOrderDiscount_Input3 = R"({"amount": 1500})";
static const char* kOrderDiscount_Input4 = R"({"amount": 3000})";
static const char* kOrderDiscount_Input5 = R"({"amount": 6000})";

// ============================================================================
// Benchmark Functions - Using Proper Cached Model API
// ============================================================================

// Helper function to create and load engine for each benchmark
static BusinessRulesEngine createEngineForA1() {
    BusinessRulesEngine engine;
    auto result = engine.load_dmn_model(kCalcDiscountA1_DMN);
    if (!result) {
        throw std::runtime_error("Failed to load CalcDiscount A1 model: " + result.error());
    }
    return engine;
}

static BusinessRulesEngine createEngineForA2() {
    BusinessRulesEngine engine;
    auto result = engine.load_dmn_model(kCalcDiscountA2_DMN);
    if (!result) {
        throw std::runtime_error("Failed to load CalcDiscount A2 model: " + result.error());
    }
    return engine;
}

static BusinessRulesEngine createEngineForOrderDiscount() {
    BusinessRulesEngine engine;
    auto result = engine.load_dmn_model(kOrderDiscount_DMN);
    if (!result) {
        throw std::runtime_error("Failed to load OrderDiscount model: " + result.error());
    }
    return engine;
}

// Benchmark: calc-discount A.1 - UNIQUE hit policy (6 rules, 2 inputs)
static void BM_CalcDiscount_A1_Infant_NoPriority(benchmark::State& state) {
    auto engine = createEngineForA1();
    for (auto _ : state) {
        std::string result = engine.evaluate(kCalcDiscountA1_Input1);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_CalcDiscount_A1_Infant_Priority(benchmark::State& state) {
    auto engine = createEngineForA1();
    for (auto _ : state) {
        std::string result = engine.evaluate(kCalcDiscountA1_Input2);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_CalcDiscount_A1_Child_NoPriority(benchmark::State& state) {
    auto engine = createEngineForA1();
    for (auto _ : state) {
        std::string result = engine.evaluate(kCalcDiscountA1_Input3);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_CalcDiscount_A1_Child_Priority(benchmark::State& state) {
    auto engine = createEngineForA1();
    for (auto _ : state) {
        std::string result = engine.evaluate(kCalcDiscountA1_Input4);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_CalcDiscount_A1_Adult_NoPriority(benchmark::State& state) {
    auto engine = createEngineForA1();
    for (auto _ : state) {
        std::string result = engine.evaluate(kCalcDiscountA1_Input5);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_CalcDiscount_A1_Adult_Priority(benchmark::State& state) {
    auto engine = createEngineForA1();
    for (auto _ : state) {
        std::string result = engine.evaluate(kCalcDiscountA1_Input6);
        benchmark::DoNotOptimize(result);
    }
}

// Benchmark: calc-discount A.2 - COLLECT+SUM aggregation (4 rules)
static void BM_CalcDiscount_A2_CollectSum(benchmark::State& state) {
    auto engine = createEngineForA2();
    for (auto _ : state) {
        std::string result = engine.evaluate(kCalcDiscountA2_Input);
        benchmark::DoNotOptimize(result);
    }
}

// Benchmark: order-discount - Volume-based discount (5 rules, ranges)
static void BM_OrderDiscount_Small(benchmark::State& state) {
    auto engine = createEngineForOrderDiscount();
    for (auto _ : state) {
        std::string result = engine.evaluate(kOrderDiscount_Input1);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_OrderDiscount_Medium(benchmark::State& state) {
    auto engine = createEngineForOrderDiscount();
    for (auto _ : state) {
        std::string result = engine.evaluate(kOrderDiscount_Input2);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_OrderDiscount_Large(benchmark::State& state) {
    auto engine = createEngineForOrderDiscount();
    for (auto _ : state) {
        std::string result = engine.evaluate(kOrderDiscount_Input3);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_OrderDiscount_Larger(benchmark::State& state) {
    auto engine = createEngineForOrderDiscount();
    for (auto _ : state) {
        std::string result = engine.evaluate(kOrderDiscount_Input4);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_OrderDiscount_Largest(benchmark::State& state) {
    auto engine = createEngineForOrderDiscount();
    for (auto _ : state) {
        std::string result = engine.evaluate(kOrderDiscount_Input5);
        benchmark::DoNotOptimize(result);
    }
}

// ============================================================================
// ItemDefinition Validation Benchmarks (v1.2.0+)
// ============================================================================

static const char* kValidationSimple_DMN = R"(<?xml version="1.0"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/DMN15.xsd">
  <itemDefinition name="tStatus">
    <typeRef>string</typeRef>
    <allowedValues>
      <text>"Active", "Disabled", "Pending"</text>
    </allowedValues>
  </itemDefinition>
  <decision name="TestDecision">
    <decisionTable hitPolicy="UNIQUE">
      <input>
        <inputExpression><text>tStatus</text></inputExpression>
      </input>
      <output name="result"/>
      <rule>
        <inputEntry><text>-</text></inputEntry>
        <outputEntry><text>"OK"</text></outputEntry>
      </rule>
    </decisionTable>
  </decision>
</definitions>)";

static const char* kValidationComplex_DMN = R"(<?xml version="1.0"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/DMN15.xsd">
  <itemDefinition name="tAddress">
    <itemComponent name="street">
      <typeRef>string</typeRef>
    </itemComponent>
    <itemComponent name="city">
      <typeRef>string</typeRef>
    </itemComponent>
    <itemComponent name="zipCode">
      <typeRef>string</typeRef>
    </itemComponent>
  </itemDefinition>
  <itemDefinition name="tPerson">
    <itemComponent name="name">
      <typeRef>string</typeRef>
    </itemComponent>
    <itemComponent name="age">
      <typeRef>number</typeRef>
    </itemComponent>
    <itemComponent name="address">
      <typeRef>tAddress</typeRef>
    </itemComponent>
  </itemDefinition>
  <decision name="TestDecision">
    <decisionTable hitPolicy="UNIQUE">
      <input>
        <inputExpression><text>tPerson</text></inputExpression>
      </input>
      <output name="result"/>
      <rule>
        <inputEntry><text>-</text></inputEntry>
        <outputEntry><text>"OK"</text></outputEntry>
      </rule>
    </decisionTable>
  </decision>
</definitions>)";

static const char* kValidationDeep_DMN = R"(<?xml version="1.0"?>
<definitions xmlns="https://www.omg.org/spec/DMN/20230324/DMN15.xsd">
  <itemDefinition name="tAddress">
    <itemComponent name="street"><typeRef>string</typeRef></itemComponent>
    <itemComponent name="city"><typeRef>string</typeRef></itemComponent>
  </itemDefinition>
  <itemDefinition name="tMember">
    <itemComponent name="name"><typeRef>string</typeRef></itemComponent>
    <itemComponent name="address"><typeRef>tAddress</typeRef></itemComponent>
  </itemDefinition>
  <itemDefinition name="tTeam">
    <itemComponent name="teamName"><typeRef>string</typeRef></itemComponent>
    <itemComponent name="members">
      <typeRef>tMember</typeRef>
      <isCollection>true</isCollection>
    </itemComponent>
  </itemDefinition>
  <itemDefinition name="tDepartment">
    <itemComponent name="deptName"><typeRef>string</typeRef></itemComponent>
    <itemComponent name="teams">
      <typeRef>tTeam</typeRef>
      <isCollection>true</isCollection>
    </itemComponent>
  </itemDefinition>
  <itemDefinition name="tOrganization">
    <itemComponent name="orgName"><typeRef>string</typeRef></itemComponent>
    <itemComponent name="departments">
      <typeRef>tDepartment</typeRef>
      <isCollection>true</isCollection>
    </itemComponent>
  </itemDefinition>
  <decision name="TestDecision">
    <decisionTable hitPolicy="UNIQUE">
      <input>
        <inputExpression><text>tOrganization</text></inputExpression>
      </input>
      <output name="result"/>
      <rule>
        <inputEntry><text>-</text></inputEntry>
        <outputEntry><text>"OK"</text></outputEntry>
      </rule>
    </decisionTable>
  </decision>
</definitions>)";

// Benchmark: No validation (baseline)
static void BM_Validation_Baseline_NoValidation(benchmark::State& state) {
    BusinessRulesEngine engine;
    engine.load_dmn_model(kValidationSimple_DMN);
    engine.set_validation_enabled(false);
    std::string_view input = R"({"tStatus": "Active"})";
    for (auto _ : state) {
        std::string result = engine.evaluate(input);
        benchmark::DoNotOptimize(result);
    }
}

// Benchmark: Simple type validation (enum check)
static void BM_Validation_SimpleType_Valid(benchmark::State& state) {
    BusinessRulesEngine engine;
    engine.load_dmn_model(kValidationSimple_DMN);
    engine.set_validation_enabled(true);
    std::string_view input = R"({"tStatus": "Active"})";
    for (auto _ : state) {
        std::string result = engine.evaluate(input);
        benchmark::DoNotOptimize(result);
    }
}

// Benchmark: Complex type validation (2 levels)
static void BM_Validation_ComplexType_Valid(benchmark::State& state) {
    BusinessRulesEngine engine;
    engine.load_dmn_model(kValidationComplex_DMN);
    engine.set_validation_enabled(true);
    std::string_view input = R"({
        "tPerson": {
            "name": "Alice",
            "age": 30,
            "address": {
                "street": "123 Main St",
                "city": "Seattle",
                "zipCode": "98101"
            }
        }
    })";
    for (auto _ : state) {
        std::string result = engine.evaluate(input);
        benchmark::DoNotOptimize(result);
    }
}

// Benchmark: Deep nesting validation (5 levels)
static void BM_Validation_DeepNesting_Valid(benchmark::State& state) {
    BusinessRulesEngine engine;
    engine.load_dmn_model(kValidationDeep_DMN);
    engine.set_validation_enabled(true);
    std::string_view input = R"({
        "tOrganization": {
            "orgName": "Acme Corp",
            "departments": [{
                "deptName": "Engineering",
                "teams": [{
                    "teamName": "Backend",
                    "members": [{
                        "name": "Alice",
                        "address": {"street": "123 Main", "city": "Seattle"}
                    }]
                }]
            }]
        }
    })";
    for (auto _ : state) {
        std::string result = engine.evaluate(input);
        benchmark::DoNotOptimize(result);
    }
}

// Benchmark: Collection validation (10 items)
static void BM_Validation_Collection_10Items(benchmark::State& state) {
    BusinessRulesEngine engine;
    engine.load_dmn_model(kValidationDeep_DMN);
    engine.set_validation_enabled(true);
    std::string_view input = R"({
        "tOrganization": {
            "orgName": "Acme",
            "departments": [{
                "deptName": "Eng",
                "teams": [{
                    "teamName": "Backend",
                    "members": [
                        {"name":"Alice","address":{"street":"1","city":"S"}},
                        {"name":"Bob","address":{"street":"2","city":"S"}},
                        {"name":"Charlie","address":{"street":"3","city":"S"}},
                        {"name":"Diana","address":{"street":"4","city":"S"}},
                        {"name":"Eve","address":{"street":"5","city":"S"}},
                        {"name":"Frank","address":{"street":"6","city":"S"}},
                        {"name":"Grace","address":{"street":"7","city":"S"}},
                        {"name":"Hank","address":{"street":"8","city":"S"}},
                        {"name":"Ivy","address":{"street":"9","city":"S"}},
                        {"name":"Jack","address":{"street":"10","city":"S"}}
                    ]
                }]
            }]
        }
    })";
    for (auto _ : state) {
        std::string result = engine.evaluate(input);
        benchmark::DoNotOptimize(result);
    }
}

// ============================================================================
// Register Benchmarks
// ============================================================================

// calc-discount A.1 benchmarks
BENCHMARK(BM_CalcDiscount_A1_Infant_NoPriority);
BENCHMARK(BM_CalcDiscount_A1_Infant_Priority);
BENCHMARK(BM_CalcDiscount_A1_Child_NoPriority);
BENCHMARK(BM_CalcDiscount_A1_Child_Priority);
BENCHMARK(BM_CalcDiscount_A1_Adult_NoPriority);
BENCHMARK(BM_CalcDiscount_A1_Adult_Priority);

// calc-discount A.2 benchmark
BENCHMARK(BM_CalcDiscount_A2_CollectSum);

// order-discount benchmarks
BENCHMARK(BM_OrderDiscount_Small);
BENCHMARK(BM_OrderDiscount_Medium);
BENCHMARK(BM_OrderDiscount_Large);
BENCHMARK(BM_OrderDiscount_Larger);
BENCHMARK(BM_OrderDiscount_Largest);

// validation benchmarks (v1.2.0+)
BENCHMARK(BM_Validation_Baseline_NoValidation);
BENCHMARK(BM_Validation_SimpleType_Valid);
BENCHMARK(BM_Validation_ComplexType_Valid);
BENCHMARK(BM_Validation_DeepNesting_Valid);
BENCHMARK(BM_Validation_Collection_10Items);

int main(int argc, char** argv) {
    // Initialize logger for benchmarks (use null logger to avoid overhead)
    orion::api::Logger::instance().set_logger(std::make_shared<orion::api::SpdlogLogger>());
    
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}
