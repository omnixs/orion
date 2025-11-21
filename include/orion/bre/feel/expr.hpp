#pragma once
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace orion::bre::feel {
    bool eval_feel_literal(std::string_view expr, const nlohmann::json& ctx, nlohmann::json& out, std::string& err);
}
