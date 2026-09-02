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

// ORION_HAS_STD_TZDB is set by CMake after probing for std::chrono::locate_zone. libstdc++
// advertises __cpp_lib_chrono without shipping a time zone database, so a compile probe is
// the only reliable detection. Without it, named zones resolve to an unknown offset.
#ifdef ORION_HAS_STD_TZDB
#include <chrono>
#endif

namespace orion::bre::feel {
    // Helper to parse integer from string_view without allocation
    inline int parse_int(std::string_view sv) {
        int value = 0;
        std::from_chars(sv.data(), sv.data() + sv.size(), value);
        return value;
    }

    std::optional<int> named_timezone_offset_seconds(std::string_view tz_name,
                                                     const Date& date,
                                                     const Time& time)
    {
#ifdef ORION_HAS_STD_TZDB
        // std::chrono::year only models a limited range; FEEL allows far wider years.
        if (date.y < static_cast<int>(std::chrono::year::min()) ||
            date.y > static_cast<int>(std::chrono::year::max()))
        {
            return std::nullopt;
        }

        try
        {
            const std::chrono::time_zone* zone = std::chrono::locate_zone(std::string(tz_name));
            if (zone == nullptr) return std::nullopt;

            const std::chrono::year_month_day ymd{
                std::chrono::year{date.y},
                std::chrono::month{static_cast<unsigned>(date.m)},
                std::chrono::day{static_cast<unsigned>(date.d)}};
            if (!ymd.ok()) return std::nullopt;

            const std::chrono::local_seconds local_tp =
                std::chrono::local_days{ymd} + std::chrono::hours{time.h} +
                std::chrono::minutes{time.m} + std::chrono::seconds{time.s};

            const std::chrono::local_info info = zone->get_info(local_tp);
            // For ambiguous/nonexistent local times, use the offset in effect before the transition.
            return static_cast<int>(info.first.offset.count());
        }
        catch (const std::exception&)
        {
            return std::nullopt; // unknown zone name or no time zone database available
        }
#else
        (void)tz_name;
        (void)date;
        (void)time;
        return std::nullopt;
#endif
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
        // Find 'T' separator
        auto tpos = str.find('T');
        if (tpos == std::string_view::npos) return std::nullopt;
        
        auto date_part = str.substr(0, tpos);
        auto time_and_tz = str.substr(tpos + 1);
        
        // Parse date part (handles negative years, 4+ digit years)
        auto date_opt = parse_date(date_part);
        if (!date_opt) return std::nullopt;
        
        // Parse time part - extract HH:MM:SS (ignore fractional seconds and timezone for now)
        int h = 0, m = 0, s = 0;
        size_t pos = 0;
        // Hours
        if (pos + 2 > time_and_tz.size()) return std::nullopt;
        h = parse_int(time_and_tz.substr(pos, 2));
        pos += 2;
        if (pos >= time_and_tz.size() || time_and_tz[pos] != ':') return std::nullopt;
        pos++;
        // Minutes
        if (pos + 2 > time_and_tz.size()) return std::nullopt;
        m = parse_int(time_and_tz.substr(pos, 2));
        pos += 2;
        if (pos >= time_and_tz.size() || time_and_tz[pos] != ':') return std::nullopt;
        pos++;
        // Seconds
        if (pos + 2 > time_and_tz.size()) return std::nullopt;
        s = parse_int(time_and_tz.substr(pos, 2));
        pos += 2;
        // Skip fractional seconds
        if (pos < time_and_tz.size() && time_and_tz[pos] == '.') {
            pos++;
            while (pos < time_and_tz.size() && std::isdigit(static_cast<unsigned char>(time_and_tz[pos])))
                pos++;
        }
        
        // Parse timezone offset for adjustment
        int tz_offset_seconds = 0;
        bool has_tz = false;
        std::string_view named_zone;
        if (pos < time_and_tz.size()) {
            if (time_and_tz[pos] == 'Z') {
                tz_offset_seconds = 0;
                has_tz = true;
            } else if (time_and_tz[pos] == '+' || time_and_tz[pos] == '-') {
                has_tz = true;
                bool neg = (time_and_tz[pos] == '-');
                pos++;
                if (pos + 2 > time_and_tz.size()) return std::nullopt;
                int tzh = parse_int(time_and_tz.substr(pos, 2));
                pos += 2;
                int tzm = 0;
                if (pos < time_and_tz.size() && time_and_tz[pos] == ':') {
                    pos++;
                    if (pos + 2 <= time_and_tz.size())
                        tzm = parse_int(time_and_tz.substr(pos, 2));
                }
                tz_offset_seconds = (tzh * 3600 + tzm * 60) * (neg ? -1 : 1);
            } else if (time_and_tz[pos] == '@') {
                has_tz = true; // named timezone
                const auto tz_name = time_and_tz.substr(pos + 1);
                if (tz_name.empty()) return std::nullopt;
                named_zone = tz_name;
            }
        }
        
        Date date{date_opt->y, date_opt->m, date_opt->d};
        Time time_val{h, m, s};
        if (!named_zone.empty())
        {
            tz_offset_seconds = named_timezone_offset_seconds(named_zone, date, time_val).value_or(0);
        }
        DateTime dt{date, time_val};
        dt.tz_offset_seconds = tz_offset_seconds;
        dt.has_tz = has_tz;
        return dt;
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

            // Handle fractional seconds (e.g., PT0.999S)
            if (current_char == '.' && in_time)
            {
                // Skip fractional digits until we hit 'S'
                ++i;
                while (i < str.size() && std::isdigit(static_cast<unsigned char>(str[i])))
                {
                    ++i;
                }
                // Now str[i] should be 'S'
                if (i < str.size() && str[i] == 'S')
                {
                    // Treat as whole seconds (fractional part ignored for now in total_seconds)
                    // But mark that we had a number
                    if (!flush('S'))
                    {
                        return std::nullopt;
                    }
                }
                else
                {
                    return std::nullopt; // Fractional without 'S'
                }
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
