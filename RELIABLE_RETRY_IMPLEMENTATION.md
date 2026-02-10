# Reliable Retry Implementation for aMule Search

## Overview

This implementation adds a comprehensive timeout and recovery mechanism to prevent search tabs from getting stuck in the [Searching] state. The system uses a dedicated `SearchTimeoutManager` to monitor all active searches and automatically recover stalled searches.

## Problem Statement

Searches in aMule could get stuck in the [Searching] state due to:

1. **Network failures**: Server or Kad network unresponsiveness
2. **Timer failures**: Search timers not starting or stopping properly
3. **Result delays**: No results arriving within expected timeframes
4. **State inconsistencies**: Search state not properly updated on completion

## Solution Architecture

### Core Components

#### 1. SearchTimeoutManager (`SearchTimeoutManager.h/cpp`)

A dedicated manager class that monitors all active searches:

**Features:**
- **Per-search timeout tracking**: Each search is registered with a timeout value based on type
- **Heartbeat mechanism**: Updates when search makes progress (results received)
- **Automatic timeout detection**: Checks for stalled searches every 10 seconds
- **Configurable timeouts**: Different timeouts for different search types:
  - Local searches: 30 seconds
  - Global searches: 2 minutes
  - Kad searches: 3 minutes
- **Callback system**: Notifies CSearchList when a search times out

**Key Methods:**
- `registerSearch()`: Register a search for monitoring
- `unregisterSearch()`: Remove a search from monitoring
- `updateHeartbeat()`: Update heartbeat when search makes progress
- `checkTimeouts()`: Check for timed out searches (called by heartbeat timer)
- `OnHeartbeatTimer()`: Timer event handler for periodic checks

#### 2. CSearchList Integration

**Integration Points:**

1. **Constructor**: Initialize SearchTimeoutManager with callbacks
2. **StartNewSearch()**: Register search with timeout manager
3. **StopSearch()**: Unregister search from timeout manager
4. **OnSearchComplete()**: Unregister search on completion
5. **AddToList()**: Update heartbeat when results are received
6. **OnSearchTimeout()**: Handle timeout events from manager
7. **ValidateAndRecoverSearches()**: Validate and recover stuck searches

**New Methods:**
- `OnSearchTimeout()`: Handles timeout events, triggers retry or marks as complete
- `ValidateAndRecoverSearches()`: Periodic validation of search states

## Implementation Details

### Search Lifecycle with Timeout Management

```
1. Search Started
   ↓
2. Register with SearchTimeoutManager
   ↓
3. Search Active (sending requests, receiving results)
   - Heartbeat updated on each result
   - Timer checks for timeout every 10 seconds
   ↓
4a. Results Received → Update Heartbeat
   ↓
4b. No Results → Timeout Check
   ↓
5. Timeout Detected
   ↓
6. OnSearchTimeout() Called
   ↓
7. Check if retry available
   ↓
8a. Retry Available → Trigger OnSearchRetry()
   ↓
8b. Max Retries Reached → Mark as Complete with No Results
```

### Timeout Handling Logic

```cpp
void CSearchList::OnSearchTimeout(uint32_t searchId, SearchTimeoutManager::SearchType type, const wxString& reason)
{
    // 1. Check if search is still active
    if (!state->isSearchActive()) {
        return; // Already handled
    }

    // 2. Check if we have results
    if (hasResults) {
        // Search has results but didn't complete properly
        OnSearchComplete(searchId, type, true);
    } else {
        // Search has no results and timed out
        if (currentRetry < maxRetries) {
            // Trigger retry
            OnSearchRetry(searchId, type, currentRetry + 1);
        } else {
            // Max retries reached, mark as complete with no results
            // This updates UI to show [No Results] instead of [Searching]
            state->setSearchActive(false);
            releaseId(searchId);
            removeSearchState(searchId);
            Notify_SearchLocalEnd();
        }
    }
}
```

### Recovery Mechanism

The `ValidateAndRecoverSearches()` method performs periodic validation:

1. **Check timeout manager registration**: Ensure all active searches are registered
2. **Check timer status**: Ensure search timers are running for ED2K searches
3. **Auto-recovery**: Fix inconsistencies automatically

```cpp
void CSearchList::ValidateAndRecoverSearches()
{
    for (auto& [searchId, state] : m_searchStates) {
        if (!state->isSearchActive()) continue;

        // Check if registered with timeout manager
        if (!m_timeoutManager->isSearchRegistered(searchId)) {
            // Register now
            m_timeoutManager->registerSearch(searchId, timeoutType);
        }

        // Check if timer is running
        if (!state->isTimerRunning()) {
            // Restart timer
            state->startTimer(timeoutMs, oneShot);
        }
    }
}
```

## Configuration

### Default Timeout Values

| Search Type | Timeout | Reason |
|-------------|---------|--------|
| Local Search | 30 seconds | Quick response expected from local server |
| Global Search | 2 minutes | Can take longer to query multiple servers |
| Kad Search | 3 minutes | Kad network can be slower |

### Heartbeat Interval

- **Default**: 10 seconds
- **Purpose**: Periodic check for stalled searches
- **Configurable**: Can be changed via `setHeartbeatInterval()`

## Benefits

1. **Prevents stuck searches**: Searches no longer get stuck in [Searching] state
2. **Automatic recovery**: Stalled searches are automatically recovered or retried
3. **Better user experience**: UI shows accurate state ([No Results] instead of [Searching])
4. **Comprehensive logging**: Detailed logging for debugging retry issues
5. **Configurable**: Timeouts can be adjusted based on network conditions
6. **Thread-safe**: All operations are thread-safe with proper mutex locking

## Testing Scenarios

### Scenario 1: Server Unresponsive
- **Expected**: Search times out after 30 seconds, retry triggered
- **Result**: UI shows [Retrying 1] instead of [Searching]

### Scenario 2: No Results Available
- **Expected**: Search times out, retries up to 3 times
- **Result**: UI shows [No Results] after max retries

### Scenario 3: Results Arrive Late
- **Expected**: Heartbeat updated, timeout prevented
- **Result**: Search completes normally with results

### Scenario 4: Network Connection Lost
- **Expected**: Search times out, retry attempted
- **Result**: If retries exhausted, shows [No Results]

## Logging

The implementation includes comprehensive logging:

```
SearchTimeoutManager: Registered search 123 (type=Local, timeout=30000ms)
SearchTimeoutManager: Updated heartbeat for search 123 (result received)
SearchTimeoutManager: Search 123 (Local) timed out after 30000 ms (timeout=30000ms)
OnSearchTimeout: SearchID=123, Type=0, Reason='Local search timed out - no response from server'
OnSearchRetry: SearchID=123, Type=0, RetryNum=1
```

## Files Modified

1. **New Files:**
   - `src/SearchTimeoutManager.h` - Timeout manager header
   - `src/SearchTimeoutManager.cpp` - Timeout manager implementation

2. **Modified Files:**
   - `src/SearchList.h` - Added timeout manager integration
   - `src/SearchList.cpp` - Integrated timeout manager into search lifecycle

## Future Enhancements

Potential improvements:

1. **Exponential backoff**: Increase retry delay after each failed retry
2. **Adaptive timeouts**: Adjust timeouts based on network conditions
3. **User notifications**: Show toast notifications for retry attempts
4. **Statistics dashboard**: Display retry statistics in UI
5. **Configurable retry limits**: Allow users to configure max retries per search type

## Backward Compatibility

This implementation is fully backward compatible:
- No changes to public APIs
- Existing search functionality unchanged
- Timeout mechanism is transparent to users
- Works with existing retry system (SearchAutoRetry)
