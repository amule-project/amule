# Unified Search Architecture - Integration Guide

**Version:** 1.0  
**Date:** 2026-02-12  
**Status:** Phase 2 Complete

---

## Overview

This guide provides detailed instructions for integrating the unified search architecture into the existing aMule codebase. It covers UI integration, feature flags, migration utilities, and testing.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Feature Flags](#feature-flags)
3. [UI Integration](#ui-integration)
4. [Migration](#migration)
5. [Testing](#testing)
6. [Rollback](#rollback)
7. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Required Components

1. **Unified Search Manager** - Phase 1 ✅
   - Core abstractions
   - Search engines
   - Command/event system

2. **Feature Flags** - Phase 2 ✅
   - Runtime feature toggling
   - Configuration management

3. **UI Adapter** - Phase 2 ✅
   - wxWidgets integration
   - Event routing

4. **Migration Utilities** - Phase 2 ✅
   - Old to new conversion
   - Rollback support

### Dependencies

- wxWidgets 3.0+
- C++17 or later
- Thread support (std::thread, std::mutex, etc.)

---

## Feature Flags

### Overview

Feature flags allow gradual rollout of the unified search architecture without disrupting existing functionality.

### Available Flags

| Flag Name | Default | Description |
|-----------|---------|-------------|
| `UNIFIED_SEARCH_MANAGER` | false | Enable unified search manager |
| `UNIFIED_LOCAL_SEARCH` | false | Enable unified local search |
| `UNIFIED_GLOBAL_SEARCH` | false | Enable unified global search |
| `UNIFIED_KAD_SEARCH` | false | Enable unified Kad search |
| `UNIFIED_SEARCH_UI` | false | Enable unified search UI |

### Using Feature Flags

#### Programmatic Control

```cpp
#include "search/unified/FeatureFlags.h"

// Enable a feature
search::FeatureFlags::Enable(search::FeatureFlags::UNIFIED_LOCAL_SEARCH);

// Check if feature is enabled
if (search::FeatureFlags::IsEnabled(search::FeatureFlags::UNIFIED_LOCAL_SEARCH)) {
    // Use unified search
} else {
    // Use old search
}

// Disable a feature
search::FeatureFlags::Disable(search::FeatureFlags::UNIFIED_LOCAL_SEARCH);
```

#### Environment Variables

```bash
export AMULE_FF_UNIFIED_SEARCH_MANAGER=1
export AMULE_FF_UNIFIED_LOCAL_SEARCH=1
export AMULE_FF_UNIFIED_GLOBAL_SEARCH=0
export AMULE_FF_UNIFIED_KAD_SEARCH=0
export AMULE_FF_UNIFIED_SEARCH_UI=1
```

#### Configuration File

Create `~/.amule/feature_flags.conf`:

```ini
# aMule Feature Flags
UNIFIED_SEARCH_MANAGER=true
UNIFIED_LOCAL_SEARCH=true
UNIFIED_GLOBAL_SEARCH=false
UNIFIED_KAD_SEARCH=false
UNIFIED_SEARCH_UI=false
```

Load from file:

```cpp
search::FeatureFlags::LoadFromFile("~/.amule/feature_flags.conf");
```

### Initialization

```cpp
#include "search/unified/FeatureFlags.h"

// Initialize from environment variables
search::FeatureFlags::InitializeFromEnvironment();

// Or load from file
search::FeatureFlags::LoadFromFile("~/.amule/feature_flags.conf");
```

---

## UI Integration

### SearchUIAdapter

The `SearchUIAdapter` class provides a convenient API for UI components to interact with the unified search system.

### Basic Usage

```cpp
#include "search/unified/manager/SearchUIAdapter.h"

// Create adapter
search::SearchUIAdapter* adapter = new search::SearchUIAdapter(parentWindow);

// Initialize
if (adapter->Initialize()) {
    // Set up callbacks
    adapter->SetOnSearchStarted([](search::SearchId searchId) {
        wxLogMessage("Search started: %s", searchId.ToString());
    });

    adapter->SetOnResultsReceived([](search::SearchId searchId,
                                      const std::vector<search::SearchResult>& results) {
        wxLogMessage("Received %zu results for search %s",
                    results.size(), searchId.ToString());
        // Update UI with results
    });

    adapter->SetOnSearchFailed([](search::SearchId searchId, const wxString& error) {
        wxLogError("Search %s failed: %s", searchId.ToString(), error);
    });

    // Start a search
    search::SearchParams params;
    params.type = search::SearchType::LOCAL;
    params.query = "test query";
    params.maxResults = 100;

    search::SearchId searchId = adapter->StartSearch(params);
    if (searchId.IsValid()) {
        wxLogMessage("Search started with ID: %s", searchId.ToString());
    }
}

// Shutdown when done
adapter->Shutdown();
delete adapter;
```

### Integrating with SearchTab

#### Step 1: Add SearchUIAdapter to SearchTab

```cpp
// SearchTab.h
#include "search/unified/manager/SearchUIAdapter.h"

class CSearchTab : public wxPanel {
private:
    search::SearchUIAdapter* m_searchAdapter;
    // ... other members
};
```

#### Step 2: Initialize in Constructor

```cpp
CSearchTab::CSearchTab(wxWindow* pParent)
    : wxPanel(pParent, -1)
{
    // ... existing initialization

    // Initialize unified search adapter
    m_searchAdapter = new search::SearchUIAdapter(this);
    if (m_searchAdapter->Initialize()) {
        SetupUnifiedSearchCallbacks();
    }
}
```

#### Step 3: Set Up Callbacks

```cpp
void CSearchTab::SetupUnifiedSearchCallbacks()
{
    m_searchAdapter->SetOnResultsReceived([this](
        search::SearchId searchId,
        const std::vector<search::SearchResult>& results
    ) {
        // Convert to UI format
        for (const auto& result : results) {
            AddResultToUI(result);
        }
    });

    m_searchAdapter->SetOnSearchProgress([this](
        search::SearchId searchId,
        int percentage,
        const wxString& message
    ) {
        // Update progress indicator
        UpdateProgress(searchId.ToString(), percentage, message);
    });

    m_searchAdapter->SetOnSearchCompleted([this](search::SearchId searchId) {
        // Mark search as complete
        MarkSearchComplete(searchId.ToString());
    });

    m_searchAdapter->SetOnSearchFailed([this](
        search::SearchId searchId,
        const wxString& error
    ) {
        // Show error to user
        ShowError(searchId.ToString(), error);
    });
}
```

#### Step 4: Convert Search Actions

```cpp
void CSearchTab::OnSearch(wxCommandEvent& event)
{
    wxString query = m_searchText->GetValue();

    if (query.IsEmpty()) {
        return;
    }

    if (m_searchAdapter && search::FeatureFlags::IsEnabled(
            search::FeatureFlags::UNIFIED_SEARCH_UI)) {
        // Use unified search
        search::SearchParams params;
        params.type = search::SearchType::LOCAL;
        params.query = query.ToStdString();
        params.maxResults = 500;

        search::SearchId searchId = m_searchAdapter->StartSearch(params);
        if (searchId.IsValid()) {
            AddSearchToList(searchId.ToString(), query);
        }
    } else {
        // Use old search system
        StartOldSearch(query);
    }
}

void CSearchTab::OnStopSearch(wxCommandEvent& event)
{
    if (m_searchAdapter && search::FeatureFlags::IsEnabled(
            search::FeatureFlags::UNIFIED_SEARCH_UI)) {
        search::SearchId searchId = GetSelectedSearchId();
        if (searchId.IsValid()) {
            m_searchAdapter->StopSearch(searchId);
        }
    } else {
        // Use old search system
        StopOldSearch();
    }
}
```

### Result Conversion

```cpp
void CSearchTab::AddResultToUI(const search::SearchResult& result)
{
    // Convert unified result to UI format
    CSearchFile* file = new CSearchFile();

    file->SetFileName(wxString::FromUTF8(result.fileName.c_str()));
    file->SetFileSize(result.fileSize);
    file->SetFileHash(CMD4Hash(result.fileHash));
    file->SetSources(result.availability);

    // Add metadata
    for (const auto& [key, value] : result.metadata) {
        file->AddTag(CTagString(wxString::FromUTF8(key.c_str()),
                              wxString::FromUTF8(value.c_str())));
    }

    // Add to UI list
    m_searchListCtrl->AddResult(file);
    delete file;
}
```

---

## Migration

### Overview

The migration utilities help transition from the old search architecture to the new unified architecture.

### Migration Process

#### Step 1: Validate Readiness

```cpp
#include "search/unified/SearchMigration.h"

if (search::SearchMigration::ValidateMigrationReadiness()) {
    // Proceed with migration
} else {
    wxLogError("Migration validation failed");
    return;
}
```

#### Step 2: Migrate Active Searches

```cpp
search::SearchMigration::ProgressCallback progress = [](
    uint32_t current,
    uint32_t total,
    const std::string& message
) {
    int percentage = (current * 100) / total;
    UpdateProgressDialog(percentage, message);
};

auto report = search::SearchMigration::MigrateAllActiveSearches(progress);

if (report.status == search::SearchMigration::Status::COMPLETED) {
    wxLogMessage("Migration completed successfully");
    wxLogMessage("  Migrated: %u searches", report.searchesMigrated);
    wxLogMessage("  Failed: %u searches", report.searchesFailed);
} else {
    wxLogError("Migration failed: %s", report.message);
}
```

#### Step 3: Enable Feature Flags

```cpp
search::FeatureFlags::Enable(search::FeatureFlags::UNIFIED_SEARCH_MANAGER);
search::FeatureFlags::Enable(search::FeatureFlags::UNIFIED_LOCAL_SEARCH);
search::FeatureFlags::Enable(search::FeatureFlags::UNIFIED_SEARCH_UI);
```

### Custom Migration

For fine-grained control over migration:

```cpp
// Migrate individual search
uint32_t oldSearchId = 12345;
int oldType = 0;  // Local search
std::string oldParams = "music;1024;104857600;mp3,flac";

search::SearchId newSearchId = search::SearchMigration::MigrateSearch(
    oldSearchId,
    oldType,
    oldParams,
    progressCallback
);

if (newSearchId.IsValid()) {
    wxLogMessage("Migrated search %u to %s",
                oldSearchId, newSearchId.ToString());
}
```

### Rollback

If issues are discovered after migration:

```cpp
// Get current report
auto report = search::SearchMigration::GenerateReport();

// Rollback
if (search::SearchMigration::RollbackMigration(report)) {
    wxLogMessage("Migration rolled back successfully");
    
    // Disable feature flags
    search::FeatureFlags::Disable(search::FeatureFlags::UNIFIED_SEARCH_MANAGER);
    search::FeatureFlags::Disable(search::FeatureFlags::UNIFIED_LOCAL_SEARCH);
    search::FeatureFlags::Disable(search::FeatureFlags::UNIFIED_SEARCH_UI);
} else {
    wxLogError("Rollback failed");
}
```

---

## Testing

### Unit Tests

Run unit tests for core abstractions:

```bash
cd build
./SearchIdTest
./SearchResultTest
./SearchParamsTest
./SearchCommandTest
./SearchEventTest
```

### Integration Tests

Run integration tests:

```bash
cd build
./LocalSearchIntegrationTest
./MultiSearchIntegrationTest
```

### Manual Testing

#### Test Case 1: Basic Local Search

1. Enable feature flags
2. Start local search
3. Verify results appear
4. Stop search
5. Verify cleanup

#### Test Case 2: Multiple Concurrent Searches

1. Start 5 different searches
2. Verify all searches run independently
3. Verify no interference between searches
4. Stop all searches
5. Verify proper cleanup

#### Test Case 3: Feature Flag Toggle

1. Run search with feature disabled (old system)
2. Enable feature flag
3. Run search with feature enabled (new system)
4. Compare results
5. Disable feature flag
6. Verify fallback to old system

#### Test Case 4: Migration

1. Start with old system active
2. Validate migration readiness
3. Migrate active searches
4. Verify searches continue in new system
5. Test rollback if needed

---

## Rollback

### Immediate Rollback

If critical issues are discovered:

```cpp
// Disable all unified search features
search::FeatureFlags::Disable(search::FeatureFlags::UNIFIED_SEARCH_MANAGER);
search::FeatureFlags::Disable(search::FeatureFlags::UNIFIED_LOCAL_SEARCH);
search::FeatureFlags::Disable(search::FeatureFlags::UNIFIED_GLOBAL_SEARCH);
search::FeatureFlags::Disable(search::FeatureFlags::UNIFIED_KAD_SEARCH);
search::FeatureFlags::Disable(search::FeatureFlags::UNIFIED_SEARCH_UI);

// Restart application
```

### Automated Rollback

```cpp
// Get migration report
auto report = search::SearchMigration::GenerateReport();

// Perform rollback
if (search::SearchMigration::RollbackMigration(report)) {
    wxLogMessage("Automated rollback successful");
} else {
    wxLogError("Automated rollback failed - manual intervention required");
}
```

### Rollback Verification

After rollback:

1. Verify old search system works correctly
2. Verify no data loss
3. Verify no search result corruption
4. Check logs for errors

---

## Troubleshooting

### Issue: Search Not Starting

**Symptoms:** Search command sent but no events received

**Possible Causes:**
- Feature flag not enabled
- UnifiedSearchManager not initialized
- Worker thread crashed

**Solutions:**
```cpp
// Check feature flags
if (!search::FeatureFlags::IsEnabled(search::FeatureFlags::UNIFIED_SEARCH_MANAGER)) {
    wxLogError("Unified search not enabled");
}

// Check initialization
if (!m_searchAdapter->IsInitialized()) {
    wxLogError("Search adapter not initialized");
}
```

### Issue: No Results Received

**Symptoms:** Search completes but no results

**Possible Causes:**
- Search parameters invalid
- No files match criteria
- Shared files list empty

**Solutions:**
```cpp
// Validate parameters
if (!params.IsValid()) {
    wxLogError("Invalid search parameters");
}

// Check shared files
if (theApp->sharedfiles->GetCount() == 0) {
    wxLogWarning("No shared files to search");
}
```

### Issue: Memory Leak

**Symptoms:** Memory usage increases over time

**Possible Causes:**
- Search results not cleaned up
- Event callbacks not unregistered
- Worker thread not shutting down

**Solutions:**
```cpp
// Always shutdown adapter
m_searchAdapter->Shutdown();
delete m_searchAdapter;

// Unregister event handlers
Unbind(wxEVT_SEARCH_EVENT, &SearchUIAdapter::OnSearchEvent, this);
```

### Issue: Race Conditions

**Symptoms:** Crashes or data corruption

**Possible Causes:**
- Accessing search results from wrong thread
- Modifying state while iterating
- Not waiting for async operations

**Solutions:**
- All search operations in search thread
- All UI updates in UI thread
- Use proper event callbacks

---

## Performance Tips

### 1. Batch Operations

Instead of sending many individual commands:

```cpp
// Bad: Send commands one by one
for (int i = 0; i < 100; ++i) {
    manager->SendCommand(SearchCommand::StartSearch(params));
}

// Good: Batch if possible (future enhancement)
// manager->SendBatchCommands(commands);
```

### 2. Limit Result Size

```cpp
// Limit results to reduce memory usage
params.maxResults = 100;  // Instead of 500
```

### 3. Clean Up Old Searches

```cpp
// Periodically clean up completed searches
auto stats = manager->GetStatistics();
if (stats.completedSearches > 100) {
    manager->SendCommand(SearchCommand::CancelAllSearches());
}
```

---

## Next Steps

After successful integration:

1. **Monitor Performance:** Track search latency and memory usage
2. **Gather Feedback:** Collect user feedback on new search experience
3. **Optimize:** Address any performance issues discovered
4. **Extend:** Implement full Global and Kad search functionality
5. **Deprecate Old Code:** Remove old search implementation once stable

---

## Support

For issues or questions:

1. Check this integration guide
2. Review the main README
3. Consult the architecture design document
4. Check unit and integration tests for examples

---

## Appendix: Feature Flag Best Practices

### DO:

- Use feature flags for all new unified search features
- Start with flags disabled by default
- Enable flags gradually for beta testers
- Monitor logs for issues when flags are enabled
- Document which flags are required for which features

### DON'T:

- Enable all flags at once
- Ship with flags enabled for all users
- Skip rollback planning
- Remove old code before confirming new code works

---

**Document Version:** 1.0  
**Last Updated:** 2026-02-12  
**Related:** [README.md](../README.md), [Architecture Design](../../../../docs/SEARCH_ARCHITECTURE_REDESIGN.md)
