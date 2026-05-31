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

#include <map>
#include <string>

namespace orion::bre::feel {
    class RegexCache; // Forward declaration
}

namespace orion::bre
{
    struct BusinessKnowledgeModel; // Forward declaration

    /**
     * @brief Evaluation context containing engine resources and runtime state
     * 
     * This structure holds resources and configuration needed during expression
     * and decision evaluation. Currently contains only regex caching, but designed
     * as an extensible container for future evaluation-time context.
     * 
     * @note Non-owning reference semantics - the context does not own the resources
     *       it references. The engine instance owns the actual RegexCache.
     */
    struct EvaluationContext
    {
        feel::RegexCache& regex_cache; ///< Non-owning reference to engine's regex cache
        const std::map<std::string, const BusinessKnowledgeModel*, std::less<>>* bkm_map = nullptr;

        /**
         * @brief Construct evaluation context with required resources
         * @param cache Reference to engine's regex cache (must outlive this context)
         */
        explicit EvaluationContext(feel::RegexCache& cache) : regex_cache(cache) {}
    };
}
