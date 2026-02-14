# Kad Search Stuck at "Searching" - Debugging Guide

## Problem Analysis

After completing the migration, Kad searches are still stuck at "Searching" state, especially when starting global/local searches quickly.

## Root Cause Analysis

### Issue 1: No Completion Notification from Kad Network

**Location**: `/home/eli/git/amule/src/search/KadSearchController.cpp`

**Problem**: The Kad search controller starts the search but doesn't receive completion notifications from the Kad network.

**Code Flow**:
1. `KadSearchController::startSearch()` is called
2. It calls `Kademlia::CSearchManager::PrepareFindKeywords()`
3. Kad network processes the search
4. **BUT**: There's no mechanism to notify the controller when Kad search completes

**Missing**: The Kad search controller needs to be notified when:
- Kad search completes (all nodes queried)
- Kad search times out
- Kad search receives results

### Issue 2: State Not Updated on Kad Completion

**Location**: `/home/eli/git/amule/src/kademlia/kademlia/Search.cpp`

**Problem**: When Kad search completes (via `PrepareToStop()`), it doesn't notify the KadSearchController.

**Current Behavior**:
```cpp
void CSearch::PrepareToStop() throw()
{
    // Sets m_stopping = true
    // Adjusts created time for cleanup
    // BUT: No notification to KadSearchController!
}
```

### Issue 3: No Timeout Handling for Kad Searches

**Location**: `/home/eli/git/amule/src/search/SearchTimeoutManager.cpp`

**Problem**: The SearchTimeoutManager is registered but doesn't have a callback to handle timeouts.

**Current State**:
- Searches are registered with timeout manager
- Timeout manager has heartbeat timer
- **BUT**: No callback is set to handle timeout events

### Issue 4: Search State Never Transitions from "Searching"

**Location**: `/home/eli/git/amule/src/SearchStateManager.cpp`

**Problem**: The `EndSearch()` method returns without updating state when retry count < MAX_RETRIES.

**Code**:
```cpp
void SearchStateManager::EndSearch(uint32_t searchId)
{
    // ...
    if (data.retryCount < MAX_RETRIES) {
        // Don't set to NO_RESULTS yet, let retry mechanism handle it
        return;  // <-- Returns without updating state!
    }
    // ...
}
```

## Debugging Steps

### Step 1: Enable Comprehensive Logging

Add detailed logging to track Kad search lifecycle:

```cpp
// In KadSearchController::startSearch()
AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::startSearch: Starting search ID=%u, Keyword='%s'"))
    % searchId % params.strKeyword);
AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::startSearch: Kad running=%d"))
    % Kademlia::CKademlia::IsRunning());

// After creating Kad search
AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::startSearch: Kad search created with ID=%u"))
    % kadSearchId);
AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::startSearch: Kad search stored in m_kadSearch=%p"))
    % m_kadSearch);
```

### Step 2: Track Kad Search Lifecycle

Monitor when Kad search completes:

```cpp
// In KadSearchController::stopSearch()
AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::stopSearch: Stopping search ID=%u"))
    % searchId);
AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::stopSearch: m_kadSearch=%p"))
    % m_kadSearch);

if (m_kadSearch) {
    uint32_t kadSearchId = m_kadSearch->GetSearchID();
    AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::stopSearch: Kad search ID=%u"))
        % kadSearchId);
    // ...
}
```

### Step 3: Check State Transitions

Log all state changes:

```cpp
// In SearchStateManager::UpdateState()
AddDebugLogLineC(logSearch, CFormat(wxT("SearchStateManager::UpdateState: ID=%u, %d -> %d"))
    % searchId % (int)oldState % (int)newState);
```

### Step 4: Monitor Timeout Events

Add timeout callback to UnifiedSearchManager:

```cpp
// In UnifiedSearchManager constructor
m_timeoutManager.setTimeoutCallback([this](uint32_t searchId, SearchTimeoutManager::SearchType type, const wxString& reason) {
    AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Search %u timed out: %s"))
        % searchId % reason);
    
    // Handle timeout
    wxMutexLocker lock(m_mutex);
    auto it = m_controllers.find(searchId);
    if (it != m_controllers.end()) {
        // Mark search as completed
        bool hasResults = (it->second->getResultCount() > 0);
        markSearchComplete(searchId, hasResults);
    }
});
```

### Step 5: Check Kad Search Results

Monitor when Kad results arrive:

```cpp
// In KadSearchController::handleResults()
AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::handleResults: Received %zu results for search ID=%u"))
    % results.size() % searchId);
```

## Immediate Fixes Required

### Fix 1: Add Kad Search Completion Callback

**Location**: `/home/eli/git/amule/src/search/KadSearchController.cpp`

Add a completion callback that's called when Kad search finishes:

```cpp
void KadSearchController::onKadSearchComplete(uint32_t kadSearchId)
{
    uint32_t searchId = m_model->getSearchId();
    AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::onKadSearchComplete: Kad search ID=%u, our ID=%u"))
        % kadSearchId % searchId);
    
    // Check if we have any results
    bool hasResults = (m_model->getResultCount() > 0);
    
    // Update state
    m_model->setSearchState(hasResults ? SearchState::Completed : SearchState::Completed);
    
    // Notify completion
    notifySearchCompleted(searchId);
    
    // Clear Kad search reference
    m_kadSearch = nullptr;
    
    AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::onKadSearchComplete: Search %u marked as complete (hasResults=%d)"))
        % searchId % hasResults);
}
```

### Fix 2: Set Timeout Callback in UnifiedSearchManager

**Location**: `/home/eli/git/amule/src/search/UnifiedSearchManager.cpp`

Add timeout callback in constructor:

```cpp
UnifiedSearchManager::UnifiedSearchManager()
{
    // Set timeout callback
    m_timeoutManager.setTimeoutCallback([this](uint32_t searchId, SearchTimeoutManager::SearchType type, const wxString& reason) {
        AddDebugLogLineC(logSearch, CFormat(wxT("UnifiedSearchManager: Search %u timed out: %s"))
            % searchId % reason);
        
        wxMutexLocker lock(m_mutex);
        auto it = m_controllers.find(searchId);
        if (it != m_controllers.end()) {
            // Get result count before stopping
            bool hasResults = (it->second->getResultCount() > 0);
            
            // Stop the search
            it->second->stopSearch();
            
            // Mark as complete
            markSearchComplete(searchId, hasResults);
        }
    });
}
```

### Fix 3: Update SearchStateManager::EndSearch()

**Location**: `/home/eli/git/amule/src/SearchStateManager.cpp`

Always update state, even when retrying:

```cpp
void SearchStateManager::EndSearch(uint32_t searchId)
{
    bool shouldUpdateState = false;
    bool hasResults = false;
    bool shouldRetry = false;
    int retryCount = 0;
    
    {
        wxMutexLocker lock(m_mutex);
        
        SearchMap::iterator it = m_searches.find(searchId);
        if (it == m_searches.end()) {
            return;
        }

        SearchData& data = it->second;

        if (data.shownCount > 0 || data.hiddenCount > 0) {
            shouldUpdateState = true;
            hasResults = true;
        } else {
            // No results - check if we should retry
            if (data.retryCount < MAX_RETRIES) {
                // Update to Retrying state
                shouldUpdateState = true;
                shouldRetry = true;
            } else {
                // Max retries reached, set to NO_RESULTS
                shouldUpdateState = true;
                hasResults = false;
            }
        }
        retryCount = data.retryCount;
    }
    
    // Update state outside the lock
    if (shouldUpdateState) {
        if (hasResults) {
            UpdateState(searchId, STATE_HAS_RESULTS);
        } else if (shouldRetry) {
            UpdateState(searchId, STATE_RETRYING);
            // Trigger retry
            if (m_onSearchCompleted) {
                // Don't call completion, trigger retry instead
            }
        } else {
            UpdateState(searchId, STATE_NO_RESULTS);
        }
    }
}
```

### Fix 4: Add Kad Search Monitoring

**Location**: `/home/eli/git/amule/src/search/KadSearchController.cpp`

Add periodic monitoring of Kad search state:

```cpp
void KadSearchController::checkKadSearchState()
{
    if (!m_kadSearch) {
        // Search object is gone, mark as complete
        uint32_t searchId = m_model->getSearchId();
        bool hasResults = (m_model->getResultCount() > 0);
        
        AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::checkKadSearchState: Kad search object is null, marking search %u as complete"))
            % searchId);
        
        m_model->setSearchState(SearchState::Completed);
        notifySearchCompleted(searchId);
        return;
    }
    
    // Check if Kad search is stopping
    if (m_kadSearch->Stopping()) {
        AddDebugLogLineC(logSearch, CFormat(wxT("KadSearchController::checkKadSearchState: Kad search is stopping")));
        // Wait a bit for cleanup, then mark as complete
    }
}
```

## Testing Procedure

### Test 1: Single Kad Search

1. Start aMule
2. Connect to Kad network
3. Start a Kad search
4. Monitor logs for:
   ```
   KadSearchController::startSearch: Starting search ID=X
   KadSearchController::startSearch: Kad running=1
   KadSearchController::startSearch: Kad search created with ID=Y
   ```

5. Wait for completion or timeout
6. Check if state transitions from "Searching" to "No Results" or has results

### Test 2: Rapid Sequential Searches

1. Start aMule
2. Connect to both ED2K and Kad networks
3. Start Global search
4. Immediately start Local search
5. Immediately start Kad search
6. Monitor logs for state transitions
7. Check if Kad search gets stuck

### Test 3: Kad Search Timeout

1. Start a Kad search
2. Wait for timeout (3 minutes default)
3. Check if timeout callback is triggered
4. Check if state updates to "No Results"

## Expected Behavior After Fixes

1. Kad searches should transition from "Searching" to either:
   - "Has Results" (if results found)
   - "No Results" (if no results and retry limit reached)
   - "Retrying" (if no results and retry count < MAX_RETRIES)

2. Timeout events should be properly handled

3. State should always be updated, never left in "Searching"

4. Multiple concurrent searches should not interfere with each other

## Additional Debugging Tools

### Enable Debug Logging

Add to your configuration or command line:
```bash
amule --verbose --log-level=debug
```

### Monitor Search States

Add periodic monitoring in SearchDlg:
```cpp
void CSearchDlg::OnTimeoutCheck(wxTimerEvent& event)
{
    // Check for stuck searches
    for (auto& searchId : m_stateManager.getActiveSearchIds()) {
        SearchState state = m_stateManager.GetSearchState(searchId);
        if (state == STATE_SEARCHING) {
            // Check elapsed time
            time_t elapsed = getSearchElapsedTime(searchId);
            if (elapsed > TIMEOUT_THRESHOLD) {
                AddDebugLogLineC(logSearch, CFormat(wxT("Search %u stuck in Searching for %ld seconds"))
                    % searchId % elapsed);
                
                // Force completion
                m_stateManager.EndSearch(searchId);
            }
        }
    }
}
```

### Check Kad Network Status

```cpp
if (!Kademlia::CKademlia::IsRunning()) {
    AddDebugLogLineC(logSearch, wxT("Kad network is not running!"));
    // Show error to user
}
```

## Conclusion

The Kad search stuck issue is caused by:
1. No completion notification from Kad network
2. No timeout callback handling
3. State not being updated when retry count < MAX_RETRIES
4. No monitoring of Kad search state

Implementing the fixes above should resolve the issue and ensure Kad searches complete properly.
