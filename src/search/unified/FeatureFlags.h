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

#ifndef FEATURE_FLAGS_H
#define FEATURE_FLAGS_H

#include <string>
#include <optional>

namespace search {

/**
 * Feature flag system for gradual rollout of unified search architecture
 * Allows enabling/disabling features at runtime
 */
class FeatureFlags {
public:
    // Feature names
    static const char* const UNIFIED_SEARCH_MANAGER;
    static const char* const UNIFIED_LOCAL_SEARCH;
    static const char* const UNIFIED_GLOBAL_SEARCH;
    static const char* const UNIFIED_KAD_SEARCH;
    static const char* const UNIFIED_SEARCH_UI;

    /**
     * Check if a feature is enabled
     * @param featureName Name of the feature
     * @return true if feature is enabled, false otherwise
     */
    static bool IsEnabled(const std::string& featureName);

    /**
     * Enable a feature
     * @param featureName Name of the feature to enable
     */
    static void Enable(const std::string& featureName);

    /**
     * Disable a feature
     * @param featureName Name of the feature to disable
     */
    static void Disable(const std::string& featureName);

    /**
     * Toggle a feature
     * @param featureName Name of the feature to toggle
     * @return New state of the feature
     */
    static bool Toggle(const std::string& featureName);

    /**
     * Get all enabled features
     * @return Set of enabled feature names
     */
    static std::set<std::string> GetEnabledFeatures();

    /**
     * Load feature flags from configuration
     * @param configPath Path to configuration file
     * @return true if loaded successfully
     */
    static bool LoadFromFile(const std::string& configPath);

    /**
     * Save feature flags to configuration
     * @param configPath Path to configuration file
     * @return true if saved successfully
     */
    static bool SaveToFile(const std::string& configPath);

    /**
     * Reset all features to default state
     */
    static void ResetToDefaults();

    /**
     * Initialize feature flags from environment variables
     * Environment variables: AMULE_FF_<FEATURE_NAME>=1 or 0
     */
    static void InitializeFromEnvironment();

private:
    // Internal storage for feature states
    static std::unordered_map<std::string, bool> s_features;
    static std::mutex s_mutex;

    /**
     * Ensure feature exists in storage
     * @param featureName Name of the feature
     * @param defaultValue Default state if feature doesn't exist
     */
    static void EnsureFeature(const std::string& featureName, bool defaultValue);
};

} // namespace search

#endif // FEATURE_FLAGS_H
