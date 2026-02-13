# Search Cache and Reuse Feature

## Overview

This feature improves user experience by automatically reusing existing search tabs when the user performs an identical search (same query text and search type). Instead of creating a new search tab, the system will request more results from the existing search.

## Benefits

1. **Better User Experience:** Users see results immediately instead of waiting for a new search
2. **Reduced Network Traffic:** Avoids sending duplicate search requests to servers/nodes
3. **Lower Resource Usage:** Fewer concurrent searches means less memory and CPU usage
4. **Cleaner UI:** Fewer duplicate search tabs cluttering the interface

## How It Works

### Detection Logic

When a user starts a search, the system:

1. **Generates Cache Key:** Creates a unique key based on:
   - Search type (Local, Global, Kad)
   - Query text (case-insensitive)
   - File size filters (min/max)
   - File type filters

2. **Checks Cache:** Looks up the cache key to see if an identical search exists

3. **Reuse or Create:**
   - **If found and active:** Reuse existing search by calling `RequestMoreResults()`
   - **If not found:** Create new search and register in cache

### Cache Key Format

```
<search_type>|<query>|min:<min_size>|max:<max_size>|types:<type1>,<type2>,...|
```

Example:
```
2|music|min:1048576|max:104857600|types:mp3,flac,ogg|
```

Where search_type:
- 0 = Local
- 1 = Global
- 2 = Kad

### Behavior

| Scenario | Action |
|----------|--------|
| First search with query "music" | Create new search, register in cache |
| Second search with query "music" (same type, same filters) | Reuse existing search, request more results |
| Search with query "Music" (different case) | Reuse existing search (case-insensitive) |
| Search with query "music" but different type | Create new search (different cache key) |
| Search with query "music" but different file size filter | Create new search (different cache key) |
| Existing search completed | Cache is marked inactive, new search will be created |

## Configuration

### Enable/Disable Search Cache

```cpp
// In UnifiedSearchManager config
UnifiedSearchManager::Config config;
config.enableSearchCache = true;  // Enable (default)
// or
config.enableSearchCache = false;  // Disable
```

### Cache Manager Configuration

```cpp
// In SearchCacheManager
struct Config {
    int maxCacheSize{100};        // Maximum searches to cache
    int maxAgeSeconds{3600};      // Maximum age before cleanup (1 hour)
    bool caseSensitive{false};    // Case-insensitive by default
};
```

## Usage Example

### Without Search Cache (Old Behavior)

```cpp
// User searches for "music"
SearchId id1 = adapter->StartSearch(params);  // Creates new search

// User searches for "music" again
SearchId id2 = adapter->StartSearch(params);  // Creates another new search (duplicate!)

// Result: Two identical search tabs, both searching independently
```

### With Search Cache (New Behavior)

```cpp
// User searches for "music"
SearchId id1 = adapter->StartSearch(params);  // Creates new search, registers in cache

// User searches for "music" again
SearchId id2 = adapter->StartSearch(params);  // Returns id1, requests more results

// Result: Single search tab showing all results from both searches
// User sees: "Requesting more results from existing search"
```

## Implementation Details

### Cache Manager

**SearchCacheManager** is responsible for:
- Storing active searches with their parameters
- Finding identical searches
- Registering new searches
- Updating search state
- Cleaning up old inactive searches

### UnifiedSearchManager Integration

The cache is integrated into `UnifiedSearchManager::ProcessCommand()`:

```cpp
case SearchCommand::Type::START_SEARCH: {
    // Check cache first
    if (m_cacheManager && m_config.enableSearchCache) {
        SearchId existingSearchId;
        if (m_cacheManager->FindExistingSearch(command.params, existingSearchId)) {
            // Reuse existing search
            engine->RequestMoreResults(existingSearchId);
            SendEvent(SearchEvent::ProgressUpdate(...));
            return;
        }
    }

    // Create new search
    SearchId searchId = engine->StartSearch(command.params);
    
    // Register in cache
    if (m_cacheManager && m_config.enableSearchCache) {
        m_cacheManager->RegisterSearch(searchId, command.params);
    }
}
```

### Cache Lifecycle

**Registration:**
- When a search starts, it's registered in the cache
- Marked as `isActive = true`

**Update:**
- When search state changes, cache is updated
- When search completes, marked as `isActive = false`

**Cleanup:**
- Old inactive searches are automatically removed
- Cleanup happens during maintenance
- Based on age (default: 1 hour)

## Testing

### Test Case 1: Identical Search Reuse

```cpp
// First search
SearchParams params1;
params1.type = SearchType::LOCAL;
params1.query = "music";
SearchId id1 = engine->StartSearch(params1);

// Second identical search
SearchParams params2;
params2.type = SearchType::LOCAL;
params2.query = "music";
SearchId id2 = engine->StartSearch(params2);

// Verify reuse
EXPECT_EQ(id1, id2);  // Should return same ID
```

### Test Case 2: Different Search Type

```cpp
SearchParams params1;
params1.type = SearchType::LOCAL;
params1.query = "music";

SearchParams params2;
params2.type = SearchType::GLOBAL;
params2.query = "music";

SearchId id1 = engine->StartSearch(params1);
SearchId id2 = engine->StartSearch(params2);

// Verify different IDs
EXPECT_NE(id1, id2);  // Different types = different searches
```

### Test Case 3: Different Filters

```cpp
SearchParams params1;
params1.type = SearchType::LOCAL;
params1.query = "music";
params1.minFileSize = 1024 * 1024;

SearchParams params2;
params2.type = SearchType::LOCAL;
params2.query = "music";
params2.minFileSize = 2 * 1024 * 1024;  // Different filter

SearchId id1 = engine->StartSearch(params1);
SearchId id2 = engine->StartSearch(params2);

// Verify different IDs
EXPECT_NE(id1, id2);  // Different filters = different searches
```

### Test Case 4: Case Insensitivity

```cpp
SearchParams params1;
params1.type = SearchType::LOCAL;
params1.query = "music";

SearchParams params2;
params2.type = SearchType::LOCAL;
params2.query = "MUSIC";  // Different case

SearchId id1 = engine->StartSearch(params1);
SearchId id2 = engine->StartSearch(params2);

// Verify reuse (case-insensitive)
EXPECT_EQ(id1, id2);
```

## Logging

### When Search is Reused

```
[UnifiedSearchManager] Reusing existing search: 1234567890
[UnifiedSearchManager] Requesting more results from existing search
```

### When New Search is Created

```
[UnifiedSearchManager] Starting local search 1234567890 for query: music
[SearchCacheManager] Registered search: 1234567890 with key: 0|music|
```

### When Cache is Cleaned Up

```
[SearchCacheManager] Cleaned up search: 1234567890
[SearchCacheManager] Cleaned up 5 old searches
```

## Performance Impact

### Memory Usage
- **Additional memory:** ~100 bytes per cached search
- **Maximum overhead:** ~10KB (100 searches × 100 bytes)
- **Negligible:** Compared to search result storage

### CPU Usage
- **Cache lookup:** O(1) hash table lookup
- **Cache key generation:** O(n) where n is query length
- **Overall:** Minimal overhead

### Network Traffic
- **Without cache:** 2 identical searches = 2 network requests
- **With cache:** 2 identical searches = 1 network request + 1 "more results" request
- **Savings:** ~50% reduction in duplicate searches

## User Experience

### Before (Without Cache)

1. User searches for "music"
2. New tab created: "Searching..."
3. Results appear after 5 seconds
4. User searches for "music" again
5. Another new tab created: "Searching..."
6. Results appear after 5 seconds
7. User has 2 identical tabs open

### After (With Cache)

1. User searches for "music"
2. New tab created: "Searching..."
3. Results appear after 5 seconds
4. User searches for "music" again
5. Same tab shows: "Requesting more results from existing search"
6. Additional results appear immediately (if cached)
7. User has 1 tab with all results

## Known Limitations

1. **Cache Size:** Limited to 100 searches by default
2. **Age-based Cleanup:** Searches older than 1 hour are removed
3. **State-based:** Only active (running/paused) searches are reused
4. **Exact Match:** Only exact parameter matches are considered identical

## Future Enhancements

1. **Partial Matching:** Reuse searches with similar but not identical parameters
2. **Smart Caching:** Cache search results for repeated queries
3. **Cache Statistics:** Track cache hit/miss ratio
4. **User Control:** Allow users to disable cache per-search
5. **Cache Visualization:** Show cached searches in UI

## Troubleshooting

### Issue: Searches not being reused

**Check:**
1. Is cache enabled? `config.enableSearchCache`
2. Are parameters exactly identical?
3. Is existing search still active (not completed/failed)?
4. Check logs for cache lookup messages

**Debug:**
```cpp
// Enable debug logging
std::cout << "[Cache] Key: " << cacheManager->GenerateCacheKey(params) << std::endl;

// Check cache contents
auto activeSearches = cacheManager->GetActiveSearches();
for (const auto& search : activeSearches) {
    std::cout << "[Cache] Active: " << search.searchId.ToString() << std::endl;
}
```

### Issue: Too many cached searches

**Solution:**
```cpp
// Reduce cache size
config.enableSearchCache = true;
// In SearchCacheManager constructor:
m_config.maxCacheSize = 50;  // Reduced from 100
```

## Conclusion

The search cache and reuse feature provides a significant improvement to user experience by intelligently reusing existing searches. This reduces duplicate network traffic, lowers resource usage, and provides a cleaner UI with fewer duplicate tabs.

**Status:** ✅ Implemented and tested  
**Configuration:** Enabled by default  
**Performance Impact:** Minimal overhead, significant savings
