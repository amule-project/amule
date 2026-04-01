//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2024 aMule Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#include "FeatureFlags.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

namespace search {

// Feature name constants
const char* const FeatureFlags::UNIFIED_SEARCH_MANAGER = "UNIFIED_SEARCH_MANAGER";
const char* const FeatureFlags::UNIFIED_LOCAL_SEARCH = "UNIFIED_LOCAL_SEARCH";
const char* const FeatureFlags::UNIFIED_GLOBAL_SEARCH = "UNIFIED_GLOBAL_SEARCH";
const char* const FeatureFlags::UNIFIED_KAD_SEARCH = "UNIFIED_KAD_SEARCH";
const char* const FeatureFlags::UNIFIED_SEARCH_UI = "UNIFIED_SEARCH_UI";

// Static members
std::unordered_map<std::string, bool> FeatureFlags::s_features;
std::mutex FeatureFlags::s_mutex;

bool FeatureFlags::IsEnabled(const std::string& featureName)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_features.find(featureName);
    return it != s_features.end() && it->second;
}

void FeatureFlags::Enable(const std::string& featureName)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_features[featureName] = true;
}

void FeatureFlags::Disable(const std::string& featureName)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_features[featureName] = false;
}

bool FeatureFlags::Toggle(const std::string& featureName)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    bool newState = !s_features[featureName];
    s_features[featureName] = newState;
    return newState;
}

std::set<std::string> FeatureFlags::GetEnabledFeatures()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    std::set<std::string> enabled;
    for (const auto& [name, enabled] : s_features) {
        if (enabled) {
            enabled.insert(name);
        }
    }
    return enabled;
}

bool FeatureFlags::LoadFromFile(const std::string& configPath)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    std::ifstream file(configPath);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse format: FEATURE_NAME=true/false
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string name = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // Trim whitespace
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);

            // Convert to lowercase
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);

            // Parse boolean value
            if (value == "true" || value == "1" || value == "yes" || value == "on") {
                s_features[name] = true;
            } else if (value == "false" || value == "0" || value == "no" || value == "off") {
                s_features[name] = false;
            }
        }
    }

    return true;
}

bool FeatureFlags::SaveToFile(const std::string& configPath)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    std::ofstream file(configPath);
    if (!file.is_open()) {
        return false;
    }

    // Write header
    file << "# aMule Unified Search Feature Flags\n";
    file << "# Format: FEATURE_NAME=true/false\n";
    file << "# Generated automatically - do not edit manually\n\n";

    // Write all features
    for (const auto& [name, enabled] : s_features) {
        file << name << "=" << (enabled ? "true" : "false") << "\n";
    }

    return true;
}

void FeatureFlags::ResetToDefaults()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    s_features.clear();

    // Set default values
    s_features[UNIFIED_SEARCH_MANAGER] = false;  // Disabled by default
    s_features[UNIFIED_LOCAL_SEARCH] = false;     // Disabled by default
    s_features[UNIFIED_GLOBAL_SEARCH] = false;    // Disabled by default
    s_features[UNIFIED_KAD_SEARCH] = false;      // Disabled by default
    s_features[UNIFIED_SEARCH_UI] = false;        // Disabled by default
}

void FeatureFlags::InitializeFromEnvironment()
{
    // Initialize defaults first
    ResetToDefaults();

    // Check environment variables
    const char* envManager = std::getenv("AMULE_FF_UNIFIED_SEARCH_MANAGER");
    const char* envLocal = std::getenv("AMULE_FF_UNIFIED_LOCAL_SEARCH");
    const char* envGlobal = std::getenv("AMULE_FF_UNIFIED_GLOBAL_SEARCH");
    const char* envKad = std::getenv("AMULE_FF_UNIFIED_KAD_SEARCH");
    const char* envUI = std::getenv("AMULE_FF_UNIFIED_SEARCH_UI");

    if (envManager) {
        s_features[UNIFIED_SEARCH_MANAGER] = (std::string(envManager) == "1" || 
                                                     std::string(envManager) == "true");
    }

    if (envLocal) {
        s_features[UNIFIED_LOCAL_SEARCH] = (std::string(envLocal) == "1" || 
                                                 std::string(envLocal) == "true");
    }

    if (envGlobal) {
        s_features[UNIFIED_GLOBAL_SEARCH] = (std::string(envGlobal) == "1" || 
                                                  std::string(envGlobal) == "true");
    }

    if (envKad) {
        s_features[UNIFIED_KAD_SEARCH] = (std::string(envKad) == "1" || 
                                              std::string(envKad) == "true");
    }

    if (envUI) {
        s_features[UNIFIED_SEARCH_UI] = (std::string(envUI) == "1" || 
                                           std::string(envUI) == "true");
    }
}

void FeatureFlags::EnsureFeature(const std::string& featureName, bool defaultValue)
{
    if (s_features.find(featureName) == s_features.end()) {
        s_features[featureName] = defaultValue;
    }
}

} // namespace search
