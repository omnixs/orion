#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>

namespace orion::bre
{
    /**
     * @brief Business Knowledge Model implementation with contract enforcement
     */
    struct BusinessKnowledgeModel
    {
        std::string name;
        std::vector<std::string> parameters;
        std::string expression_text;

        /**
         * @brief Invoke the BKM with given arguments
         * @param args Resolved argument values
         * @param context JSON context for evaluation
         * @param available_bkms Map of available BKMs for recursive calls
         * @return Result of BKM evaluation
         * @throws contract_violation if preconditions are violated
         */
        [[nodiscard]] nlohmann::json invoke(const std::vector<nlohmann::json>& args,
                              const nlohmann::json& context,
                              const std::map<std::string, BusinessKnowledgeModel>& available_bkms) const;

        /**
         * @brief Validate BKM structure
         * @return true if BKM is valid, false otherwise
         */
        [[nodiscard]] bool is_valid() const noexcept
        {
            return !name.empty() && !expression_text.empty();
        }
    };
} // namespace orion::bre
