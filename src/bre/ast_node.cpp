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

#include "orion/bre/ast_node.hpp"
#include <orion/bre/business_knowledge_model.hpp>
#include <orion/bre/feel/functions.hpp>
#include <orion/bre/feel/parameter_binder.hpp>
#include <orion/bre/feel/types.hpp>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace orion::bre
{
    namespace
    {
        // Duration arithmetic helpers
        bool is_duration_string(const json& val)
        {
            if (!val.is_string()) return false;
            auto s = val.get<std::string>();
            if (s.empty()) return false;
            return s[0] == 'P' || (s[0] == '-' && s.size() > 1 && s[1] == 'P');
        }
        
        std::string format_duration(const feel::Duration& d, bool is_ym = false)
        {
            bool negative = (d.total_months < 0 || d.total_seconds < 0);
            int months = std::abs(d.total_months);
            long long secs = std::abs(d.total_seconds);
            
            std::string result;
            if (negative) result += "-";
            result += "P";
            
            if (months > 0 || is_ym)
            {
                int y = months / 12;
                int m = months % 12;
                if (y > 0) result += std::to_string(y) + "Y";
                if (m > 0) result += std::to_string(m) + "M";
                if (y == 0 && m == 0) result += "0M";
            }
            else
            {
                long long days = secs / 86400;
                long long rem = secs % 86400;
                long long hrs = rem / 3600;
                rem %= 3600;
                long long mins = rem / 60;
                long long s = rem % 60;
                
                if (days > 0) result += std::to_string(days) + "D";
                if (hrs > 0 || mins > 0 || s > 0)
                {
                    result += "T";
                    if (hrs > 0) result += std::to_string(hrs) + "H";
                    if (mins > 0) result += std::to_string(mins) + "M";
                    if (s > 0) result += std::to_string(s) + "S";
                }
                else if (days == 0)
                {
                    result += "0D";
                }
            }
            return result;
        }

        bool is_date_string(const std::string& s)
        {
            // [-]YYYY-MM-DD
            size_t offset = (s.size() > 0 && s[0] == '-') ? 1 : 0;
            return s.size() >= offset + 10 && s[offset + 4] == '-' && s[offset + 7] == '-' && s.find('T') == std::string::npos;
        }
        
        bool is_datetime_string(const std::string& s)
        {
            size_t offset = (s.size() > 0 && s[0] == '-') ? 1 : 0;
            return s.find('T') != std::string::npos && s.size() >= offset + 19 && s[offset + 4] == '-';
        }
        
        bool is_time_string(const std::string& s)
        {
            // HH:MM:SS
            return s.size() >= 8 && s[2] == ':' && s[5] == ':' && s[0] != 'P' && s[0] != '-';
        }
        
        int days_in_month(int y, int m)
        {
            static const int days[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
            if (m == 2 && ((y%4==0 && y%100!=0) || y%400==0)) return 29;
            return days[m];
        }
        
        std::string add_duration_to_date(const std::string& date_str, const feel::Duration& dur, bool subtract = false)
        {
            auto date = feel::parse_date(date_str);
            if (!date) return {};
            
            int months = subtract ? -dur.total_months : dur.total_months;
            long long secs = subtract ? -dur.total_seconds : dur.total_seconds;
            
            // Add months
            int y = date->y, m = date->m, d = date->d;
            m += months;
            while (m > 12) { m -= 12; y++; }
            while (m < 1) { m += 12; y--; }
            // Clamp day
            int max_d = days_in_month(y, m);
            if (d > max_d) d = max_d;
            
            // Add days from seconds (for dates, any partial day counts as a full day in that direction)
            long long total_days = secs / 86400;
            long long remainder = secs % 86400;
            if (remainder < 0) { total_days--; }  // e.g., -3600 secs = -1 day (floor division)
            d += static_cast<int>(total_days);
            while (d > days_in_month(y, m)) { d -= days_in_month(y, m); m++; if (m > 12) { m = 1; y++; } }
            while (d < 1) { m--; if (m < 1) { m = 12; y--; } d += days_in_month(y, m); }
            
            char buf[32];
            if (y < 0) {
                snprintf(buf, sizeof(buf), "-%04d-%02d-%02d", -y, m, d);
            } else {
                snprintf(buf, sizeof(buf), "%04d-%02d-%02d", y, m, d);
            }
            return buf;
        }
        
        std::string add_duration_to_datetime(const std::string& dt_str, const feel::Duration& dur, bool subtract = false)
        {
            // Split at T
            auto t_pos = dt_str.find('T');
            if (t_pos == std::string::npos) return {};
            std::string date_part = dt_str.substr(0, t_pos);
            std::string time_suffix = dt_str.substr(t_pos); // includes T
            
            auto date = feel::parse_date(date_part);
            if (!date) return {};
            
            int months = subtract ? -dur.total_months : dur.total_months;
            long long secs = subtract ? -dur.total_seconds : dur.total_seconds;
            
            // Parse time part (after T)
            int h = 0, mi = 0, s = 0;
            std::string tz_suffix;
            if (time_suffix.size() >= 9) {
                h = std::stoi(time_suffix.substr(1, 2));
                mi = std::stoi(time_suffix.substr(4, 2));
                s = std::stoi(time_suffix.substr(7, 2));
                if (time_suffix.size() > 9) tz_suffix = time_suffix.substr(9);
            }
            
            // Add months to date
            int y = date->y, mo = date->m, d = date->d;
            mo += months;
            while (mo > 12) { mo -= 12; y++; }
            while (mo < 1) { mo += 12; y--; }
            int max_d = days_in_month(y, mo);
            if (d > max_d) d = max_d;
            
            // Add seconds to time
            long long total_secs = h * 3600LL + mi * 60LL + s + secs;
            long long day_offset = 0;
            if (total_secs < 0) {
                day_offset = (total_secs - 86399) / 86400;
                total_secs -= day_offset * 86400;
            } else {
                day_offset = total_secs / 86400;
                total_secs %= 86400;
            }
            h = static_cast<int>(total_secs / 3600);
            mi = static_cast<int>((total_secs % 3600) / 60);
            s = static_cast<int>(total_secs % 60);
            
            d += static_cast<int>(day_offset);
            while (d > days_in_month(y, mo)) { d -= days_in_month(y, mo); mo++; if (mo > 12) { mo = 1; y++; } }
            while (d < 1) { mo--; if (mo < 1) { mo = 12; y--; } d += days_in_month(y, mo); }
            
            char buf[64];
            if (y < 0) {
                snprintf(buf, sizeof(buf), "-%04d-%02d-%02dT%02d:%02d:%02d", -y, mo, d, h, mi, s);
            } else {
                snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d", y, mo, d, h, mi, s);
            }
            return std::string(buf) + tz_suffix;
        }
        
        std::string add_duration_to_time(const std::string& time_str, const feel::Duration& dur, bool subtract = false)
        {
            // Parse HH:MM:SS[+offset]
            if (time_str.size() < 8) return {};
            int h = std::stoi(time_str.substr(0, 2));
            int m = std::stoi(time_str.substr(3, 2));
            int s = std::stoi(time_str.substr(6, 2));
            std::string suffix;
            if (time_str.size() > 8) suffix = time_str.substr(8);
            
            long long secs = subtract ? -dur.total_seconds : dur.total_seconds;
            long long total = h * 3600LL + m * 60LL + s + secs;
            // Wrap around 24h
            total = ((total % 86400) + 86400) % 86400;
            h = static_cast<int>(total / 3600);
            m = static_cast<int>((total % 3600) / 60);
            s = static_cast<int>(total % 60);
            
            char buf[32];
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
            return std::string(buf) + suffix;
        }

        /**
         * @brief Resolve a variable from context with multiple naming variants
         * 
         * Tries multiple variations of the variable name to handle DMN naming flexibility:
         * - Exact match
         * - With underscores instead of spaces
         * - Lowercase
         * - Lowercase with underscores
         * - No spaces
         */
        json resolveVariable(std::string_view name, const json& context)
        {
            // Try exact match first (hot path — avoids all string allocations)
            if (auto it = context.find(name); it != context.end())
            {
                return *it;
            }
            
            // Try with underscores instead of spaces (only if name contains spaces)
            if (name.find(' ') != std::string_view::npos)
            {
                std::string underscored(name);
                std::replace(underscored.begin(), underscored.end(), ' ', '_');
                if (auto it = context.find(underscored); it != context.end())
                {
                    return *it;
                }
                
                // Try lowercase with underscores
                std::string lower_us = underscored;
                std::transform(lower_us.begin(), lower_us.end(), lower_us.begin(), ::tolower);
                if (lower_us != underscored)
                {
                    if (auto it = context.find(lower_us); it != context.end())
                    {
                        return *it;
                    }
                }
                
                // Try without spaces
                std::string nospace(name);
                nospace.erase(std::remove(nospace.begin(), nospace.end(), ' '), nospace.end());
                if (auto it = context.find(nospace); it != context.end())
                {
                    return *it;
                }
            }
            
            // Try lowercase (only if name has uppercase characters)
            {
                bool has_upper = false;
                for (char c : name)
                {
                    if (std::isupper(static_cast<unsigned char>(c)))
                    {
                        has_upper = true;
                        break;
                    }
                }
                if (has_upper)
                {
                    std::string lower(name);
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    if (auto it = context.find(lower); it != context.end())
                    {
                        return *it;
                    }
                }
            }
            
            // Variable not found
            std::ostringstream oss;
            oss << "Undefined variable: '" << name << "'";
            throw std::runtime_error(oss.str());
        }
        
        /**
         * @brief Convert JSON value to number for arithmetic operations
         */
        double toNumber(const json& value, std::string_view operation)
        {
            if (value.is_number())
            {
                return value.get<double>();
            }
            if (value.is_null())
            {
                return 0.0; // null treated as 0 in arithmetic
            }
            if (value.is_boolean())
            {
                return value.get<bool>() ? 1.0 : 0.0;
            }
            if (value.is_string())
            {
                try
                {
                    return std::stod(value.get<std::string>());
                }
                catch (const std::invalid_argument&)
                {
                    std::ostringstream oss;
                    oss << "Cannot convert string to number in " << operation;
                    throw std::runtime_error(oss.str());
                }
                catch (const std::out_of_range&)
                {
                    std::ostringstream oss;
                    oss << "Number out of range in " << operation;
                    throw std::runtime_error(oss.str());
                }
            }
            
            std::ostringstream oss;
            oss << "Type error in " << operation << ": expected number";
            throw std::runtime_error(oss.str());
        }
        
        /**
         * @brief Convert JSON value to string for concatenation
         */
        std::string toString(const json& value)
        {
            if (value.is_string())
            {
                return value.get<std::string>();
            }
            if (value.is_number())
            {
                double d = value.get<double>();
                // Check if it's an integer
                if (d == std::floor(d))
                {
                    return std::to_string(static_cast<long long>(d));
                }
                return std::to_string(d);
            }
            if (value.is_boolean())
            {
                return value.get<bool>() ? "true" : "false";
            }
            if (value.is_null())
            {
                return "null";
            }
            return value.dump(); // Fallback: JSON representation
        }
        
        /**
         * @brief Convert JSON value to boolean for logical operations
         */
        bool toBoolean(const json& value)
        {
            if (value.is_boolean())
            {
                return value.get<bool>();
            }
            if (value.is_number())
            {
                return value.get<double>() != 0.0;
            }
            if (value.is_string())
            {
                std::string s = value.get<std::string>();
                return !s.empty() && s != "false" && s != "0";
            }
            if (value.is_null())
            {
                return false;
            }
            return true; // Non-empty objects/arrays are truthy
        }
    }
    
    json ASTNode::evaluate(const json& input, const EvaluationContext& eval_ctx) const
    {
        switch (type)
        {
            case ASTNodeType::LITERAL_NUMBER:
            {
                // Handle special keywords
                if (value == "true") return true;
                if (value == "false") return false;
                if (value == "null") return nullptr;
                
                // Parse numeric literal
                try
                {
                    // Try integer first
                    if (value.find('.') == std::string::npos && 
                        value.find('e') == std::string::npos && 
                        value.find('E') == std::string::npos)
                    {
                        return std::stoll(value);
                    }
                    // Parse as double
                    return std::stod(value);
                }
                catch (const std::invalid_argument&)
                {
                    std::ostringstream oss;
                    oss << "Invalid number literal: '" << value << "'";
                    throw std::runtime_error(oss.str());
                }
                catch (const std::out_of_range&)
                {
                    std::ostringstream oss;
                    oss << "Number literal out of range: '" << value << "'";
                    throw std::runtime_error(oss.str());
            }
        }
        
        case ASTNodeType::LITERAL_STRING:
        {
            return value; // Already unquoted by parser
        }
        
        case ASTNodeType::LITERAL_LIST:
        {
            json listArray = json::array();
            for (const auto& child : children)
            {
                listArray.push_back(child->evaluate(input, eval_ctx));
            }
            return listArray;
        }
        
        case ASTNodeType::LITERAL_CONTEXT:
        {
            json contextObject = json::object();
            // Children are stored as pairs: [key_node, value_node, key_node, value_node, ...]
            for (size_t i = 0; i + 1 < children.size(); i += 2)
            {
                // Key is stored in a LITERAL_STRING node
                const std::string& key = children[i]->value;
                // Value is any expression
                json val = children[i + 1]->evaluate(input, eval_ctx);
                contextObject[key] = val;
            }
            return contextObject;
        }
        
        case ASTNodeType::VARIABLE:
        {
            return resolveVariable(value, input);
        }           case ASTNodeType::UNARY_OP:
            {
                if (children.size() != 1)
                {
                    throw std::runtime_error("Unary operator requires exactly one operand");
                }
                
                json operand = children[0]->evaluate(input, eval_ctx);
                
                if (value == "-")
                {
                    if (operand.is_null()) return nullptr;
                    if (operand.is_number()) return -operand.get<double>();
                    if (operand.is_string())
                    {
                        std::string s = operand.get<std::string>();
                        if (is_duration_string(s))
                        {
                            // Negate duration: toggle leading '-'
                            if (!s.empty() && s[0] == '-')
                                return s.substr(1);
                            else
                                return "-" + s;
                        }
                        // date, time, datetime, plain string cannot be negated
                        return nullptr;
                    }
                    return nullptr;
                }
                else if (value == "not")
                {
                    return !toBoolean(operand);
                }
                
                std::ostringstream oss;
                oss << "Unknown unary operator: '" << value << "'";
                throw std::runtime_error(oss.str());
            }
            
            case ASTNodeType::BINARY_OP:
            {
                if (children.size() != 2)
                {
                    throw std::runtime_error("Binary operator requires exactly two operands");
                }
                
                json left = children[0]->evaluate(input, eval_ctx);
                json right = children[1]->evaluate(input, eval_ctx);
                
                // Arithmetic operators - DMN strict type checking
                if (value == "+")
                {
                    // DMN: null in arithmetic returns null
                    if (left.is_null() || right.is_null())
                    {
                        return nullptr;
                    }
                    // String concatenation: both must be strings
                    if (left.is_string() && right.is_string())
                    {
                        auto ls = left.get<std::string>();
                        auto rs = right.get<std::string>();
                        // Duration + Duration
                        if (is_duration_string(left) && is_duration_string(right))
                        {
                            auto dl = feel::parse_duration(ls);
                            auto dr = feel::parse_duration(rs);
                            if (dl && dr)
                            {
                                bool l_ym = (dl->total_months != 0);
                                bool r_ym = (dr->total_months != 0);
                                bool l_dt = (dl->total_seconds != 0);
                                bool r_dt = (dr->total_seconds != 0);
                                // Both must be same type
                                if (l_ym && r_dt) return nullptr;
                                if (l_dt && r_ym) return nullptr;
                                bool is_ym = l_ym || r_ym;
                                feel::Duration result;
                                result.total_months = dl->total_months + dr->total_months;
                                result.total_seconds = dl->total_seconds + dr->total_seconds;
                                return format_duration(result, is_ym);
                            }
                        }
                        // date/datetime/time + duration
                        if (is_duration_string(right))
                        {
                            auto dur = feel::parse_duration(rs);
                            if (dur)
                            {
                                bool dur_is_ym = (dur->total_months != 0);
                                if (is_datetime_string(ls))
                                {
                                    auto r = add_duration_to_datetime(ls, *dur);
                                    if (!r.empty()) return r;
                                }
                                else if (is_date_string(ls))
                                {
                                    auto r = add_duration_to_date(ls, *dur);
                                    if (!r.empty()) return r;
                                }
                                else if (is_time_string(ls))
                                {
                                    if (dur_is_ym) return nullptr;
                                    auto r = add_duration_to_time(ls, *dur);
                                    if (!r.empty()) return r;
                                }
                            }
                        }
                        // duration + date/datetime/time (commutative)
                        if (is_duration_string(left))
                        {
                            auto dur = feel::parse_duration(ls);
                            if (dur)
                            {
                                bool dur_is_ym = (dur->total_months != 0);
                                if (is_datetime_string(rs))
                                {
                                    auto r = add_duration_to_datetime(rs, *dur);
                                    if (!r.empty()) return r;
                                }
                                else if (is_date_string(rs))
                                {
                                    auto r = add_duration_to_date(rs, *dur);
                                    if (!r.empty()) return r;
                                }
                                else if (is_time_string(rs))
                                {
                                    if (dur_is_ym) return nullptr;
                                    auto r = add_duration_to_time(rs, *dur);
                                    if (!r.empty()) return r;
                                }
                            }
                        }
                        return ls + rs;
                    }
                    // Numeric addition: both must be numbers
                    if (left.is_number() && right.is_number())
                    {
                        return left.get<double>() + right.get<double>();
                    }
                    // Invalid type combination
                    return nullptr;
                }
                else if (value == "-")
                {
                    if (left.is_null() || right.is_null()) return nullptr;
                    if (left.is_string() && right.is_string())
                    {
                        auto ls = left.get<std::string>();
                        auto rs = right.get<std::string>();
                        // Duration - Duration
                        if (is_duration_string(left) && is_duration_string(right))
                        {
                            auto dl = feel::parse_duration(ls);
                            auto dr = feel::parse_duration(rs);
                            if (dl && dr)
                            {
                                bool l_ym = (dl->total_months != 0);
                                bool r_ym = (dr->total_months != 0);
                                bool l_dt = (dl->total_seconds != 0);
                                bool r_dt = (dr->total_seconds != 0);
                                if (l_ym && r_dt) return nullptr;
                                if (l_dt && r_ym) return nullptr;
                                bool is_ym = l_ym || r_ym;
                                feel::Duration result;
                                result.total_months = dl->total_months - dr->total_months;
                                result.total_seconds = dl->total_seconds - dr->total_seconds;
                                return format_duration(result, is_ym);
                            }
                        }
                        // date/datetime/time - duration
                        if (is_duration_string(right))
                        {
                            auto dur = feel::parse_duration(rs);
                            if (dur)
                            {
                                bool dur_is_ym = (dur->total_months != 0);
                                if (is_datetime_string(ls))
                                {
                                    auto r = add_duration_to_datetime(ls, *dur, true);
                                    if (!r.empty()) return r;
                                }
                                else if (is_date_string(ls))
                                {
                                    auto r = add_duration_to_date(ls, *dur, true);
                                    if (!r.empty()) return r;
                                }
                                else if (is_time_string(ls))
                                {
                                    if (dur_is_ym) return nullptr; // time ± ymDuration is invalid
                                    auto r = add_duration_to_time(ls, *dur, true);
                                    if (!r.empty()) return r;
                                }
                            }
                        }
                        // date - date = duration, datetime - datetime = duration, time - time = duration
                        if (is_datetime_string(ls) && is_datetime_string(rs))
                        {
                            auto dt1 = feel::parse_datetime(ls);
                            auto dt2 = feel::parse_datetime(rs);
                            if (dt1 && dt2)
                            {
                                // Both or neither must have timezone info
                                if (dt1->has_tz != dt2->has_tz) return nullptr;
                                // Convert both to total seconds from epoch-ish (UTC-normalized)
                                auto to_secs = [](const feel::DateTime& dt) -> long long {
                                    long long days = dt.date.y * 365LL + dt.date.y/4 - dt.date.y/100 + dt.date.y/400;
                                    for (int mm = 1; mm < dt.date.m; mm++)
                                    {
                                        static const int md[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
                                        days += md[mm];
                                    }
                                    if (dt.date.m > 2 && ((dt.date.y%4==0 && dt.date.y%100!=0) || dt.date.y%400==0))
                                        days++;
                                    days += dt.date.d;
                                    return days * 86400LL + dt.time.h * 3600LL + dt.time.m * 60LL + dt.time.s - dt.tz_offset_seconds;
                                };
                                long long diff = to_secs(*dt1) - to_secs(*dt2);
                                feel::Duration dur;
                                dur.total_seconds = diff;
                                return format_duration(dur);
                            }
                        }
                        if (is_date_string(ls) && is_date_string(rs))
                        {
                            auto d1 = feel::parse_date(ls);
                            auto d2 = feel::parse_date(rs);
                            if (d1 && d2)
                            {
                                auto to_days = [](const feel::Date& d) -> long long {
                                    long long days = d.y * 365LL + d.y/4 - d.y/100 + d.y/400;
                                    for (int mm = 1; mm < d.m; mm++)
                                    {
                                        static const int md[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
                                        days += md[mm];
                                    }
                                    if (d.m > 2 && ((d.y%4==0 && d.y%100!=0) || d.y%400==0))
                                        days++;
                                    days += d.d;
                                    return days;
                                };
                                long long diff = to_days(*d1) - to_days(*d2);
                                feel::Duration dur;
                                dur.total_seconds = diff * 86400LL;
                                return format_duration(dur);
                            }
                        }
                        // datetime - date or date - datetime: treat date as UTC midnight
                        if ((is_datetime_string(ls) && is_date_string(rs)) || (is_date_string(ls) && is_datetime_string(rs)))
                        {
                            // The datetime must have timezone (date implies UTC)
                            std::string dt_str = is_datetime_string(ls) ? ls : rs;
                            auto dt_check = feel::parse_datetime(dt_str);
                            if (!dt_check || !dt_check->has_tz) return nullptr;
                            
                            std::string ldt = ls, rdt = rs;
                            if (is_date_string(ls)) ldt = ls + "T00:00:00Z";
                            if (is_date_string(rs)) rdt = rs + "T00:00:00Z";
                            auto dt1 = feel::parse_datetime(ldt);
                            auto dt2 = feel::parse_datetime(rdt);
                            if (dt1 && dt2)
                            {
                                auto to_secs = [](const feel::DateTime& dt) -> long long {
                                    long long days = dt.date.y * 365LL + dt.date.y/4 - dt.date.y/100 + dt.date.y/400;
                                    for (int mm = 1; mm < dt.date.m; mm++)
                                    {
                                        static const int md[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
                                        days += md[mm];
                                    }
                                    if (dt.date.m > 2 && ((dt.date.y%4==0 && dt.date.y%100!=0) || dt.date.y%400==0))
                                        days++;
                                    days += dt.date.d;
                                    return days * 86400LL + dt.time.h * 3600LL + dt.time.m * 60LL + dt.time.s - dt.tz_offset_seconds;
                                };
                                long long diff = to_secs(*dt1) - to_secs(*dt2);
                                feel::Duration dur;
                                dur.total_seconds = diff;
                                return format_duration(dur);
                            }
                        }
                        if (is_time_string(ls) && is_time_string(rs))
                        {
                            int h1 = std::stoi(ls.substr(0,2)), m1 = std::stoi(ls.substr(3,2)), s1 = std::stoi(ls.substr(6,2));
                            int h2 = std::stoi(rs.substr(0,2)), m2 = std::stoi(rs.substr(3,2)), s2 = std::stoi(rs.substr(6,2));
                            long long diff = (h1*3600+m1*60+s1) - (h2*3600+m2*60+s2);
                            feel::Duration dur;
                            dur.total_seconds = diff;
                            return format_duration(dur);
                        }
                        return nullptr;
                    }
                    if (!left.is_number() || !right.is_number()) return nullptr;
                    return left.get<double>() - right.get<double>();
                }
                else if (value == "*")
                {
                    if (left.is_null() || right.is_null()) return nullptr;
                    // number * duration or duration * number
                    if (left.is_number() && is_duration_string(right))
                    {
                        auto dr = feel::parse_duration(right.get<std::string>());
                        if (dr)
                        {
                            double n = left.get<double>();
                            bool is_ym = (dr->total_months != 0 || dr->total_seconds == 0);
                            feel::Duration result;
                            if (is_ym)
                            {
                                result.total_months = static_cast<int>(dr->total_months * n);
                            }
                            else
                            {
                                result.total_seconds = static_cast<long long>(std::round(dr->total_seconds * n));
                            }
                            return format_duration(result, is_ym);
                        }
                    }
                    if (is_duration_string(left) && right.is_number())
                    {
                        auto dl = feel::parse_duration(left.get<std::string>());
                        if (dl)
                        {
                            double n = right.get<double>();
                            bool is_ym = (dl->total_months != 0 || dl->total_seconds == 0);
                            feel::Duration result;
                            if (is_ym)
                            {
                                result.total_months = static_cast<int>(dl->total_months * n);
                            }
                            else
                            {
                                result.total_seconds = static_cast<long long>(std::round(dl->total_seconds * n));
                            }
                            return format_duration(result, is_ym);
                        }
                    }
                    if (!left.is_number() || !right.is_number()) return nullptr;
                    return left.get<double>() * right.get<double>();
                }
                else if (value == "/")
                {
                    if (left.is_null() || right.is_null()) return nullptr;
                    // duration / number
                    if (is_duration_string(left) && right.is_number())
                    {
                        double n = right.get<double>();
                        if (n == 0.0) return nullptr;
                        auto dl = feel::parse_duration(left.get<std::string>());
                        if (dl)
                        {
                            bool is_ym = (dl->total_months != 0 || dl->total_seconds == 0);
                            feel::Duration result;
                            if (is_ym)
                            {
                                result.total_months = static_cast<int>(dl->total_months / n);
                            }
                            else
                            {
                                result.total_seconds = static_cast<long long>(std::round(dl->total_seconds / n));
                            }
                            return format_duration(result, is_ym);
                        }
                    }
                    // duration / duration = number
                    if (is_duration_string(left) && is_duration_string(right))
                    {
                        auto dl = feel::parse_duration(left.get<std::string>());
                        auto dr = feel::parse_duration(right.get<std::string>());
                        if (dl && dr)
                        {
                            bool l_ym = (dl->total_months != 0);
                            bool r_ym = (dr->total_months != 0);
                            bool l_dt = (dl->total_seconds != 0);
                            bool r_dt = (dr->total_seconds != 0);
                            // Cross-type division is an error
                            if (l_ym && r_dt) return nullptr;
                            if (l_dt && r_ym) return nullptr;
                            // Zero numerator
                            if (!l_ym && !l_dt) {
                                if (r_ym || r_dt) return 0.0;
                                return nullptr; // 0/0
                            }
                            if (l_ym && r_ym) {
                                if (dr->total_months == 0) return nullptr;
                                return static_cast<double>(dl->total_months) / dr->total_months;
                            }
                            if (dr->total_seconds == 0) return nullptr;
                            return static_cast<double>(dl->total_seconds) / dr->total_seconds;
                        }
                    }
                    if (!left.is_number() || !right.is_number()) return nullptr;
                    double divisor = right.get<double>();
                    if (divisor == 0.0) return nullptr;
                    return left.get<double>() / divisor;
                }
                else if (value == "**")
                {
                    if (left.is_null() || right.is_null()) return nullptr;
                    if (!left.is_number() || !right.is_number()) return nullptr;
                    return std::pow(left.get<double>(), right.get<double>());
                }
                
                // Comparison operators - DMN strict type checking
                else if (value == "<")
                {
                    if (left.is_null() || right.is_null()) return nullptr;
                    if (left.is_string() && right.is_string())
                        return left.get<std::string>() < right.get<std::string>();
                    if (left.is_number() && right.is_number())
                        return left.get<double>() < right.get<double>();
                    return nullptr;
                }
                else if (value == ">")
                {
                    if (left.is_null() || right.is_null()) return nullptr;
                    if (left.is_string() && right.is_string())
                        return left.get<std::string>() > right.get<std::string>();
                    if (left.is_number() && right.is_number())
                        return left.get<double>() > right.get<double>();
                    return nullptr;
                }
                else if (value == "<=")
                {
                    if (left.is_null() || right.is_null()) return nullptr;
                    if (left.is_string() && right.is_string())
                        return left.get<std::string>() <= right.get<std::string>();
                    if (left.is_number() && right.is_number())
                        return left.get<double>() <= right.get<double>();
                    return nullptr;
                }
                else if (value == ">=")
                {
                    if (left.is_null() || right.is_null()) return nullptr;
                    if (left.is_string() && right.is_string())
                        return left.get<std::string>() >= right.get<std::string>();
                    if (left.is_number() && right.is_number())
                        return left.get<double>() >= right.get<double>();
                    return nullptr;
                }
                else if (value == "=" || value == "==")
                {
                    // DMN/TCK: null = null → true, X = null → false, null = X → false
                    if (left.is_null() && right.is_null()) return true;
                    if (left.is_null() || right.is_null()) return false;
                    // Numbers of different JSON sub-types are still comparable
                    if (left.is_number() && right.is_number())
                    {
                        return left == right;
                    }
                    // DMN spec: comparing incomparable non-null types returns null
                    if (left.type() != right.type())
                    {
                        return nullptr;
                    }
                    // Temporal string normalization: Z ↔ +00:00
                    if (left.is_string() && right.is_string())
                    {
                        std::string ls = left.get<std::string>();
                        std::string rs = right.get<std::string>();
                        // Normalize Z to +00:00 for comparison
                        auto normalize_tz = [](std::string& s) {
                            if (!s.empty() && s.back() == 'Z') {
                                s = s.substr(0, s.size() - 1) + "+00:00";
                            }
                        };
                        normalize_tz(ls);
                        normalize_tz(rs);
                        return ls == rs;
                    }
                    return left == right;
                }
                else if (value == "!=")
                {
                    // DMN/TCK: null != null → false, X != null → true, null != X → true
                    if (left.is_null() && right.is_null()) return false;
                    if (left.is_null() || right.is_null()) return true;
                    if (left.is_number() && right.is_number())
                    {
                        return left != right;
                    }
                    if (left.type() != right.type())
                    {
                        return nullptr;
                    }
                    // Temporal string normalization: Z ↔ +00:00
                    if (left.is_string() && right.is_string())
                    {
                        std::string ls = left.get<std::string>();
                        std::string rs = right.get<std::string>();
                        auto normalize_tz = [](std::string& s) {
                            if (!s.empty() && s.back() == 'Z') {
                                s = s.substr(0, s.size() - 1) + "+00:00";
                            }
                        };
                        normalize_tz(ls);
                        normalize_tz(rs);
                        return ls != rs;
                    }
                    return left != right;
                }
                
                // Logical operators - DMN ternary logic with null propagation
                else if (value == "and")
                {
                    // DMN ternary AND logic: non-boolean values treated as null
                    bool left_is_bool = left.is_boolean();
                    bool right_is_bool = right.is_boolean();
                    bool left_is_null = left.is_null() || !left_is_bool;
                    bool right_is_null = right.is_null() || !right_is_bool;
                    
                    if (left_is_null && right_is_null) return nullptr;
                    if (left_is_null)
                    {
                        if (!right.get<bool>()) return false;
                        return nullptr;
                    }
                    if (right_is_null)
                    {
                        if (!left.get<bool>()) return false;
                        return nullptr;
                    }
                    return left.get<bool>() && right.get<bool>();
                }
                else if (value == "or")
                {
                    // DMN ternary OR logic: non-boolean values treated as null
                    bool left_is_bool = left.is_boolean();
                    bool right_is_bool = right.is_boolean();
                    bool left_is_null = left.is_null() || !left_is_bool;
                    bool right_is_null = right.is_null() || !right_is_bool;
                    
                    if (left_is_null && right_is_null) return nullptr;
                    if (left_is_null)
                    {
                        if (right.get<bool>()) return true;
                        return nullptr;
                    }
                    if (right_is_null)
                    {
                        if (left.get<bool>()) return true;
                        return nullptr;
                    }
                    return left.get<bool>() || right.get<bool>();
                }
                else if (value == "in")
                {
                    // FEEL 'in' operator: check if left is in right
                    // right can be: list, range, unary test, or single value
                    if (left.is_null()) return nullptr;
                    
                    // Don't evaluate right as a value - it's a test/range/list
                    // The right child is already evaluated above as `right`
                    
                    // Right is a list: check membership
                    if (right.is_array()) {
                        for (const auto& item : right) {
                            if (item.is_object() && item.contains("__range__")) {
                                // Range object: check bounds
                                std::string type = item["__range__"].get<std::string>();
                                auto start_val = item["__start__"];
                                auto end_val = item["__end__"];
                                bool start_incl = (type[0] == '[');
                                bool end_incl = (type[1] == ']');
                                
                                if (left.is_number() && start_val.is_number() && end_val.is_number()) {
                                    double v = left.get<double>();
                                    double s = start_val.get<double>();
                                    double e = end_val.get<double>();
                                    bool in_range = (start_incl ? v >= s : v > s) && (end_incl ? v <= e : v < e);
                                    if (in_range) return true;
                                } else if (left.is_string() && start_val.is_string() && end_val.is_string()) {
                                    auto v = left.get<std::string>();
                                    auto s = start_val.get<std::string>();
                                    auto e = end_val.get<std::string>();
                                    bool in_range = (start_incl ? v >= s : v > s) && (end_incl ? v <= e : v < e);
                                    if (in_range) return true;
                                }
                            } else if (item.is_object() && item.contains("__unary_test__")) {
                                // Unary test object
                                std::string op = item["__unary_test__"].get<std::string>();
                                if (op == "not") {
                                    // not(tests) - negate the result of checking item list
                                    auto& tests = item["__operand__"];
                                    bool matched = false;
                                    if (tests.is_array()) {
                                        for (const auto& t : tests) {
                                            if (t.is_object() && t.contains("__unary_test__")) {
                                                std::string top = t["__unary_test__"].get<std::string>();
                                                auto topnd = t["__operand__"];
                                                if (top == "=") { if (left == topnd) matched = true; }
                                                else if (top == "!=") { if (left != topnd) matched = true; }
                                                else if (left.is_number() && topnd.is_number()) {
                                                    double v2 = left.get<double>(), o2 = topnd.get<double>();
                                                    if (top == "<" && v2 < o2) matched = true;
                                                    else if (top == ">" && v2 > o2) matched = true;
                                                    else if (top == "<=" && v2 <= o2) matched = true;
                                                    else if (top == ">=" && v2 >= o2) matched = true;
                                                }
                                            } else if (t.is_object() && t.contains("__range__")) {
                                                // range check inside not
                                                if (left.is_number() && t["__start__"].is_number() && t["__end__"].is_number()) {
                                                    double v2 = left.get<double>(), s2 = t["__start__"].get<double>(), e2 = t["__end__"].get<double>();
                                                    std::string rt = t["__range__"].get<std::string>();
                                                    bool si = rt[0] == '[', ei = rt[1] == ']';
                                                    if ((si ? v2 >= s2 : v2 > s2) && (ei ? v2 <= e2 : v2 < e2)) matched = true;
                                                }
                                            } else {
                                                if (left == t) matched = true;
                                            }
                                        }
                                    }
                                    if (!matched) return true;
                                } else {
                                auto operand = item["__operand__"];
                                if (op == "=") {
                                    if (left == operand) return true;
                                } else if (op == "!=") {
                                    if (left != operand) return true;
                                } else if (left.is_number() && operand.is_number()) {
                                    double v = left.get<double>();
                                    double o = operand.get<double>();
                                    bool result = false;
                                    if (op == "<") result = v < o;
                                    else if (op == ">") result = v > o;
                                    else if (op == "<=") result = v <= o;
                                    else if (op == ">=") result = v >= o;
                                    if (result) return true;
                                } else if (left.is_string() && operand.is_string()) {
                                    auto v = left.get<std::string>();
                                    auto o = operand.get<std::string>();
                                    bool result = false;
                                    if (op == "<") result = v < o;
                                    else if (op == ">") result = v > o;
                                    else if (op == "<=") result = v <= o;
                                    else if (op == ">=") result = v >= o;
                                    if (result) return true;
                                }
                                }
                            } else {
                                // Plain value: equality check
                                if (left == item) return true;
                            }
                        }
                        return false;
                    }
                    
                    // Right is a range object
                    if (right.is_object() && right.contains("__range__")) {
                        std::string type = right["__range__"].get<std::string>();
                        auto start_val = right["__start__"];
                        auto end_val = right["__end__"];
                        bool start_incl = (type[0] == '[');
                        bool end_incl = (type[1] == ']');
                        
                        if (left.is_number() && start_val.is_number() && end_val.is_number()) {
                            double v = left.get<double>();
                            double s = start_val.get<double>();
                            double e = end_val.get<double>();
                            return (start_incl ? v >= s : v > s) && (end_incl ? v <= e : v < e);
                        }
                        if (left.is_string() && start_val.is_string() && end_val.is_string()) {
                            auto v = left.get<std::string>();
                            auto s = start_val.get<std::string>();
                            auto e = end_val.get<std::string>();
                            return (start_incl ? v >= s : v > s) && (end_incl ? v <= e : v < e);
                        }
                        return nullptr;
                    }
                    
                    // Right is a unary test object
                    if (right.is_object() && right.contains("__unary_test__")) {
                        std::string op = right["__unary_test__"].get<std::string>();
                        if (op == "not") {
                            // not(tests) - check if left is NOT in the test list
                            auto& tests = right["__operand__"];
                            if (tests.is_array()) {
                                for (const auto& t : tests) {
                                    if (t.is_object() && t.contains("__unary_test__")) {
                                        std::string top = t["__unary_test__"].get<std::string>();
                                        auto topnd = t["__operand__"];
                                        if (top == "=") { if (left == topnd) return false; }
                                        else if (top == "!=") { if (left != topnd) return false; }
                                        else if (left.is_number() && topnd.is_number()) {
                                            double v = left.get<double>(), o = topnd.get<double>();
                                            if (top == "<" && v < o) return false;
                                            if (top == ">" && v > o) return false;
                                            if (top == "<=" && v <= o) return false;
                                            if (top == ">=" && v >= o) return false;
                                        }
                                    } else if (t.is_object() && t.contains("__range__")) {
                                        if (left.is_number() && t["__start__"].is_number() && t["__end__"].is_number()) {
                                            double v = left.get<double>(), s = t["__start__"].get<double>(), e = t["__end__"].get<double>();
                                            std::string rt = t["__range__"].get<std::string>();
                                            bool si = rt[0] == '[', ei = rt[1] == ']';
                                            if ((si ? v >= s : v > s) && (ei ? v <= e : v < e)) return false;
                                        }
                                    } else {
                                        if (left == t) return false;
                                    }
                                }
                            }
                            return true;
                        }
                        auto operand = right["__operand__"];
                        if (op == "=") return left == operand;
                        if (op == "!=") return left != operand;
                        if (left.is_number() && operand.is_number()) {
                            double v = left.get<double>();
                            double o = operand.get<double>();
                            if (op == "<") return v < o;
                            if (op == ">") return v > o;
                            if (op == "<=") return v <= o;
                            if (op == ">=") return v >= o;
                        }
                        if (left.is_string() && operand.is_string()) {
                            auto v = left.get<std::string>();
                            auto o = operand.get<std::string>();
                            if (op == "<") return v < o;
                            if (op == ">") return v > o;
                            if (op == "<=") return v <= o;
                            if (op == ">=") return v >= o;
                        }
                        return nullptr;
                    }
                    
                    // Right is a single value: equality check
                    if (right.is_null()) return nullptr;
                    return left == right;
                }
                
                std::ostringstream oss;
                oss << "Unknown binary operator: '" << value << "'";
                throw std::runtime_error(oss.str());
            }
            
            case ASTNodeType::PROPERTY_ACCESS:
            {
                // Property access: object.property
                // The child is the object expression, value is the property name
                if (children.size() != 1)
                {
                    throw std::runtime_error("Property access requires exactly one child (object expression)");
                }
                
                // Evaluate the object expression
                json obj = children[0]->evaluate(input, eval_ctx);
                
                // Property name is stored in value
                const std::string& propertyName = value;
                
                // Handle null object - return null per DMN semantics
                if (obj.is_null())
                {
                    return nullptr;
                }
                
                // Handle temporal property access on strings
                if (obj.is_string())
                {
                    std::string str_val = obj.get<std::string>();
                    json temporal_result = feel::get_temporal_property(str_val, propertyName);
                    if (!temporal_result.is_null())
                    {
                        return temporal_result;
                    }
                    // Not a temporal property - fall through to error
                }
                
                // Obj must be an object/dict to have properties
                if (!obj.is_object())
                {
                    return nullptr;
                }
                
                // Try exact property name first (hot path — avoids all string allocations)
                if (auto it = obj.find(propertyName); it != obj.end())
                {
                    return *it;
                }
                
                // Try with spaces replaced by underscores (only if name contains spaces)
                if (propertyName.find(' ') != std::string::npos)
                {
                    std::string underscored = propertyName;
                    std::replace(underscored.begin(), underscored.end(), ' ', '_');
                    if (auto it = obj.find(underscored); it != obj.end())
                    {
                        return *it;
                    }
                }
                
                // Convert camelCase to snake_case (only if name has uppercase)
                bool has_upper = false;
                for (char c : propertyName)
                {
                    if (std::isupper(static_cast<unsigned char>(c)))
                    {
                        has_upper = true;
                        break;
                    }
                }
                
                if (has_upper)
                {
                    std::string snake_case;
                    snake_case.reserve(propertyName.length() + 4);
                    for (size_t i = 0; i < propertyName.length(); ++i)
                    {
                        char c = propertyName[i];
                        if (std::isupper(c) && i > 0)
                        {
                            snake_case += '_';
                            snake_case += static_cast<char>(std::tolower(c));
                        }
                        else
                        {
                            snake_case += static_cast<char>(std::tolower(c));
                        }
                    }
                    if (auto it = obj.find(snake_case); it != obj.end())
                    {
                        return *it;
                    }
                    
                    // Try lowercase (reuse first part of snake_case logic)
                    std::string lower = propertyName;
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    if (lower != snake_case)
                    {
                        if (auto it = obj.find(lower); it != obj.end())
                        {
                            return *it;
                        }
                    }
                }
                
                // Property not found - throw error
                std::ostringstream oss;
                oss << "Property '" << propertyName << "' not found on object";
                throw std::runtime_error(oss.str());
            }
            
        case ASTNodeType::CONDITIONAL:
        {
            // If-then-else conditional expression
            // children[0] = condition
            // children[1] = then expression
            // children[2] = else expression
            
            if (children.size() != 3)
            {
                throw std::runtime_error("Conditional expression requires exactly 3 children (condition, then, else)");
            }
            
            // Evaluate condition
            json condition = children[0]->evaluate(input, eval_ctx);
            
            // DMN spec: null condition → else branch
            if (condition.is_null())
            {
                return children[2]->evaluate(input, eval_ctx);
            }
            
            // Type validation: condition should be boolean
            if (!condition.is_boolean())
            {
                // Per DMN spec, return null for type errors
                return json(nullptr);
            }
            
            // Short-circuit evaluation: only evaluate selected branch
            if (condition.get<bool>())
            {
                // Evaluate then branch
                return children[1]->evaluate(input, eval_ctx);
            }
            else
            {
                // Evaluate else branch
                return children[2]->evaluate(input, eval_ctx);
            }
        }
            
        case ASTNodeType::FUNCTION_CALL:
        {
            const std::string& funcName = value;
            
            // Bind parameters using parameter binder
            // This handles both positional and named parameters
            // Parameter validation errors should return null per DMN spec
            std::vector<json> args;
            try {
                args = feel::bind_parameters(funcName, parameters, input, eval_ctx);
            } catch (const std::runtime_error&) {
                // Parameter validation failed (wrong param names, wrong count, etc.)
                // Return null as per DMN 1.5 spec
                return json(nullptr);
            }
            
            // Dispatch to appropriate function
            if (funcName == "not")
            {
                return feel::evaluate_not_function(args);
            }
            else if (funcName == "all")
            {
                return feel::evaluate_all_function(args);
            }
            else if (funcName == "any")
            {
                return feel::evaluate_any_function(args);
            }
            else if (funcName == "contains")
            {
                return feel::evaluate_contains_function(args);
            }
            // Math functions
            else if (funcName == "abs")
            {
                return feel::evaluate_abs_function(args);
            }
            else if (funcName == "sqrt")
            {
                return feel::evaluate_sqrt_function(args);
            }
            else if (funcName == "floor")
            {
                return feel::evaluate_floor_function(args);
            }
            else if (funcName == "ceiling")
            {
                return feel::evaluate_ceiling_function(args);
            }
            else if (funcName == "exp")
            {
                return feel::evaluate_exp_function(args);
            }
            else if (funcName == "log")
            {
                return feel::evaluate_log_function(args);
            }
            else if (funcName == "modulo")
            {
                return feel::evaluate_modulo_function(args);
            }
            else if (funcName == "decimal")
            {
                return feel::evaluate_decimal_function(args);
            }
            else if (funcName == "round")
            {
                return feel::evaluate_round_function(args);
            }
            else if (funcName == "round up")
            {
                return feel::evaluate_round_up_function(args);
            }
            else if (funcName == "round down")
            {
                return feel::evaluate_round_down_function(args);
            }
            else if (funcName == "round half up")
            {
                return feel::evaluate_round_half_up_function(args);
            }
            else if (funcName == "round half down")
            {
                return feel::evaluate_round_half_down_function(args);
            }
            // String functions
            else if (funcName == "substring before")
            {
                return feel::evaluate_substring_before_function(args);
            }
            else if (funcName == "substring after")
            {
                return feel::evaluate_substring_after_function(args);
            }
            else if (funcName == "substring")
            {
                return feel::evaluate_substring_function(args);
            }
            else if (funcName == "string length")
            {
                return feel::evaluate_string_length_function(args);
            }
            else if (funcName == "upper case")
            {
                return feel::evaluate_upper_case_function(args);
            }
            else if (funcName == "lower case")
            {
                return feel::evaluate_lower_case_function(args);
            }
            else if (funcName == "starts with")
            {
                return feel::evaluate_starts_with_function(args);
            }
            else if (funcName == "ends with")
            {
                return feel::evaluate_ends_with_function(args);
            }
            else if (funcName == "replace")
            {
                return feel::evaluate_replace_function(args);
            }
            else if (funcName == "matches")
            {
                return feel::evaluate_matches_function(args, eval_ctx);
            }
            else if (funcName == "split")
            {
                return feel::evaluate_split_function(args);
            }
            else if (funcName == "string join")
            {
                return feel::evaluate_string_join_function(args);
            }
            else if (funcName == "date")
            {
                return feel::evaluate_date_function(args);
            }
            else if (funcName == "duration")
            {
                return feel::evaluate_duration_function(args);
            }
            else if (funcName == "time")
            {
                return feel::evaluate_time_function(args);
            }
            else if (funcName == "date and time")
            {
                return feel::evaluate_date_and_time_function(args);
            }
            else if (funcName == "years and months duration")
            {
                return feel::evaluate_years_and_months_duration_function(args);
            }
            else if (funcName == "day of year")
            {
                return feel::evaluate_day_of_year_function(args);
            }
            else if (funcName == "day of week")
            {
                return feel::evaluate_day_of_week_function(args);
            }
            else if (funcName == "month of year")
            {
                return feel::evaluate_month_of_year_function(args);
            }
            else if (funcName == "week of year")
            {
                return feel::evaluate_week_of_year_function(args);
            }
            else if (funcName == "now")
            {
                return feel::evaluate_now_function(args);
            }
            else if (funcName == "today")
            {
                return feel::evaluate_today_function(args);
            }
            // Phase 1: Trivial functions
            else if (funcName == "odd")
            {
                return feel::evaluate_odd_function(args);
            }
            else if (funcName == "even")
            {
                return feel::evaluate_even_function(args);
            }
            else if (funcName == "number")
            {
                return feel::evaluate_number_function(args);
            }
            else if (funcName == "string")
            {
                return feel::evaluate_string_function(args);
            }
            else if (funcName == "is")
            {
                return feel::evaluate_is_function(args);
            }
            // Phase 2A: Aggregation functions
            else if (funcName == "count")
            {
                return feel::evaluate_count_function(args);
            }
            else if (funcName == "sum")
            {
                return feel::evaluate_sum_function(args);
            }
            else if (funcName == "min")
            {
                return feel::evaluate_min_function(args);
            }
            else if (funcName == "max")
            {
                return feel::evaluate_max_function(args);
            }
            else if (funcName == "mean")
            {
                return feel::evaluate_mean_function(args);
            }
            else if (funcName == "product")
            {
                return feel::evaluate_product_function(args);
            }
            else if (funcName == "median")
            {
                return feel::evaluate_median_function(args);
            }
            else if (funcName == "stddev")
            {
                return feel::evaluate_stddev_function(args);
            }
            else if (funcName == "mode")
            {
                return feel::evaluate_mode_function(args);
            }
            // Phase 2B: List manipulation functions
            else if (funcName == "list contains")
            {
                return feel::evaluate_list_contains_function(args);
            }
            else if (funcName == "append")
            {
                return feel::evaluate_append_function(args);
            }
            else if (funcName == "concatenate")
            {
                return feel::evaluate_concatenate_function(args);
            }
            else if (funcName == "insert before")
            {
                return feel::evaluate_insert_before_function(args);
            }
            else if (funcName == "remove")
            {
                return feel::evaluate_remove_function(args);
            }
            else if (funcName == "reverse")
            {
                return feel::evaluate_reverse_function(args);
            }
            else if (funcName == "index of")
            {
                return feel::evaluate_index_of_function(args);
            }
            else if (funcName == "sublist")
            {
                return feel::evaluate_sublist_function(args);
            }
            else if (funcName == "union")
            {
                return feel::evaluate_union_function(args);
            }
            else if (funcName == "distinct values")
            {
                return feel::evaluate_distinct_values_function(args);
            }
            else if (funcName == "flatten")
            {
                return feel::evaluate_flatten_function(args);
            }
            else if (funcName == "sort")
            {
                return feel::evaluate_sort_function(args);
            }
            else if (funcName == "list replace")
            {
                return feel::evaluate_list_replace_function(args);
            }
            // ========== PHASE 3: CONTEXT FUNCTIONS ==========
            else if (funcName == "get value")
            {
                return feel::evaluate_get_value_function(args);
            }
            else if (funcName == "get entries")
            {
                return feel::evaluate_get_entries_function(args);
            }
            else if (funcName == "context")
            {
                return feel::evaluate_context_function(args);
            }
            else if (funcName == "context put")
            {
                return feel::evaluate_context_put_function(args);
            }
            else if (funcName == "context merge")
            {
                return feel::evaluate_context_merge_function(args);
            }
            else if (eval_ctx.bkm_map)
            {
                auto bkm_it = eval_ctx.bkm_map->find(funcName);
                if (bkm_it != eval_ctx.bkm_map->end())
                {
                    return bkm_it->second->invoke(args, input, *eval_ctx.bkm_map, eval_ctx);
                }
                std::ostringstream oss;
                oss << "Unknown function: " << funcName;
                throw std::runtime_error(oss.str());
            }
            else
            {
                std::ostringstream oss;
                oss << "Unknown function: " << funcName;
                throw std::runtime_error(oss.str());
            }
        }           case ASTNodeType::BETWEEN:
            {
                // x between a and b → x >= a and x <= b
                if (children.size() != 3) return nullptr;
                auto val = children[0]->evaluate(input, eval_ctx);
                auto lower = children[1]->evaluate(input, eval_ctx);
                auto upper = children[2]->evaluate(input, eval_ctx);
                
                if (val.is_null() || lower.is_null() || upper.is_null()) return nullptr;
                if (!val.is_number() || !lower.is_number() || !upper.is_number())
                {
                    // Try string comparison
                    if (val.is_string() && lower.is_string() && upper.is_string())
                    {
                        auto v = val.get<std::string>();
                        return v >= lower.get<std::string>() && v <= upper.get<std::string>();
                    }
                    return nullptr;
                }
                double v = val.get<double>();
                return v >= lower.get<double>() && v <= upper.get<double>();
            }
            case ASTNodeType::INSTANCE_OF:
            {
                if (children.size() != 1) return nullptr;
                auto val = children[0]->evaluate(input, eval_ctx);
                
                const std::string& type_name = value;
                
                if (val.is_null()) return type_name == "Null" || type_name == "null";
                if (type_name == "Any") return true;
                if (type_name == "number") return val.is_number();
                if (type_name == "string") return val.is_string();
                if (type_name == "boolean") return val.is_boolean();
                if (type_name == "list") return val.is_array();
                if (type_name == "context") return val.is_object() && !val.contains("__range__") && !val.contains("__unary_test__");
                if (type_name == "null" || type_name == "Null") return val.is_null();
                
                // Temporal types - check string-encoded temporal values
                if (val.is_string())
                {
                    auto s = val.get<std::string>();
                    if (type_name == "date") return is_date_string(s);
                    if (type_name == "time") return is_time_string(s);
                    if (type_name == "date and time") return is_datetime_string(s);
                    if (type_name == "days and time duration") {
                        auto d = feel::parse_duration(s);
                        return d.has_value() && d->total_months == 0;
                    }
                    if (type_name == "years and months duration") {
                        auto d = feel::parse_duration(s);
                        return d.has_value() && d->total_months != 0;
                    }
                }
                
                // Parameterized types
                if (type_name.starts_with("list")) return val.is_array();
                if (type_name.starts_with("context")) return val.is_object();
                if (type_name.starts_with("function")) return false; // functions not supported yet
                
                return false;
            }
            case ASTNodeType::FOR_EXPR:
            {
                // children: [var1, list1, var2, list2, ..., returnExpr]
                // Last child is always the return expression
                if (children.size() < 3) return nullptr;
                
                size_t num_bindings = (children.size() - 1) / 2;
                auto return_expr = children.back().get();
                
                // For simplicity, handle single binding first
                // for x in list return expr
                if (num_bindings == 1)
                {
                    auto& var_node = children[0];
                    auto list_val = children[1]->evaluate(input, eval_ctx);
                    
                    if (list_val.is_null()) return nullptr;
                    
                    // Handle range object: expand to array for iteration
                    if (list_val.is_object() && list_val.contains("__range__"))
                    {
                        auto& start = list_val["__start__"];
                        auto& end = list_val["__end__"];
                        // Only numeric integer ranges can be expanded
                        if (start.is_number() && end.is_number())
                        {
                            auto s = start.get<double>();
                            auto e = end.get<double>();
                            if (s != std::floor(s) || e != std::floor(e) || s > e)
                                return nullptr;
                            json list = json::array();
                            for (double v = s; v <= e; v += 1.0)
                                list.push_back(v);
                            json result = json::array();
                            json local_ctx = input;
                            for (const auto& item : list)
                            {
                                local_ctx[var_node->value] = item;
                                auto val = return_expr->evaluate(local_ctx, eval_ctx);
                                result.push_back(val);
                            }
                            return result;
                        }
                        // Non-numeric ranges (string, date, time, duration) → null
                        return nullptr;
                    }
                    
                    // Auto-wrap non-list to list
                    json list;
                    if (list_val.is_array())
                    {
                        list = list_val;
                    }
                    else
                    {
                        list = json::array();
                        list.push_back(list_val);
                    }
                    
                    json result = json::array();
                    json local_ctx = input;
                    for (const auto& item : list)
                    {
                        local_ctx[var_node->value] = item;
                        auto val = return_expr->evaluate(local_ctx, eval_ctx);
                        result.push_back(val);
                    }
                    return result;
                }
                else
                {
                    // Multi-binding for loop - nested iteration
                    // Recursive approach: iterate first binding, for each value recurse
                    std::function<json(size_t, json&)> iterate;
                    iterate = [&](size_t binding_idx, json& ctx) -> json {
                        if (binding_idx >= num_bindings)
                        {
                            return return_expr->evaluate(ctx, eval_ctx);
                        }
                        
                        auto& var_node = children[binding_idx * 2];
                        auto list_val = children[binding_idx * 2 + 1]->evaluate(ctx, eval_ctx);
                        
                        if (list_val.is_null()) return json::array();
                        
                        json list = list_val.is_array() ? list_val : json::array({list_val});
                        
                        json results = json::array();
                        for (const auto& item : list)
                        {
                            ctx[var_node->value] = item;
                            auto inner = iterate(binding_idx + 1, ctx);
                            if (inner.is_array() && binding_idx + 1 < num_bindings)
                            {
                                for (const auto& r : inner) results.push_back(r);
                            }
                            else
                            {
                                results.push_back(inner);
                            }
                        }
                        return results;
                    };
                    
                    json local_ctx = input;
                    return iterate(0, local_ctx);
                }
            }
            case ASTNodeType::QUANTIFIED_EXPR:
            {
                // children: [var1, list1, ..., condition]
                if (children.size() < 3) return nullptr;
                
                size_t num_bindings = (children.size() - 1) / 2;
                auto condition = children.back().get();
                bool is_some = (value == "some");
                
                if (num_bindings == 1)
                {
                    auto& var_node = children[0];
                    auto list_val = children[1]->evaluate(input, eval_ctx);
                    
                    if (list_val.is_null()) return nullptr;
                    json list = list_val.is_array() ? list_val : json::array({list_val});
                    
                    json local_ctx = input;
                    for (const auto& item : list)
                    {
                        local_ctx[var_node->value] = item;
                        auto result = condition->evaluate(local_ctx, eval_ctx);
                        
                        if (result.is_boolean() && result.get<bool>())
                        {
                            if (is_some) return true;
                        }
                        else if (result.is_boolean() && !result.get<bool>())
                        {
                            if (!is_some) return false; // every: found false
                        }
                    }
                    return !is_some; // some: all false → false; every: all true → true
                }
                else
                {
                    // Multi-binding: nested iteration with short-circuit
                    std::function<json(size_t, json&)> iterate;
                    bool found_true = false;
                    bool found_false = false;
                    
                    iterate = [&](size_t binding_idx, json& ctx) -> json {
                        if (binding_idx >= num_bindings)
                        {
                            auto result = condition->evaluate(ctx, eval_ctx);
                            if (result.is_boolean())
                            {
                                if (result.get<bool>()) found_true = true;
                                else found_false = true;
                            }
                            return result;
                        }
                        
                        auto& var_node = children[binding_idx * 2];
                        auto list_val = children[binding_idx * 2 + 1]->evaluate(ctx, eval_ctx);
                        if (list_val.is_null()) return nullptr;
                        json list = list_val.is_array() ? list_val : json::array({list_val});
                        
                        for (const auto& item : list)
                        {
                            ctx[var_node->value] = item;
                            iterate(binding_idx + 1, ctx);
                            if (is_some && found_true) return json(true);
                            if (!is_some && found_false) return json(false);
                        }
                        return nullptr;
                    };
                    
                    json local_ctx = input;
                    iterate(0, local_ctx);
                    
                    if (is_some) return found_true;
                    return !found_false;
                }
            }
            case ASTNodeType::FILTER_EXPR:
            {
                // children[0] = list, children[1] = filter condition
                if (children.size() != 2) return nullptr;
                auto list_val = children[0]->evaluate(input, eval_ctx);
                
                if (list_val.is_null()) return nullptr;
                // DMN: singleton coercion - non-list value treated as [value]
                if (!list_val.is_array())
                {
                    list_val = json::array({list_val});
                }
                
                auto& filter = children[1];
                
                // Check if filter is a numeric index
                auto idx_val = filter->evaluate(input, eval_ctx);
                if (idx_val.is_number())
                {
                    int idx = static_cast<int>(idx_val.get<double>());
                    int size = static_cast<int>(list_val.size());
                    
                    if (idx > 0 && idx <= size) return list_val[idx - 1];
                    if (idx < 0 && -idx <= size) return list_val[size + idx];
                    return nullptr;
                }
                
                // Filter by condition
                json result = json::array();
                json local_ctx = input;
                for (size_t i = 0; i < list_val.size(); ++i)
                {
                    const auto& item = list_val[i];
                    local_ctx["item"] = item;
                    
                    // If item is context, merge its keys into local scope
                    if (item.is_object())
                    {
                        for (auto it = item.begin(); it != item.end(); ++it)
                        {
                            local_ctx[it.key()] = it.value();
                        }
                    }
                    
                    auto cond_result = filter->evaluate(local_ctx, eval_ctx);
                    if (cond_result.is_boolean() && cond_result.get<bool>())
                    {
                        result.push_back(item);
                    }
                }
                return result;
            }
            case ASTNodeType::RANGE:
            {
                // Evaluates to a special JSON object representing a range
                // value contains the bracket types (e.g., "[]", "(]", "[)", "()")
                if (children.size() != 2) return nullptr;
                json start_val = children[0]->evaluate(input, eval_ctx);
                json end_val = children[1]->evaluate(input, eval_ctx);
                json range_obj;
                range_obj["__range__"] = value;
                range_obj["__start__"] = start_val;
                range_obj["__end__"] = end_val;
                return range_obj;
            }
            case ASTNodeType::UNARY_TEST:
            {
                // Evaluates to a special JSON object representing a unary test
                // value contains the operator (e.g., "<=", ">=", "<", ">")
                if (children.size() != 1) return nullptr;
                json operand = children[0]->evaluate(input, eval_ctx);
                json test_obj;
                test_obj["__unary_test__"] = value;
                test_obj["__operand__"] = operand;
                return test_obj;
            }
            default:
            {
                std::ostringstream oss;
                oss << "Unknown AST node type: " << static_cast<int>(type);
                throw std::runtime_error(oss.str());
            }
        }
    }
} // namespace orion::bre
