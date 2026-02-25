# Search System Architecture Redesign

**Version:** 1.0  
**Date:** 2026-02-12  
**Status:** Design Phase  
**Author:** Architecture Team  

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Current Architecture Problems](#current-architecture-problems)
3. [Proposed Solution](#proposed-solution)
4. [Core Abstractions](#core-abstractions)
5. [Thread Management](#thread-management)
6. [Data Flow](#data-flow)
7. [Implementation Plan](#implementation-plan)
8. [Migration Strategy](#migration-strategy)
9. [Testing Strategy](#testing-strategy)
10. [Performance Considerations](#performance-considerations)

---

## Executive Summary

This document outlines a complete architectural redesign of the aMule search system to unify local (ED2K), global (server-based), and Kad (Kademlia) searches under a single abstraction layer. The redesign addresses critical race conditions in the current implementation and establishes clear thread boundaries with centralized thread management.

### Key Objectives

- **Eliminate Race Conditions:** Fix all concurrency issues in the current Kademlia search implementation
- **Unified Abstraction:** Provide a consistent interface for all search types
- **Single-Threaded Search Operations:** All search logic runs in a dedicated search thread
- **Clear Boundaries:** Well-defined interfaces between UI, search, and network layers
- **Maintainability:** Clean separation of concerns with testable components

### Success Criteria

- Zero race conditions in search operations
- Consistent API across all search types
- No performance regression compared to current implementation
- Comprehensive test coverage for all search types

---

## Current Architecture Problems

### Problem 1: Lock Scope Too Narrow

**Location:** `src/kademlia/kademlia/SearchManager.cpp:488-520`

```cpp
void CSearchManager::ProcessResponse(const CUInt128& target, uint32_t fromIP, uint16_t fromPort, ContactList *results)
{
    CSearch *s = NULL;
    {
        std::lock_guard<std::mutex> lock(m_searchesMutex);
        SearchMap::const_iterator it = m_searches.find(target);
        if (it != m_searches.end()) {
            s = it->second;
        }
    }
    // LOCK RELEASED HERE - s is now unsafe!

    if (s == NULL) {
        DeleteContents(*results);
    } else {
        s->ProcessResponse(fromIP, fromPort, results); // Potential use-after-free
    }
    delete results;
}
```

**Issue:** The mutex protecting `m_searches` is released before accessing the `CSearch` object, creating a use-after-free vulnerability.

### Problem 2: No Protection on CSearch Internal State

**All CSearch member variables accessed without locks:**

- `ContactMap m_possible` - Modified by `Go()`, `ProcessResponse()`, `JumpStart()`, `~CSearch()`
- `ContactMap m_tried` - Modified by `Go()`, `ProcessResponse()`, `JumpStart()`, `~CSearch()`
- `ContactMap m_best` - Modified by `Go()`, `ProcessResponse()`, `JumpStart()`, `~CSearch()`
- `ContactMap m_inUse` - Modified by `Go()`, `ProcessResponse()`, `JumpStart()`, `~CSearch()`
- `ContactMap m_responded` - Modified by `ProcessResponse()`, `JumpStart()`, `~CSearch()`
- `ContactList m_delete` - Modified by `ProcessResponse()`, `~CSearch()`
- `uint32_t m_answers` - Modified by `ProcessResponse()`, `ProcessResult()`, `ProcessPublishResult()`
- `bool m_stopping` - Modified by `PrepareToStop()`, accessed by `Go()`, `JumpStart()`, `SendFindValue()`, `StorePacket()`

### Problem 3: Iterator Invalidations During Iteration

**Location:** `src/kademlia/kademlia/SearchManager.cpp:259-410`

```cpp
void CSearchManager::JumpStart()
{
    std::lock_guard<std::mutex> lock(m_searchesMutex);
    SearchMap::iterator next_it = m_searches.begin();
    while (next_it != m_searches.end()) {
        SearchMap::iterator current_it = next_it++;
        // ... calls current_it->second->JumpStart()
        // If JumpStart() modifies m_searches (through callbacks), 
        // iterators become invalid!
    }
}
```

### Problem 4: Contact Reference Counting Without Memory Ordering

**Location:** `src/kademlia/routing/Contact.h:123-124`

```cpp
void IncUse() throw() { m_inUse++; }
void DecUse() throw() { if (m_inUse) m_inUse--; }
```

**Issues:**
- Not atomic operations (no `std::atomic`)
- No memory barriers
- Can be corrupted even in single-threaded mode if accessed from different call contexts

### Problem 5: Inconsistent Threading Model

**The system is fundamentally single-threaded** (wxWidgets main loop), but includes:
- Mutex added to `SearchManager::m_searches` (recent addition)
- `CContact::m_inUse` reference counting suggests sharing
- `ParallelSearch` classes exist but aren't integrated
- Signs of abandoned multi-threading attempt

This creates a dangerous state where the code appears thread-safe but actually isn't.

---

## Proposed Solution

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    UI Layer (Main Thread)                  │
│  - SearchTab UI                                             │
│  - User interactions                                        │
│  - Result display                                           │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              Search Abstraction Layer                       │
│  - UnifiedSearchManager (search thread)                     │
│  - Command queues (UI → Search)                             │
│  - Event dispatchers (Search → UI)                          │
│  - Result aggregation                                       │
└─────────────────────────────────────────────────────────────┘
                              │
            ┌─────────────────┼─────────────────┐
            ▼                 ▼                 ▼
┌──────────────────┐ ┌──────────────────┐ ┌──────────────────┐
│  Local Search    │ │  Global Search   │ │   Kad Search     │
│  Implementation  │ │  Implementation  │ │  Implementation  │
│  (ED2K local)    │ │  (ED2K server)   │ │  (Kademlia)      │
└──────────────────┘ └──────────────────┘ └──────────────────┘
```

### Core Design Principles

1. **Single Search Thread:** All search operations (local, global, Kad) run in a dedicated thread
2. **Main UI Thread:** Handles user interactions and UI updates only
3. **Network Threads:** Handle packet reception only (no search logic)
4. **Clear Ownership:** Each data structure has exactly one owning thread
5. **No Shared Mutable State:** All data passed by value or via immutable structures
6. **Message Passing:** All inter-thread communication through queues

### Thread Model

```
┌─────────────────────────────────────────────────────────────┐
│                    Main UI Thread                           │
│  - wxWidgets event loop                                     │
│  - User input handling                                      │
│  - UI rendering                                             │
│  - Receives search events via wxQueueEvent()                │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ Commands (thread-safe queue)
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                 Search Worker Thread                        │
│  - UnifiedSearchManager event loop                          │
│  - Processes all search commands                            │
│  - Coordinates search engines                               │
│  - Sends events back to UI thread                           │
└─────────────────────────────────────────────────────────────┘
                              │
                              │ UDP packets (read-only)
                              ▼
┌─────────────────────────────────────────────────────────────┐
│              Network I/O Thread(s)                          │
│  - Socket event loop                                        │
│  - Packet reception only                                    │
│  - Forwards packets to search thread via queue              │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Abstractions

### Search Types

```cpp
namespace search {

enum class SearchType {
    LOCAL,      // Local file search (shared files)
    GLOBAL,     // Global server search
    KADEMLIA    // Kademlia network search
};

enum class SearchState {
    IDLE,
    STARTING,
    RUNNING,
    PAUSED,
    COMPLETED,
    FAILED,
    CANCELLED
};

struct SearchId {
    uint64_t value;
    
    static SearchId Generate();
    bool IsValid() const;
    bool operator==(const SearchId& other) const;
    bool operator!=(const SearchId& other) const;
    bool operator<(const SearchId& other) const;
};

} // namespace search
```

### Unified Search Result

```cpp
namespace search {

struct SearchResult {
    SearchId searchId;
    SearchType sourceType;
    
    // File information
    std::string fileName;
    uint64_t fileSize;
    std::string fileType;
    
    // Source information
    std::string fileHash;  // MD4 hash
    uint32_t availability;
    
    // Metadata
    std::map<std::string, std::string> metadata;
    
    // Source locations
    struct SourceLocation {
        std::string ip;
        uint16_t port;
        uint32_t kadId;  // For Kad sources
    };
    std::vector<SourceLocation> sources;
    
    // Timing
    std::chrono::system_clock::time_point discoveredAt;
    
    // Serialization for thread-safe passing
    std::vector<uint8_t> Serialize() const;
    static SearchResult Deserialize(const std::vector<uint8_t>& data);
};

} // namespace search
```

### Search Parameters

```cpp
namespace search {

struct SearchParams {
    SearchType type;
    std::string query;
    
    // Type-specific parameters
    struct KadParams {
        std::string keywordHash;
        uint32_t maxNodes;
        bool requestMoreResults;
    };
    
    struct GlobalParams {
        uint32_t serverIp;
        uint16_t serverPort;
    };
    
    std::optional<KadParams> kadParams;
    std::optional<GlobalParams> globalParams;
    
    // Common parameters
    uint32_t maxResults;
    std::chrono::seconds timeout;
    
    // Filtering
    std::optional<uint64_t> minFileSize;
    std::optional<uint64_t> maxFileSize;
    std::vector<std::string> fileTypes;
    
    // Serialization
    std::vector<uint8_t> Serialize() const;
    static SearchParams Deserialize(const std::vector<uint8_t>& data);
};

} // namespace search
```

---

## Thread Management

### ISearchEngine Interface

```cpp
namespace search {

/**
 * Abstract interface for all search engines (local, global, Kad)
 * All implementations must be thread-safe for single-threaded use
 * (no internal locking needed as all calls happen in search thread)
 */
class ISearchEngine {
public:
    virtual ~ISearchEngine() = default;
    
    /**
     * Start a new search with given parameters
     */
    virtual SearchId StartSearch(const SearchParams& params) = 0;
    
    /**
     * Stop an active search
     */
    virtual void StopSearch(SearchId searchId) = 0;
    
    /**
     * Pause an active search (can be resumed)
     */
    virtual void PauseSearch(SearchId searchId) = 0;
    
    /**
     * Resume a paused search
     */
    virtual void ResumeSearch(SearchId searchId) = 0;
    
    /**
     * Request more results for an active search
     */
    virtual void RequestMoreResults(SearchId searchId) = 0;
    
    /**
     * Get current state of a search
     */
    virtual SearchState GetSearchState(SearchId searchId) const = 0;
    
    /**
     * Get search parameters
     */
    virtual SearchParams GetSearchParams(SearchId searchId) const = 0;
    
    /**
     * Get current results for a search
     */
    virtual std::vector<SearchResult> GetResults(SearchId searchId, size_t maxResults = 0) const = 0;
    
    /**
     * Get result count for a search
     */
    virtual size_t GetResultCount(SearchId searchId) const = 0;
    
    /**
     * Process a command (called by UnifiedSearchManager)
     */
    virtual void ProcessCommand(const SearchCommand& command) = 0;
    
    /**
     * Perform periodic maintenance (called by UnifiedSearchManager)
     */
    virtual void ProcessMaintenance(std::chrono::system_clock::time_point currentTime) = 0;
    
    /**
     * Shutdown the engine and cleanup resources
     */
    virtual void Shutdown() = 0;
};

} // namespace search
```

### Search Command Structure

```cpp
namespace search {

/**
 * Commands sent from UI thread to search thread
 * All commands are serializable for thread-safe queue passing
 */
struct SearchCommand {
    enum class Type {
        START_SEARCH,
        STOP_SEARCH,
        PAUSE_SEARCH,
        RESUME_SEARCH,
        REQUEST_MORE_RESULTS,
        GET_SEARCH_STATE,
        GET_SEARCH_PARAMS,
        GET_RESULTS,
        GET_RESULT_COUNT,
        CANCEL_ALL_SEARCHES
    };
    
    Type type;
    SearchId searchId;
    SearchParams params;
    size_t maxResults;
    
    // Response callback (executed in search thread, result sent back to UI)
    using ResponseCallback = std::function<void(const std::vector<uint8_t>& response)>;
    ResponseCallback responseCallback;
    
    // Serialization
    std::vector<uint8_t> Serialize() const;
    static SearchCommand Deserialize(const std::vector<uint8_t>& data);
};

/**
 * Events sent from search thread to UI thread
 * All events are serializable for thread-safe passing via wxEvents
 */
struct SearchEvent {
    enum class Type {
        SEARCH_STARTED,
        SEARCH_COMPLETED,
        SEARCH_FAILED,
        SEARCH_CANCELLED,
        SEARCH_PAUSED,
        SEARCH_RESUMED,
        RESULTS_RECEIVED,
        PROGRESS_UPDATE,
        ERROR_OCCURRED
    };
    
    Type type;
    SearchId searchId;
    std::string errorMessage;
    
    // For RESULTS_RECEIVED
    std::vector<SearchResult> results;
    
    // For PROGRESS_UPDATE
    struct ProgressInfo {
        int percentage;
        int serversContacted;
        int nodesContacted;
        int resultsReceived;
        std::string statusMessage;
    };
    std::optional<ProgressInfo> progress;
    
    // Serialization
    std::vector<uint8_t> Serialize() const;
    static SearchEvent Deserialize(const std::vector<uint8_t>& data);
};

} // namespace search
```

### UnifiedSearchManager

```cpp
namespace search {

/**
 * Central manager for all search operations
 * Runs in its own thread, coordinating all search engines
 * All search operations are single-threaded within this manager
 */
class UnifiedSearchManager {
public:
    struct Config {
        std::chrono::milliseconds maintenanceInterval{1000};
        std::chrono::milliseconds commandTimeout{5000};
        size_t maxConcurrentSearches{10};
        size_t maxResultsPerSearch{500};
    };
    
    explicit UnifiedSearchManager(const Config& config = Config{});
    ~UnifiedSearchManager();
    
    // Non-copyable, non-movable
    UnifiedSearchManager(const UnifiedSearchManager&) = delete;
    UnifiedSearchManager& operator=(const UnifiedSearchManager&) = delete;
    
    /**
     * Start the search manager and its worker thread
     */
    void Start();
    
    /**
     * Shutdown the search manager and wait for completion
     */
    void Shutdown();
    
    /**
     * Send a command to the search thread
     * Thread-safe: can be called from any thread
     */
    bool SendCommand(const SearchCommand& command);
    
    /**
     * Register a callback for search events
     * Callback will be called in the UI thread
     */
    void SetEventCallback(std::function<void(const SearchEvent&)> callback);
    
    /**
     * Get current statistics
     */
    struct Statistics {
        size_t activeSearches;
        size_t completedSearches;
        size_t failedSearches;
        size_t totalResults;
        std::chrono::system_clock::time_point startTime;
    };
    Statistics GetStatistics() const;
    
 private:
    // Worker thread function
    void WorkerThread();
    
    // Process a single command
    void ProcessCommand(const SearchCommand& command);
    
    // Send event to UI thread
    void SendEvent(const SearchEvent& event);
    
    // Periodic maintenance
    void PerformMaintenance();
    
    // Engine management
    void InitializeEngines();
    void CleanupEngines();
    
    // Configuration
    Config m_config;
    
    // Thread management
    std::thread m_workerThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_shutdownRequested{false};
    
    // Command queue (thread-safe)
    mutable std::mutex m_commandQueueMutex;
    std::condition_variable m_commandQueueCV;
    std::queue<SearchCommand> m_commandQueue;
    
    // Event callback (called in UI thread)
    std::function<void(const SearchEvent&)> m_eventCallback;
    mutable std::mutex m_eventCallbackMutex;
    
    // Search engines (only accessed in worker thread)
    std::unique_ptr<ISearchEngine> m_localSearchEngine;
    std::unique_ptr<ISearchEngine> m_globalSearchEngine;
    std::unique_ptr<ISearchEngine> m_kadSearchEngine;
    
    // Search tracking (only accessed in worker thread)
    std::unordered_map<SearchId, SearchState> m_searchStates;
    std::unordered_map<SearchId, SearchParams> m_searchParams;
    std::unordered_map<SearchId, std::vector<SearchResult>> m_searchResults;
    
    // Statistics (only accessed in worker thread)
    Statistics m_statistics;
    
    // Engine selection
    ISearchEngine* SelectEngine(SearchType type);
};

} // namespace search
```

---

## Data Flow

### Starting a Search

```
1. User clicks "Search" button in UI (Main Thread)
   ↓
2. SearchTab creates SearchParams
   ↓
3. SearchTab calls UnifiedSearchManager::SendCommand() with START_SEARCH command
   ↓
4. Command is serialized and added to thread-safe queue
   ↓
5. Search Worker Thread wakes up and processes command
   ↓
6. UnifiedSearchManager::ProcessCommand() routes to appropriate engine
   ↓
7. KadSearchEngine::StartSearch() creates new search state
   ↓
8. KadSearchEngine sends initial KADEMLIA2_REQ packets to contacts
   ↓
9. KadSearchEngine sends SEARCH_STARTED event to UI thread
   ↓
10. UI thread receives event via wxQueueEvent() and updates UI
```

### Receiving Search Results

```
1. Network I/O Thread receives UDP packet
   ↓
2. Packet is parsed and validated
   ↓
3. If packet is KADEMLIA2_RES or KADEMLIA_SEARCH_RES, forward to search thread
   ↓
4. Search Worker Thread processes packet
   ↓
5. KadSearchEngine::ProcessPacket() updates search state
   ↓
6. If packet contains results, KadSearchEngine::ProcessResult() extracts them
   ↓
7. KadSearchEngine sends RESULTS_RECEIVED event to UI thread
   ↓
8. UI thread receives event and updates search results display
```

### JumpStart (Periodic Maintenance)

```
1. Search Worker Thread wakes up for maintenance (every 1 second)
   ↓
2. UnifiedSearchManager::PerformMaintenance() is called
   ↓
3. KadSearchEngine::ProcessMaintenance() is called
   ↓
4. For each stalled search, KadSearchEngine::JumpStart() is called
   ↓
5. JumpStart() queries more contacts to advance the search
   ↓
6. If search is complete, KadSearchEngine sends SEARCH_COMPLETED event
```

---

## Implementation Plan

### Phase 1: Foundation (Week 1-2)

**Goal:** Create core abstractions and infrastructure

**Tasks:**
1. Create new directory structure:
   ```
   src/search/
   ├── core/
   │   ├── SearchTypes.h
   │   ├── SearchId.h
   │   ├── SearchResult.h
   │   ├── SearchParams.h
   │   ├── SearchCommand.h
   │   └── SearchEvent.h
   ├── manager/
   │   ├── UnifiedSearchManager.h
   │   └── UnifiedSearchManager.cpp
   └── engines/
       ├── ISearchEngine.h
       ├── LocalSearchEngine.h
       ├── LocalSearchEngine.cpp
       ├── GlobalSearchEngine.h
       ├── GlobalSearchEngine.cpp
       ├── KadSearchEngine.h
       └── KadSearchEngine.cpp
   ```

2. Implement core data structures:
   - SearchId with generation
   - SearchResult with serialization
   - SearchParams with validation
   - SearchCommand with serialization
   - SearchEvent with serialization

3. Implement UnifiedSearchManager:
   - Worker thread with event loop
   - Command queue with mutex/condition variable
   - Event dispatch to UI thread via wxQueueEvent
   - Engine lifecycle management
   - Statistics tracking

4. Create unit tests for core abstractions

**Deliverables:**
- Core header files
- UnifiedSearchManager implementation
- Unit tests passing

### Phase 2: Local Search Engine (Week 3)

**Goal:** Implement local file search as proof of concept

**Tasks:**
1. Implement LocalSearchEngine:
   - Search shared files database
   - Filter by query parameters
   - Return results as SearchResult objects
   - Support pause/resume/cancel

2. Integrate with UnifiedSearchManager
3. Create integration tests
4. Update UI to use new abstraction (for local searches only)

**Deliverables:**
- LocalSearchEngine implementation
- Integration tests passing
- UI integration for local searches

### Phase 3: Global Search Engine (Week 4)

**Goal:** Implement global server search

**Tasks:**
1. Implement GlobalSearchEngine:
   - Communicate with ED2K servers
   - Send search requests
   - Process search results
   - Handle server disconnections
   - Support pause/resume/cancel

2. Integrate with UnifiedSearchManager
3. Create integration tests
4. Update UI to use new abstraction (for global searches)

**Deliverables:**
- GlobalSearchEngine implementation
- Integration tests passing
- UI integration for global searches

### Phase 4: Kad Search Engine - Foundation (Week 5-6)

**Goal:** Refactor Kad search to new architecture

**Tasks:**
1. Create KadSearchEngine skeleton:
   - Implement ISearchEngine interface
   - Create KadSearchState class (refactored from CSearch)
   - Implement search lifecycle management

2. Refactor Kad contact management:
   - Remove mutex from SearchManager
   - Remove reference counting from CContact
   - Move contact state into KadSearchState

3. Implement Kad packet processing:
   - Process KADEMLIA2_REQ (routing queries)
   - Process KADEMLIA2_RES (routing responses)
   - Process KADEMLIA_SEARCH_RES (search results)
   - Process KADEMLIA_PUBLISH operations

4. Implement Kad search logic:
   - Contact selection and querying
   - Result aggregation
   - JumpStart (periodic maintenance)
   - RequestMoreResults

5. Create unit tests for KadSearchEngine
6. Create integration tests with mock Kad network

**Deliverables:**
- KadSearchEngine implementation
- Refactored contact management
- Unit tests passing
- Integration tests passing

### Phase 5: Kad Search Engine - Integration (Week 7)

**Goal:** Integrate KadSearchEngine with existing Kad infrastructure

**Tasks:**
1. Integrate with CKademliaUDPListener:
   - Forward Kad packets to search thread
   - Use thread-safe queue for packet forwarding

2. Integrate with CKademlia routing zone:
   - Get contacts for queries
   - Update routing zone with new contacts

3. Integrate with SearchList:
   - Convert SearchResult to existing format
   - Update UI with results

4. Remove old SearchManager and CSearch classes:
   - Deprecate old classes
   - Migrate all search creation to new abstraction
   - Remove deprecated code

5. End-to-end testing with real Kad network

**Deliverables:**
- Full Kad search integration
- All old code removed
- End-to-end tests passing

### Phase 6: UI Integration (Week 8)

**Goal:** Update UI to use unified search abstraction

**Tasks:**
1. Update SearchTab:
   - Use UnifiedSearchManager for all searches
   - Handle search events
   - Display unified search results

2. Update search result display:
   - Show results from all search types
   - Filter by search type
   - Sort and group results

3. Update search controls:
   - Unified start/stop/pause/resume buttons
   - Unified progress display
   - Unified error handling

4. Create UI integration tests
5. User acceptance testing

**Deliverables:**
- Fully updated UI
- UI integration tests passing
- User acceptance approved

### Phase 7: Testing and Optimization (Week 9-10)

**Goal:** Ensure quality and performance

**Tasks:**
1. Comprehensive testing:
   - Unit tests for all components
   - Integration tests for all search types
   - End-to-end tests for complete workflows
   - Performance benchmarks
   - Stress tests with many concurrent searches

2. Performance optimization:
   - Optimize serialization/deserialization
   - Optimize command queue operations
   - Optimize result aggregation
   - Profile and address bottlenecks

3. Code review and cleanup:
   - Remove dead code
   - Improve documentation
   - Address code review comments
   - Finalize API documentation

**Deliverables:**
- All tests passing
- Performance benchmarks meeting targets
- Code review approved
- Documentation complete

### Phase 8: Deployment (Week 11)

**Goal:** Deploy to production

**Tasks:**
1. Create deployment package
2. Update user documentation
3. Create migration guide
4. Train support team
5. Deploy to staging environment
6. Monitor for issues
7. Deploy to production
8. Monitor and address any issues

**Deliverables:**
- Deployment package ready
- User documentation updated
- Migration guide created
- Production deployment successful

---

## Migration Strategy

### Backward Compatibility

During migration, we will maintain backward compatibility by:

1. **Dual Implementation:** Keep old and new implementations running in parallel
2. **Feature Flags:** Use feature flags to switch between old and new implementations
3. **Gradual Rollout:** Roll out new implementation gradually to users
4. **Rollback Plan:** Ability to quickly rollback if issues are discovered

### Migration Steps

#### Step 1: Prepare Codebase (Phase 1-2)

- Add new directory structure
- Implement core abstractions
- Keep all old code unchanged
- Add feature flag `USE_UNIFIED_SEARCH_MANAGER`

#### Step 2: Migrate Local Searches (Phase 3)

- Implement LocalSearchEngine
- Add feature flag `USE_LOCAL_SEARCH_ENGINE`
- Default: old implementation
- Beta testers: new implementation
- Monitor for issues

#### Step 3: Migrate Global Searches (Phase 4)

- Implement GlobalSearchEngine
- Add feature flag `USE_GLOBAL_SEARCH_ENGINE`
- Default: old implementation
- Beta testers: new implementation
- Monitor for issues

#### Step 4: Migrate Kad Searches (Phase 5-6)

- Implement KadSearchEngine
- Add feature flag `USE_KAD_SEARCH_ENGINE`
- Default: old implementation
- Beta testers: new implementation
- Monitor for issues

#### Step 5: UI Integration (Phase 7)

- Update UI to use unified abstraction
- Add feature flag `USE_UNIFIED_SEARCH_UI`
- Default: old UI
- Beta testers: new UI
- Monitor for issues

#### Step 6: Full Rollout (Phase 8-11)

- Enable all feature flags for beta testers
- Monitor for issues
- Gradually enable for all users
- Keep old code for rollback
- Once stable, remove old code

#### Step 7: Cleanup

- Remove old implementation code
- Remove feature flags
- Update documentation
- Archive old code for reference

### Rollback Plan

If critical issues are discovered during rollout:

1. Disable feature flag for affected component
2. Users automatically revert to old implementation
3. Fix issues in new implementation
4. Re-enable feature flag for beta testers
5. Monitor for issues
6. Gradually re-enable for all users

---

## Testing Strategy

### Unit Tests

**Coverage Requirements:**
- Core abstractions: 100%
- UnifiedSearchManager: 90%
- LocalSearchEngine: 90%
- GlobalSearchEngine: 90%
- KadSearchEngine: 85%

**Test Framework:** Google Test (gtest)

**Example Test Cases:**

```cpp
// Test SearchId generation
TEST(SearchIdTest, GenerateUniqueId) {
    SearchId id1 = SearchId::Generate();
    SearchId id2 = SearchId::Generate();
    EXPECT_NE(id1.value, id2.value);
    EXPECT_TRUE(id1.IsValid());
    EXPECT_TRUE(id2.IsValid());
}

// Test SearchResult serialization
TEST(SearchResultTest, SerializeDeserialize) {
    SearchResult original;
    original.searchId = SearchId::Generate();
    original.fileName = "test.mp3";
    original.fileSize = 1024 * 1024;
    
    auto serialized = original.Serialize();
    SearchResult deserialized = SearchResult::Deserialize(serialized);
    
    EXPECT_EQ(original.searchId, deserialized.searchId);
    EXPECT_EQ(original.fileName, deserialized.fileName);
    EXPECT_EQ(original.fileSize, deserialized.fileSize);
}

// Test UnifiedSearchManager command processing
TEST(UnifiedSearchManagerTest, ProcessStartSearchCommand) {
    UnifiedSearchManager manager;
    manager.Start();
    
    SearchParams params;
    params.type = SearchType::LOCAL;
    params.query = "test";
    
    bool eventReceived = false;
    manager.SetEventCallback([&eventReceived](const SearchEvent& event) {
        if (event.type == SearchEvent::Type::SEARCH_STARTED) {
            eventReceived = true;
        }
    });
    
    SearchCommand command;
    command.type = SearchCommand::Type::START_SEARCH;
    command.params = params;
    manager.SendCommand(command);
    
    // Wait for event to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(eventReceived);
    
    manager.Shutdown();
}
```

### Integration Tests

**Test Scenarios:**

1. **Local Search Integration:**
   - Start local search
   - Receive results
   - Stop search
   - Pause and resume search

2. **Global Search Integration:**
   - Start global search
   - Receive results from multiple servers
   - Handle server disconnection
   - Stop search

3. **Kad Search Integration:**
   - Start Kad search
   - Receive routing responses
   - Receive search results
   - JumpStart stalled search
   - Request more results
   - Stop search

4. **Multi-Search Integration:**
   - Start multiple searches of different types
   - Ensure they don't interfere with each other
   - Stop all searches
   - Verify cleanup

**Test Framework:** Google Test with mock network

### End-to-End Tests

**Test Scenarios:**

1. **Complete User Workflow:**
   - User starts local search
   - User starts global search
   - User starts Kad search
   - User views results from all searches
   - User stops all searches

2. **Error Handling:**
   - Network disconnection during search
   - Invalid search parameters
   - Search timeout
   - Server not responding

3. **Performance:**
   - 100 concurrent searches
   - Large result sets (1000+ results)
   - Long-running searches (10+ minutes)

**Test Framework:** Selenium-like UI automation

### Performance Tests

**Benchmarks:**

1. **Search Startup Time:**
   - Target: < 100ms from command to search started

2. **Result Delivery Time:**
   - Target: < 50ms from packet received to UI updated

3. **Search Throughput:**
   - Target: 1000 searches/second

4. **Memory Usage:**
   - Target: < 100MB for 100 concurrent searches

5. **CPU Usage:**
   - Target: < 10% for idle search manager

**Test Framework:** Custom benchmarking framework

---

## Performance Considerations

### Serialization Optimization

**Issue:** Serializing/deserializing all commands and events adds overhead

**Solutions:**
1. Use binary serialization instead of text-based (e.g., protobuf, flatbuffers)
2. Cache serialized objects when possible
3. Use move semantics to avoid copies
4. Implement zero-copy queues for large payloads

### Command Queue Optimization

**Issue:** Queue operations can become bottleneck with high command volume

**Solutions:**
1. Use lock-free queue (e.g., boost::lockfree::queue)
2. Batch multiple commands together
3. Use multiple queues for different command priorities
4. Implement queue length monitoring and backpressure

### Result Aggregation Optimization

**Issue:** Aggregating results from multiple sources can be expensive

**Solutions:**
1. Use incremental aggregation
2. Limit result set size
3. Implement result deduplication at aggregation level
4. Use efficient data structures (e.g., hash sets)

### Memory Management

**Issue:** Large result sets can consume significant memory

**Solutions:**
1. Implement result pagination
2. Use memory pools for frequent allocations
3. Implement result aging and cleanup
4. Use smart pointers for automatic cleanup

---

## Conclusion

This architectural redesign provides a comprehensive solution to the race conditions in the current Kademlia search implementation while unifying all search types under a single abstraction layer. The design ensures:

1. **Thread Safety:** All search operations are single-threaded with clear boundaries
2. **Maintainability:** Clean separation of concerns with testable components
3. **Performance:** Optimized for high throughput and low latency
4. **Extensibility:** Easy to add new search types or features
5. **Backward Compatibility:** Gradual migration with rollback capability

The implementation plan provides a clear path from current architecture to the new design, with comprehensive testing and performance optimization at each phase.

---

## Appendix A: File Structure

```
src/search/
├── core/
│   ├── SearchTypes.h
│   ├── SearchId.h
│   ├── SearchId.cpp
│   ├── SearchResult.h
│   ├── SearchResult.cpp
│   ├── SearchParams.h
│   ├── SearchParams.cpp
│   ├── SearchCommand.h
│   ├── SearchCommand.cpp
│   ├── SearchEvent.h
│   └── SearchEvent.cpp
├── manager/
│   ├── UnifiedSearchManager.h
│   └── UnifiedSearchManager.cpp
├── engines/
│   ├── ISearchEngine.h
│   ├── LocalSearchEngine.h
│   ├── LocalSearchEngine.cpp
│   ├── GlobalSearchEngine.h
│   ├── GlobalSearchEngine.cpp
│   ├── KadSearchEngine.h
│   ├── KadSearchEngine.cpp
│   ├── kad/
│   │   ├── KadSearchState.h
│   │   ├── KadSearchState.cpp
│   │   ├── KadContactManager.h
│   │   ├── KadContactManager.cpp
│   │   ├── KadPacketProcessor.h
│   │   └── KadPacketProcessor.cpp
│   └── global/
│       ├── GlobalSearchState.h
│       ├── GlobalSearchState.cpp
│       ├── ServerConnection.h
│       └── ServerConnection.cpp
└── tests/
    ├── unit/
    │   ├── SearchIdTest.cpp
    │   ├── SearchResultTest.cpp
    │   ├── SearchParamsTest.cpp
    │   ├── SearchCommandTest.cpp
    │   ├── SearchEventTest.cpp
    │   ├── UnifiedSearchManagerTest.cpp
    │   ├── LocalSearchEngineTest.cpp
    │   ├── GlobalSearchEngineTest.cpp
    │   └── KadSearchEngineTest.cpp
    ├── integration/
    │   ├── LocalSearchIntegrationTest.cpp
    │   ├── GlobalSearchIntegrationTest.cpp
    │   ├── KadSearchIntegrationTest.cpp
    │   └── MultiSearchIntegrationTest.cpp
    └── e2e/
        ├── UserWorkflowTest.cpp
        ├── ErrorHandlingTest.cpp
        └── PerformanceTest.cpp

docs/
└── SEARCH_ARCHITECTURE_REDESIGN.md (this file)
```

---

## Appendix B: API Reference

### UnifiedSearchManager API

```cpp
class UnifiedSearchManager {
public:
    struct Config {
        std::chrono::milliseconds maintenanceInterval{1000};
        std::chrono::milliseconds commandTimeout{5000};
        size_t maxConcurrentSearches{10};
        size_t maxResultsPerSearch{500};
    };
    
    UnifiedSearchManager(const Config& config = Config{});
    ~UnifiedSearchManager();
    
    void Start();
    void Shutdown();
    bool SendCommand(const SearchCommand& command);
    void SetEventCallback(std::function<void(const SearchEvent&)> callback);
    Statistics GetStatistics() const;
};
```

### ISearchEngine API

```cpp
class ISearchEngine {
public:
    virtual SearchId StartSearch(const SearchParams& params) = 0;
    virtual void StopSearch(SearchId searchId) = 0;
    virtual void PauseSearch(SearchId searchId) = 0;
    virtual void ResumeSearch(SearchId searchId) = 0;
    virtual void RequestMoreResults(SearchId searchId) = 0;
    virtual SearchState GetSearchState(SearchId searchId) const = 0;
    virtual SearchParams GetSearchParams(SearchId searchId) const = 0;
    virtual std::vector<SearchResult> GetResults(SearchId searchId, size_t maxResults = 0) const = 0;
    virtual size_t GetResultCount(SearchId searchId) const = 0;
    virtual void ProcessCommand(const SearchCommand& command) = 0;
    virtual void ProcessMaintenance(std::chrono::system_clock::time_point currentTime) = 0;
    virtual void Shutdown() = 0;
};
```

---

## Appendix C: Glossary

- **Search Thread:** Dedicated thread that runs all search operations
- **UI Thread:** Main wxWidgets thread that handles user interactions
- **Network Thread:** Thread that handles socket I/O and packet reception
- **Command:** Message sent from UI thread to search thread
- **Event:** Message sent from search thread to UI thread
- **Search Engine:** Implementation of ISearchEngine for a specific search type
- **Search State:** Current status of a search (IDLE, RUNNING, COMPLETED, etc.)
- **Search Result:** Single result from a search, containing file information
- **Serialization:** Converting objects to byte streams for thread-safe passing
- **Race Condition:** Concurrent access to shared mutable state without synchronization

---

## Appendix D: References

- [C++ Concurrency in Action](https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition)
- [wxWidgets Thread Programming](https://docs.wxwidgets.org/3.0/overview_thread.html)
- [Kademlia DHT Specification](https://pdos.csail.mit.edu/~petar/papers/maymounkov-kademlia-lncs.pdf)
- [ED2K Protocol Specification](http://www.edonkey2000.com/documentation/ed2k_protocol.html)

---

**Document Version History:**

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-02-12 | Architecture Team | Initial design document |

---

**Review Status:**

- [ ] Technical Review
- [ ] Architecture Review
- [ ] Security Review
- [ ] Performance Review
- [ ] User Experience Review
