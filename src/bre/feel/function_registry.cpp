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

#include <orion/bre/feel/function_registry.hpp>
#include <algorithm>
#include <array>

namespace orion::bre::feel {

namespace {

// Formal parameter lists, based on DMN 1.5 specification (formal-24-01-01.txt)
constexpr std::array<FormalParameter, 1> kN{{{"n"}}};
constexpr std::array<FormalParameter, 1> kNumber{{{"number"}}};
constexpr std::array<FormalParameter, 2> kDividendDivisor{{{"dividend"}, {"divisor"}}};
constexpr std::array<FormalParameter, 2> kNScale{{{"n"}, {"scale"}}};
constexpr std::array<FormalParameter, 1> kString{{{"string"}}};
constexpr std::array<FormalParameter, 2> kStringMatch{{{"string"}, {"match"}}};
constexpr std::array<FormalParameter, 3> kSubstring{{{"string"}, {"start position"}, {"length", true}}};
constexpr std::array<FormalParameter, 4> kReplace{{{"input"}, {"pattern"}, {"replacement"}, {"flags", true}}};
constexpr std::array<FormalParameter, 3> kMatches{{{"input"}, {"pattern"}, {"flags", true}}};
constexpr std::array<FormalParameter, 2> kSplit{{{"string"}, {"delimiter"}}};
constexpr std::array<FormalParameter, 2> kStringJoin{{{"list"}, {"delimiter", true}}};
constexpr std::array<FormalParameter, 1> kList{{{"list"}}};
constexpr std::array<FormalParameter, 2> kListElement{{{"list"}, {"element"}}};
constexpr std::array<FormalParameter, 2> kListMatch{{{"list"}, {"match"}}};
constexpr std::array<FormalParameter, 2> kListPosition{{{"list"}, {"position"}}};
constexpr std::array<FormalParameter, 3> kListPositionNewItem{{{"list"}, {"position"}, {"newItem"}}};
constexpr std::array<FormalParameter, 3> kSublist{{{"list"}, {"start position"}, {"length", true}}};
constexpr std::array<FormalParameter, 2> kSort{{{"list"}, {"precedes"}}};
constexpr std::array<FormalParameter, 1> kFrom{{{"from"}}};
constexpr std::array<FormalParameter, 2> kFromTo{{{"from"}, {"to"}}};
constexpr std::array<FormalParameter, 3> kNumberFrom{
    {{"from"}, {"grouping separator", true}, {"decimal separator", true}}};
constexpr std::array<FormalParameter, 1> kDate{{{"date"}}};
constexpr std::array<FormalParameter, 1> kNegand{{{"negand"}}};
constexpr std::array<FormalParameter, 2> kMKey{{{"m"}, {"key"}}};
constexpr std::array<FormalParameter, 1> kM{{{"m"}}};
constexpr std::array<FormalParameter, 1> kEntries{{{"entries"}}};
constexpr std::array<FormalParameter, 3> kContextPut{{{"context"}, {"key"}, {"value"}}};
constexpr std::array<FormalParameter, 1> kContexts{{{"contexts"}}};
constexpr std::array<FormalParameter, 2> kValue1Value2{{{"value1"}, {"value2"}}};
constexpr std::array<FormalParameter, 2> kPoint1Point2{{{"point1"}, {"point2"}}};
constexpr std::array<FormalParameter, 2> kRange1Range2{{{"range1"}, {"range2"}}};
constexpr std::array<FormalParameter, 2> kPointRange{{{"point"}, {"range"}}};
constexpr std::array<FormalParameter, 2> kRangePoint{{{"range"}, {"point"}}};

// Built at compile time: no heap allocation and no lazy (first-call) initialization.
constexpr auto make_signature_table()
{
    auto table = std::to_array<FunctionSignature>({
        // Numeric functions
        {"abs", kN},
        {"floor", kN},
        {"ceiling", kN},
        {"sqrt", kNumber},
        {"exp", kNumber},
        {"log", kNumber},
        {"odd", kNumber},
        {"even", kNumber},
        {"modulo", kDividendDivisor},
        {"decimal", kNScale},
        {"round", kNScale},
        {"round up", kNScale},
        {"round down", kNScale},
        {"round half up", kNScale},
        {"round half down", kNScale},

        // String functions
        {"substring", kSubstring},
        {"string length", kString},
        {"upper case", kString},
        {"lower case", kString},
        {"substring before", kStringMatch},
        {"substring after", kStringMatch},
        {"contains", kStringMatch},
        {"starts with", kStringMatch},
        {"ends with", kStringMatch},
        {"replace", kReplace},
        {"matches", kMatches},
        {"split", kSplit},
        {"string join", kStringJoin},

        // List functions
        {"list contains", kListElement},
        {"count", kList},
        {"min", kList, true},
        {"max", kList, true},
        {"sum", kList, true},
        {"mean", kList, true},
        {"all", kList, true},
        {"any", kList, true},
        {"sublist", kSublist},
        {"append", kList, true},
        {"concatenate", kList, true},
        {"insert before", kListPositionNewItem},
        {"remove", kListPosition},
        {"reverse", kList},
        {"index of", kListMatch},
        {"union", kList, true},
        {"distinct values", kList},
        {"flatten", kList},
        {"product", kList, true},
        {"median", kList, true},
        {"stddev", kList, true},
        {"mode", kList, true},
        {"list replace", kListPositionNewItem},

        // Date/time conversion functions. The date/time constructors are NOT
        // registered - they use fallback positional binding because they have
        // multiple overloaded signatures with different parameter names.
        {"duration", kFrom},
        {"number", kNumberFrom},
        {"string", kFrom},
        {"years and months duration", kFromTo},

        // Temporal functions
        {"day of year", kDate},
        {"day of week", kDate},
        {"month of year", kDate},
        {"week of year", kDate},
        {"now", {}},
        {"today", {}},

        // Boolean, context and miscellaneous functions
        {"not", kNegand},
        {"get value", kMKey},
        {"get entries", kM},
        {"context", kEntries},
        {"context put", kContextPut},
        {"context merge", kContexts},
        {"sort", kSort},
        {"is", kValue1Value2},

        // Range functions
        {"before", kPoint1Point2},
        {"after", kPoint1Point2},
        {"meets", kRange1Range2},
        {"met by", kRange1Range2},
        {"overlaps", kRange1Range2},
        {"overlaps before", kRange1Range2},
        {"overlaps after", kRange1Range2},
        {"finishes", kPointRange},
        {"finished by", kRangePoint},
        {"includes", kRangePoint},
        {"during", kPointRange},
        {"starts", kPointRange},
        {"started by", kRangePoint},
        {"coincides", kPoint1Point2},
    });
    std::ranges::sort(table, {}, &FunctionSignature::name);
    return table;
}

constexpr auto signature_table = make_signature_table();
static_assert(std::ranges::adjacent_find(signature_table, {}, &FunctionSignature::name) == signature_table.end(),
              "duplicate function signature name");

} // namespace

const FunctionRegistry& FunctionRegistry::instance() noexcept {
    static constexpr FunctionRegistry registry{};
    return registry;
}

const FunctionSignature* FunctionRegistry::get_signature(
    std::string_view name) const noexcept
{
    const auto iter = std::ranges::lower_bound(signature_table, name, {}, &FunctionSignature::name);
    return (iter != signature_table.end() && iter->name == name) ? &*iter : nullptr;
}

} // namespace orion::bre::feel

