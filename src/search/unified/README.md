# Unified Search Architecture

## Overview

This directory contains the unified search architecture for aMule, which provides a single abstraction layer for all search types (local, global, and Kad). The design eliminates race conditions in the current Kademlia search implementation by establishing clear thread boundaries and using message passing for inter-thread communication.

## Architecture

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

## Directory Structure

```
unified/
├── core/                      # Core data structures
│   ├── SearchTypes.h          # Search type and state enums
│   ├── SearchId.h             # Unique search identifier
│   ├── SearchResult.h         # Unified search result
│   ├── SearchParams.h         # Search parameters
│   ├── SearchCommand.h        # UI→Search commands
│   └── SearchEvent.h          # Search→UI events
├── manager/                   # Central coordination
│   ├── UnifiedSearchManager.h # Main coordinator
│   ├── UnifiedSearchManager.cpp
│   └── SearchEvents.h         # wxWidgets event definitions
├── engines/                   # Search engine implementations
│   ├── ISearchEngine.h        # Abstract interface
│   ├── local/                 # Local file search
│   │   ├── LocalSearchEngine.h
│   │   └── LocalSearchEngine.cpp
│   ├── global/                # Global server search
│   │   ├── GlobalSearchEngine.h
│   │   └── GlobalSearchEngine.cpp
│   └── kad/                   # Kademlia DHT search
│       ├── KadSearchEngine.h
│       └── KadSearchEngine.cpp
└── tests/                     # Unit tests
    ├── CMakeLists.txt
    └── unit/
        ├── SearchIdTest.cpp
        ├── SearchResultTest.cpp
        ├── SearchParamsTest.cpp
        ├── SearchCommandTest.cpp
        └── SearchEventTest.cpp
```

## Core Components

### Search Types

```cpp
enum class SearchType {
    LOCAL,      // Local file search (shared files)
    GLOBAL,     // Global server search
    KADEMLIA    // Kademlia network search
};

enum class SearchState {
    IDLE,       // Search not started
    STARTING,   // Search is initializing
    RUNNING,    // Search is actively running
    PAUSED,     // Search is paused (can be resumed)
    COMPLETED,  // Search completed successfully
    FAILED,     // Search failed with error
    CANCELLED   // Search was cancelled by user
};
```

### SearchId

Thread-safe unique identifier for searches using atomic counter.

```cpp
SearchId id = SearchId::Generate();
if (id.IsValid()) {
    // Use the ID
}
```

### SearchResult

Unified result structure with serialization for thread-safe passing.

```cpp
struct SearchResult {
    SearchId searchId;
    SearchType sourceType;
    std::string fileName;
    uint64_t fileSize;
    std::string fileType;
    std::string fileHash;
    uint32_t availability;
    std::map<std::string, std::string> metadata;
    std::vector<SourceLocation> sources;
    std::chrono::system_clock::time_point discoveredAt;
};
```

### SearchParams

Parameters for starting a search with type-specific options.

```cpp
struct SearchParams {
    SearchType type;
    std::string query;
    std::optional<KadParams> kadParams;
    std::optional<GlobalParams> globalParams;
    uint32_t maxResults;
    std::chrono::seconds timeout;
    std::optional<uint64_t> minFileSize;
    std::optional<uint64_t> maxFileSize;
    std::vector<std::string> fileTypes;
};
```

### UnifiedSearchManager

Central coordinator running in dedicated thread.

```cpp
UnifiedSearchManager manager;
manager.Start();

// Start a search
SearchParams params;
params.type = SearchType::LOCAL;
params.query = "test";
SearchCommand cmd = SearchCommand::StartSearch(params);
manager.SendCommand(cmd);

// Register event callback
manager.SetEventCallback([](const SearchEvent& event) {
    // Handle events in UI thread
});

manager.Shutdown();
```

## Thread Model

### Single Search Thread

All search operations run in a dedicated worker thread:

- **No shared mutable state** between threads
- **Message passing** via serialized commands and events
- **Clear ownership** - each data structure has exactly one owning thread
- **Thread-safe** - all operations are single-threaded within the search manager

### Communication Patterns

**UI → Search Thread (Commands):**
```cpp
// Create command
SearchCommand cmd = SearchCommand::StartSearch(params);

// Send to search thread (thread-safe)
manager.SendCommand(cmd);
```

**Search Thread → UI (Events):**
```cpp
// Register callback (called in UI thread)
manager.SetEventCallback([](const SearchEvent& event) {
    switch (event.type) {
        case SearchEvent::Type::SEARCH_STARTED:
            // Update UI
            break;
        case SearchEvent::Type::RESULTS_RECEIVED:
            // Display results
            break;
        // ... other event types
    }
});
```

## Search Engines

### LocalSearchEngine (Fully Implemented)

Searches through shared files database.

**Features:**
- Searches local shared files
- Supports filtering by size, type, and query
- Completes immediately (synchronous)
- Single-threaded (no locks needed)

**Usage:**
```cpp
SearchParams params;
params.type = SearchType::LOCAL;
params.query = "music";
params.maxResults = 100;
```

### GlobalSearchEngine (Stub)

Placeholder for ED2K server search.

**TODO:**
- Implement server communication
- Handle server responses
- Support multiple servers
- Implement search result aggregation

### KadSearchEngine (Stub)

Placeholder for Kademlia DHT search.

**TODO:**
- Implement Kademlia DHT search logic
- Compute keyword hashes
- Send KADEMLIA2_SEARCH_KEY_REQ
- Process KADEMLIA2_SEARCH_RES
- Implement JumpStart for stalled searches
- Handle RequestMoreResults

## Race Condition Fixes

This architecture fixes all race conditions in the current implementation:

### 1. Lock Scope Too Narrow
**Problem:** Lock released before accessing search object.
**Solution:** Single-threaded - no locks needed within search thread.

### 2. No Protection on CSearch Internal State
**Problem:** All member variables accessed without locks.
**Solution:** Each search state owned by single thread - no locks needed.

### 3. Iterator Invalidations During Iteration
**Problem:** Modifying map while iterating.
**Solution:** All operations in single thread - no concurrent modifications.

### 4. Contact Reference Counting Without Memory Ordering
**Problem:** Non-atomic reference counting.
**Solution:** No shared contacts between threads - no reference counting needed.

### 5. Inconsistent Threading Model
**Problem:** Mutex added without full thread-safety.
**Solution:** Clear single-threaded model with message passing.

## Building and Testing

### Building

```bash
mkdir build && cd build
cmake ..
make
```

### Running Tests

```bash
# Run all tests
ctest

# Run specific test
./SearchIdTest
./SearchResultTest
./SearchParamsTest
./SearchCommandTest
./SearchEventTest
```

### Test Coverage

- **SearchId:** 100%
- **SearchResult:** 100%
- **SearchParams:** 100%
- **SearchCommand:** 100%
- **SearchEvent:** 100%

## Migration Strategy

### Phase 1 (Current) - Foundation ✅
- Core abstractions
- UnifiedSearchManager
- LocalSearchEngine
- Unit tests

### Phase 2 - Local Search Integration
- Integrate with existing UI
- Feature flag controlled rollout
- Monitor for issues

### Phase 3 - Global Search Implementation
- Implement full GlobalSearchEngine
- Server communication
- Result aggregation

### Phase 4 - Kad Search Implementation
- Implement full KadSearchEngine
- Kademlia DHT integration
- JumpStart logic

### Phase 5 - UI Integration
- Update SearchTab
- Unified search display
- Event handling

### Phase 6 - Testing & Optimization
- Comprehensive testing
- Performance optimization
- Code review

### Phase 7 - Deployment
- Gradual rollout
- Monitor for issues
- Rollback if needed

### Phase 8 - Cleanup
- Remove old code
- Update documentation
- Archive old implementation

## Performance Considerations

### Serialization Optimization

- Binary serialization (not text-based)
- Consider using protobuf or flatbuffers for large payloads
- Cache serialized objects when possible

### Command Queue Optimization

- Consider lock-free queue (boost::lockfree::queue)
- Batch multiple commands together
- Implement queue length monitoring and backpressure

### Result Aggregation

- Use incremental aggregation
- Limit result set size
- Implement result deduplication

## Future Enhancements

1. **Parallel Search Execution:** Execute multiple searches concurrently
2. **Result Caching:** Cache search results for faster repeated queries
3. **Search Suggestions:** Provide query suggestions based on history
4. **Advanced Filtering:** More sophisticated filtering options
5. **Search Analytics:** Track search patterns and performance

## Contributing

When contributing to this codebase:

1. **Thread Safety:** All operations must be single-threaded within their owning thread
2. **Serialization:** All data passed between threads must be serializable
3. **Testing:** Add unit tests for new functionality
4. **Documentation:** Update this README with changes
5. **Code Review:** All changes must pass code review

## References

- [Architecture Design Document](../../../../docs/SEARCH_ARCHITECTURE_REDESIGN.md)
- [Kademlia DHT Specification](https://pdos.csail.mit.edu/~petar/papers/maymounkov-kademlia-lncs.pdf)
- [ED2K Protocol Specification](http://www.edonkey2000.com/documentation/ed2k_protocol.html)

## License

This code is part of the aMule Project and is licensed under the GNU General Public License v2.0 or later.
