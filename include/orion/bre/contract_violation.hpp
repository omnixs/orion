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

#include <stdexcept>
#include <string>

namespace orion::bre
{
    /**
     * @brief Exception thrown when programming contract violations are detected
     * 
     * This exception indicates a bug in the code (programming error) rather than
     * a business logic or user input error. Unlike std::logic_error, this can be
     * caught and handled gracefully in production to avoid termination.
     * 
     * Examples of contract violations:
     * - Null pointer passed where non-null expected
     * - Array index out of bounds  
     * - Invalid state transitions
     * - Precondition/postcondition failures
     * 
     * @note This should NEVER happen in correct code, but production systems
     *       should handle it gracefully rather than terminate.
     */
    class ContractViolation : public std::logic_error
    {
    public:
        explicit ContractViolation(const std::string& message)
            : std::logic_error("Contract violation: " + message)
        {
        }

        explicit ContractViolation(const char* message)
            : std::logic_error("Contract violation: " + std::string(message))
        {
        }

        /**
         * @brief Create contract violation with source location info
         * @param message Error description
         * @param function Function name where violation occurred
         * @param file Source file name
         * @param line Line number
         */
        ContractViolation(const std::string& message,
                           const char* function,
                           const char* file,
                           int line)
            : std::logic_error("Contract violation in " + std::string(function) +
                " (" + file + ":" + std::to_string(line) + "): " + message)
        {
        }
    };

    /**
     * @brief Macro to throw ContractViolation with source location
     * @note Macro required to capture __FUNCTION__, __FILE__, __LINE__ at call site
     * 
     * Usage:
     *   if (ptr == nullptr) [[unlikely]] {
     *       THROW_CONTRACT_VIOLATION("Pointer cannot be null");
     *   }
     */
#define THROW_CONTRACT_VIOLATION(msg) /* NOLINT(cppcoreguidelines-macro-usage) */ \
    throw ::orion::bre::ContractViolation((msg), __FUNCTION__, __FILE__, __LINE__)

} // namespace orion::bre
