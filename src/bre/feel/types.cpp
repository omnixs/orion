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
#include <ctre.hpp>
#include <climits>  // For INT_MAX
#include <charconv> // For std::from_chars

namespace orion::bre::feel {
    // Helper to parse integer from string_view without allocation
    inline int parse_int(std::string_view sv) {
        int value = 0;
        std::from_chars(sv.data(), sv.data() + sv.size(), value);
        return value;
    }
    std::optional<Date> parse_date(std::string_view str)
    {
        // CTRE compile-time regex for date pattern (with optional negative year)
        if (auto match = ctre::match<R"(-?(\d{4,})-(\d{2})-(\d{2}))">(str))
        {
            Date date;
            // Parse year including potential negative sign
            auto year_str = std::string(match.get<1>().to_view());
            date.y = parse_int(year_str);
            if (str[0] == '-') date.y = -date.y;
            date.m = parse_int(match.get<2>().to_view());
            date.d = parse_int(match.get<3>().to_view());
            return date;
        }
        return std::nullopt;
    }

    std::optional<Time> parse_time(std::string_view str)
    {
        // CTRE compile-time regex for time patterns
        if (auto match = ctre::match<R"((\d{2}):(\d{2}):(\d{2}))">(str))
        {
            return Time{parse_int(match.get<1>().to_view()), parse_int(match.get<2>().to_view()), parse_int(match.get<3>().to_view())};
        }
        if (auto match = ctre::match<R"((\d{2}):(\d{2}))">(str))
        {
            return Time{parse_int(match.get<1>().to_view()), parse_int(match.get<2>().to_view()), 0};
        }
        return std::nullopt;
    }

    std::optional<DateTime> parse_datetime(std::string_view str)
    {
        // CTRE compile-time regex for datetime pattern
        if (auto match = ctre::match<R"((\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2}))">(str))
        {
            Date date{parse_int(match.get<1>().to_view()), parse_int(match.get<2>().to_view()), parse_int(match.get<3>().to_view())};
            Time time_val{parse_int(match.get<4>().to_view()), parse_int(match.get<5>().to_view()), parse_int(match.get<6>().to_view())};
            return DateTime{date, time_val};
        }
        return std::nullopt;
    }

    std::optional<Duration> parse_duration(std::string_view str)
    {
        if (str.empty())
        {
            return std::nullopt;
        }
        
        bool negative = false;
        if (str[0] == '-')
        {
            negative = true;
            str = str.substr(1);
        }
        
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

        if (negative)
        {
            duration.total_months = -duration.total_months;
            duration.total_seconds = -duration.total_seconds;
        }

        return duration;
    }
}
