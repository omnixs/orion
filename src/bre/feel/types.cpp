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

#include <orion/bre/feel/types.hpp>
#include <regex>
#include <climits>  // For INT_MAX

namespace orion::bre::feel {
    std::optional<Date> parse_date(std::string_view str)
    {
        static const std::regex regex_pattern(R"((\d{4})-(\d{2})-(\d{2}))");
        std::smatch match;
        std::string str_copy(str);  // regex requires std::string
        if (!std::regex_match(str_copy, match, regex_pattern))
        {
            return std::nullopt;
        }
        Date date;
        date.y = std::stoi(match[1]);
        date.m = std::stoi(match[2]);
        date.d = std::stoi(match[3]);
        return date;
    }

    std::optional<Time> parse_time(std::string_view str)
    {
        static const std::regex regex_time_full(R"((\d{2}):(\d{2}):(\d{2}))");
        static const std::regex regex_time_short(R"((\d{2}):(\d{2}))");
        std::smatch match;
        std::string str_copy(str);  // regex requires std::string
        if (std::regex_match(str_copy, match, regex_time_full))
        {
            return Time{std::stoi(match[1]), std::stoi(match[2]), std::stoi(match[3])};
        }
        if (std::regex_match(str_copy, match, regex_time_short))
        {
            return Time{std::stoi(match[1]), std::stoi(match[2]), 0};
        }
        return std::nullopt;
    }

    std::optional<DateTime> parse_datetime(std::string_view str)
    {
        static const std::regex regex_pattern(R"((\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2}))");
        std::smatch match;
        std::string str_copy(str);  // regex requires std::string
        if (!std::regex_match(str_copy, match, regex_pattern))
        {
            return std::nullopt;
        }
        Date date{std::stoi(match[1]), std::stoi(match[2]), std::stoi(match[3])};
        Time time_val{std::stoi(match[4]), std::stoi(match[5]), std::stoi(match[6])};
        return DateTime{date, time_val};
    }

    std::optional<Duration> parse_duration(std::string_view str)
    {
        if (str.empty() || str[0] != 'P')
        {
            return std::nullopt;
        }

        int years = 0;
        int months = 0;
        int days = 0;
        int hours = 0;
        int minutes = 0;
        int seconds = 0;
        bool in_time = false;

        int num = 0;
        bool have_num = false;

        auto flush = [&](char unit) -> bool
        {
            if (!have_num)
            {
                return false; // e.g., "PTM" is invalid
            }
            switch (unit)
            {
            case 'Y': years = num;
                break;
            case 'M':
                if (in_time)
                {
                    minutes = num;
                }
                else
                {
                    months = num;
                }
                break;
            case 'D': days = num;
                break;
            case 'H': hours = num;
                break;
            case 'S': seconds = num;
                break;
            default: return false;
            }
            num = 0;
            have_num = false;
            return true;
        };

        for (size_t i = 1; i < str.size(); ++i)
        {
            char current_char = str[i];
            if (current_char == 'T')
            {
                in_time = true;
                continue;
            }

            if (std::isdigit(static_cast<unsigned char>(current_char)) != 0)
            {
                have_num = true;
                int digit = current_char - '0';
                // (optional) overflow guard for extremely large inputs
                if (num > (INT_MAX - digit) / 10)
                {
                    return std::nullopt;
                }
                num = num * 10 + digit; // <-- was "num10 + ..." before
                continue;
            }

            if (!flush(current_char))
            {
                return std::nullopt; // expects Y/M/D/H/S
            }
        }

        if (have_num)
        {
            return std::nullopt; // dangling number without unit
        }

        Duration duration;
        duration.total_months = years * 12 + months; // <-- was "years12 + months" before

        long long total_sec = 0;
        total_sec += static_cast<long long>(days) * 24LL * 3600LL;
        total_sec += static_cast<long long>(hours) * 3600LL;
        total_sec += static_cast<long long>(minutes) * 60LL;
        total_sec += seconds;
        duration.total_seconds = total_sec;

        return duration;
    }
}
