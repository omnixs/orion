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

#include <cstdint>

namespace orion::api
{
    /**
     * @brief Hit policy for decision tables
     * 
     * Determines how multiple matching rules are handled in decision table evaluation.
     * 
     * @see DMN 1.5 Specification Section 8.2.8 "Hit policy"
     * @see DMN 1.5 Specification Section 8.3.4 "Hit policy and result aggregation"
     */
    enum class HitPolicy : std::uint8_t
    {
        UNIQUE,     // Exactly one rule can match (default for most cases)
        FIRST,      // Return result of first matching rule
        PRIORITY,   // Return result of rule with highest priority
        ANY,        // All matching rules must have same output
        COLLECT,    // Collect all outputs (requires aggregation)
        RULE_ORDER, // Return results in rule definition order
        OUTPUT_ORDER // Return results in output value priority order
    };

    /**
     * @brief Aggregation function for COLLECT hit policy
     * 
     * Specifies how to aggregate multiple outputs when using COLLECT hit policy.
     * Only applicable when HitPolicy is COLLECT.
     * 
     * @see DMN 1.5 Specification Section 8.2.8 "Hit policy"
     * @see DMN 1.5 Specification Section 8.3.4 "Hit policy and result aggregation"
     */
    enum class CollectAggregation : std::uint8_t
    {
        NONE,  // No aggregation (return list of results)
        SUM,   // Sum of numeric results
        COUNT, // Count of results
        MIN,   // Minimum value
        MAX    // Maximum value
    };

} // namespace orion::api