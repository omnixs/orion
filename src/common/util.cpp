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

#include "util.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <cctype>

namespace orion::common
{
    std::string load_text_file(const std::filesystem::path& file_path)
    {
        std::ifstream ifs(file_path, std::ios::binary);
        if (!ifs) {
            throw std::runtime_error("Cannot open file: " + file_path.string());
        }
        std::ostringstream oss;
        oss << ifs.rdbuf();
        return oss.str();
    }

    void save_text_file(const std::filesystem::path& file_path, std::string_view content)
    {
        std::filesystem::create_directories(file_path.parent_path());
        std::ofstream ofs(file_path, std::ios::binary | std::ios::trunc);
        if (!ofs) {
            throw std::runtime_error("Cannot write file: " + file_path.string());
        }
        ofs.write(content.data(), static_cast<std::streamsize>
                  (content.size()));
    }

    std::string trim(std::string_view str)
    {
        size_t begin = 0;
        while (begin < str.size() && (std::isspace(static_cast<unsigned char>(str[begin])) != 0)) {
            ++begin;
        }
        size_t end = str.size();
        while (end > begin && (std::isspace(static_cast<unsigned char>(str[end - 1])) != 0)) {
            --end;
        }
        return std::string(str.substr(begin, end - begin));
    }

    std::vector<std::string>
    split(std::string_view str, char delim)
    {
        std::vector<std::string>
            out;
        std::string cur;
        for (char chr : str)
        {
            if (chr == delim)
            {
                out.push_back(cur);
                cur.clear();
            }
            else {
                cur.push_back(chr);
            }
        }
        out.push_back(cur);
        return out;
    }

    bool iequals(std::string_view str_a, std::string_view str_b)
    {
        if (str_a.size() != str_b.size()) {
            return false;
        }
        for (size_t i = 0; i < str_a.size(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(str_a[i])) != std::tolower(static_cast<unsigned char>(str_b[i]))) {
                return false;
            }
        }
        return true;
    }
}
