# Kad Search Stuck at "Searching" - Debugging Guide

## Issue Description

The first Kad search (or subsequent searches started quickly) gets stuck at "Searching" state and never completes.

## Root Cause Analysis

### Identified Issues

**Issue 1: Race Condition in `SelectNodesForSearch`**

In `KadSearchEngine.cpp`, the `SelectNodesForSearch` method checks for `it->second.params.kadParams`:

```cpp
// Sort by distance to target (keyword hash)
if (it->second.params.kadParams) {
    SortContactsByDistance(it->second.params.kadParams->keywordHash, responsiveContacts);
}

// Create search requests
for (size_t i = 0; i < count; ++i) {
    KadSearchRequest request;
    request.searchId = searchId;
    request.targetNodeId = it->second.params.kadParams ?
        it->second.params.kadParams->keywordHash : "";
    // ...
}
```

**Problem:** `kadParams` is only set in `SendSearchToNodes`, but `SelectNodesForSearch` is called from:
1. `SendSearchToNodes` (line 532) - AFTER kadParams is set
2. `ResumeSearch` (line 152) - BEFORE kadParams is set again

When `ResumeSearch` is called, it calls `SendSearchToNodes` which calls `SelectNodesForSearch` BEFORE setting `kadParams`, causing the search to fail silently.

**Issue 2: No Nodes Available**

If no responsive contacts are found, the search is marked as FAILED but this might not be properly communicated to the UI.

**Issue 3: JumpStart Not Triggered**

The `jumpStarted` flag is set to `false` initially but never properly managed during the first search.

## Debugging Steps

### Step 1: Enable Debug Logging

Add detailed logging to track the search flow:

```cpp
// In KadSearchEngine::StartSearch
std::cout << "[KadSearchEngine] Starting Kad search " << searchId.ToString()
          << " for query: " << params.query << std::endl;
std::cout << "[KadSearchEngine] Kad connected: " << m_kadConnected << std::endl;
std::cout << "[KadSearchEngine] Contacts available: " << m_contacts.size() << std::endl;
```

### Step 2: Check Kad Connection Status

```cpp
// Add logging in SendSearchToNodes
std::cout << "[KadSearchEngine] Kad connected: " << m_kadConnected << std::endl;
std::cout << "[KadSearchEngine] Contacts count: " << m_contacts.size() << std::endl;
```

### Step 3: Verify Node Selection

```cpp
// Add logging in SelectNodesForSearch
std::cout << "[KadSearchEngine] SelectNodesForSearch called" << std::endl;
std::cout << "[KadSearchEngine] Responsive contacts: " << responsiveContacts.size() << std::endl;
std::cout << "[KadSearchEngine] kadParams set: " << (it->second.params.kadParams.has_value() ? "yes" : "no") << std::endl;
```

### Step 4: Check Search State Transitions

```cpp
// Add logging for state changes
std::cout << "[KadSearchEngine] Search " << searchId.ToString()
          << " state: " << static_cast<int>(data.state) << std::endl;
```

## Solution

### Fix 1: Ensure kadParams is Set Before SelectNodes

Modify `SendSearchToNodes` to set `kadParams` BEFORE calling `SelectNodesForSearch`:

```cpp
void KadSearchEngine::SendSearchToNodes(SearchId searchId, const SearchParams& params)
{
    auto it = m_searches.find(searchId);
    if (it == m_searches.end()) {
        return;
    }

    // Extract keywords
    auto keywords = ExtractKeywords(params.query);
    if (keywords.empty()) {
        std::cout << "[KadSearchEngine] No keywords extracted" << std::endl;
        return;
    }

    // Use first keyword for hash computation
    std::string keywordHash = ComputeKeywordHash(keywords[0]);
    it->second.params.kadParams = KadParams{keywordHash};  // SET THIS FIRST

    std::cout << "[KadSearchEngine] Keyword hash: " << keywordHash << std::endl;

    // Select nodes to query (NOW kadParams is set)
    SelectNodesForSearch(searchId, m_config.maxConcurrentRequests);

    // Build search request
    std::vector<uint8_t> requestData = BuildKadSearchPacket(params, keywordHash);

    // Send to selected nodes
    for (auto& request : it->second.activeRequests) {
        if (!request.completed) {
            request.requestData = requestData;
            request.sentTime = std::chrono::system_clock::now();

            std::cout << "[KadSearchEngine] Sent search request to node "
                      << request.contactNodeId.substr(0, 16) << "..."
                      << " (" << (request.contactIp >> 24) << "."
                      << ((request.contactIp >> 16) & 0xFF) << "."
                      << ((request.contactIp >> 8) & 0xFF) << "."
                      << (request.contactIp & 0xFF) << ":" << request.contactPort << ")" << std::endl;

            it->second.totalRequests++;
            it->second.contactedNodes.insert(request.contactNodeId);
        }
    }

    // Mark as completed if no nodes available
    if (it->second.activeRequests.empty()) {
        std::cout << "[KadSearchEngine] No nodes available for search "
                  << searchId.ToString() << std::endl;
        it->second.state = SearchState::FAILED;
        m_statistics.activeSearches--;
        m_statistics.failedSearches++;
    }
}
```

### Fix 2: Add Search Completion Event

When a search fails due to no available nodes, send an event to the UI:

```cpp
if (it->second.activeRequests.empty()) {
    std::cout << "[KadSearchEngine] No nodes available for search "
              << searchId.ToString() << std::endl;
    it->second.state = SearchState::FAILED;
    m_statistics.activeSearches--;
    m_statistics.failedSearches++;

    // Send failure event
    if (m_onKadResult) {
        SearchEvent event = SearchEvent::SearchFailed(
            searchId,
            "No Kad contacts available"
        );
        // Send event to UI
    }
}
```

### Fix 3: Initialize jumpStarted Flag

In `StartSearch`, initialize the `jumpStarted` flag:

```cpp
SearchId KadSearchEngine::StartSearch(const SearchParams& params)
{
    // ... existing code ...

    SearchData data;
    data.params = params;
    data.state = SearchState::STARTING;
    data.jumpStarted = false;  // Initialize this
    data.jumpStartCount = 0;   // Initialize this

    m_searches[searchId] = std::move(data);
    // ...
}
```

### Fix 4: Add Timeout Event

When a search times out, send an event to the UI:

```cpp
if (elapsed > data.params.timeout.count()) {
    std::cout << "[KadSearchEngine] Search " << searchId.ToString()
              << " timed out after " << elapsed << "ms" << std::endl;

    data.state = SearchState::COMPLETED;
    m_statistics.activeSearches--;
    m_statistics.completedSearches++;
    UpdateAverageSearchTime(elapsed);

    // Send completion event with timeout message
    if (m_onKadResult) {
        SearchEvent event = SearchEvent::SearchCompleted(searchId);
        // Send event to UI
    }
}
```

## Testing the Fix

### Test 1: Single Kad Search

```cpp
// Add test contacts
KadContact contact;
contact.nodeId = "0123456789abcdef0123456789abcdef";
contact.ip = 0x7F000001;
contact.port = 4671;
contact.isResponsive = true;
engine->AddContact(contact);

// Start search
SearchParams params;
params.type = SearchType::KADEMLIA;
params.query = "test";
params.timeout = std::chrono::seconds(30);

SearchId searchId = engine->StartSearch(params);

// Wait and check state
std::this_thread::sleep_for(std::chrono::seconds(1));
auto state = engine->GetSearchState(searchId);
EXPECT_EQ(state, SearchState::RUNNING);
```

### Test 2: Rapid Sequential Searches

```cpp
// Start multiple searches quickly
for (int i = 0; i < 5; ++i) {
    SearchParams params;
    params.type = SearchType::KADEMLIA;
    params.query = "query" + std::to_string(i);
    SearchId searchId = engine->StartSearch(params);
    searchIds.push_back(searchId);
}

// Verify all searches are running
for (const auto& searchId : searchIds) {
    auto state = engine->GetSearchState(searchId);
    EXPECT_TRUE(state == SearchState::RUNNING || state == SearchState::COMPLETED);
}
```

### Test 3: Resume After Pause

```cpp
SearchId searchId = engine->StartSearch(params);
engine->PauseSearch(searchId);
engine->ResumeSearch(searchId);

// Verify search resumes correctly
auto state = engine->GetSearchState(searchId);
EXPECT_EQ(state, SearchState::RUNNING);
```

## Verification Checklist

- [ ] kadParams is set before SelectNodesForSearch is called
- [ ] No nodes available sends failure event to UI
- [ ] Timeout sends completion event to UI
- [ ] jumpStarted flag is properly initialized
- [ ] Single search completes successfully
- [ ] Rapid sequential searches work correctly
- [ ] Resume after pause works correctly
- [ ] All state transitions are logged

## Additional Debugging Tips

### Check Contact List

```cpp
auto contacts = engine->GetContacts();
std::cout << "Total contacts: " << contacts.size() << std::endl;
for (const auto& contact : contacts) {
    std::cout << "  " << contact.nodeId.substr(0, 16) << "..."
              << " responsive: " << contact.isResponsive
              << " failed: " << contact.failedRequests << std::endl;
}
```

### Check Search State

```cpp
auto state = engine->GetSearchState(searchId);
std::cout << "Search state: " << static_cast<int>(state) << std::endl;
switch (state) {
    case SearchState::IDLE: std::cout << "IDLE" << std::endl; break;
    case SearchState::STARTING: std::cout << "STARTING" << std::endl; break;
    case SearchState::RUNNING: std::cout << "RUNNING" << std::endl; break;
    case SearchState::PAUSED: std::cout << "PAUSED" << std::endl; break;
    case SearchState::COMPLETED: std::cout << "COMPLETED" << std::endl; break;
    case SearchState::FAILED: std::cout << "FAILED" << std::endl; break;
    case SearchState::CANCELLED: std::cout << "CANCELLED" << std::endl; break;
}
```

### Check Statistics

```cpp
auto stats = engine->GetStatistics();
std::cout << "Total searches: " << stats.totalSearches << std::endl;
std::cout << "Active searches: " << stats.activeSearches << std::endl;
std::cout << "Completed searches: " << stats.completedSearches << std::endl;
std::cout << "Failed searches: " << stats.failedSearches << std::endl;
```

## Conclusion

The root cause is a race condition where `SelectNodesForSearch` is called before `kadParams` is set. The fix is to ensure `kadParams` is set BEFORE calling `SelectNodesForSearch` in `SendSearchToNodes`.

Additionally, proper event handling for failures and timeouts will ensure the UI is notified when searches complete or fail, preventing the "stuck at Searching" issue.
