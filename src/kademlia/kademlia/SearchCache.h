//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2011 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2024 aMule Team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

#ifndef __SEARCH_CACHE__
#define __SEARCH_CACHE__

#include "../utils/UInt128.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <list>
#include <mutex>
#include <chrono>

namespace Kademlia {

/**
 * Cached search result entry
 */
struct CachedSearchResult {
    CUInt128 target_id;
    std::string search_term;
    std::vector<CUInt128> file_ids;
    std::vector<std::string> file_names;
    uint32_t result_count;
    std::chrono::steady_clock::time_point timestamp;
    uint32_t access_count;
    std::chrono::steady_clock::time_point last_access;

    CachedSearchResult()
        : result_count(0)
        , access_count(0)
    {
    }
};

/**
 * LRU cache for Kademlia search results
 */
class SearchResultCache {
public:
    /**
     * Constructor
     * @param max_size Maximum number of entries in the cache
     * @param ttl_seconds Time-to-live for cache entries in seconds
     */
    SearchResultCache(size_t max_size = 1000, uint32_t ttl_seconds = 3600);

    /**
     * Destructor
     */
    ~SearchResultCache();

    /**
     * Check if a result is cached for the given target
     * @param target_id The target ID to look up
     * @return true if cached result exists and is valid
     */
    bool has_cached_result(const CUInt128& target_id) const;

    /**
     * Get cached result for the given target
     * @param target_id The target ID to look up
     * @return Cached result or nullptr if not found/expired
     */
    std::shared_ptr<CachedSearchResult> get_cached_result(const CUInt128& target_id);

    /**
     * Cache a search result
     * @param result The result to cache
     */
    void cache_result(const CachedSearchResult& result);

    /**
     * Cache a search result with parameters
     * @param target_id The target ID
     * @param search_term The search term
     * @param file_ids The file IDs found
     * @param file_names The file names found
     */
    void cache_result(
        const CUInt128& target_id,
        const std::string& search_term,
        const std::vector<CUInt128>& file_ids,
        const std::vector<std::string>& file_names);

    /**
     * Invalidate cache entry for the given target
     * @param target_id The target ID to invalidate
     */
    void invalidate(const CUInt128& target_id);

    /**
     * Clear all cache entries
     */
    void clear();

    /**
     * Remove expired entries from the cache
     */
    void cleanup_expired();

    /**
     * Get cache statistics
     */
    struct CacheStats {
        size_t size;
        size_t max_size;
        size_t hits;
        size_t misses;
        double hit_rate;
        size_t expired_removed;
    };

    CacheStats get_stats() const;

    /**
     * Set the maximum cache size
     * @param max_size New maximum size
     */
    void set_max_size(size_t max_size);

    /**
     * Set the time-to-live for cache entries
     * @param ttl_seconds New TTL in seconds
     */
    void set_ttl(uint32_t ttl_seconds);

private:
    /**
     * Check if a cache entry is expired
     * @param entry The entry to check
     * @return true if expired
     */
    bool is_expired(const CachedSearchResult& entry) const;

    /**
     * Evict the least recently used entry
     */
    void evict_lru();

    /**
     * Move an entry to the front of the LRU list
     * @param target_id The target ID of the entry
     */
    void touch(const CUInt128& target_id);

    mutable std::mutex mutex_;
    size_t max_size_;
    uint32_t ttl_seconds_;

    // LRU list: most recently used at front, least recently used at back
    std::list<CUInt128> lru_list_;

    // Map from target ID to cached result (using std::map instead of unordered_map)
    std::map<CUInt128, std::shared_ptr<CachedSearchResult>> cache_;

    // Statistics
    mutable size_t hits_;
    mutable size_t misses_;
    mutable size_t expired_removed_;
};

/**
 * Singleton accessor for the search result cache
 */
class SearchCacheManager {
public:
    static SearchCacheManager& instance();

    /**
     * Initialize the search cache
     * @param max_size Maximum cache size
     * @param ttl_seconds TTL for cache entries
     */
    void initialize(size_t max_size = 1000, uint32_t ttl_seconds = 3600);

    /**
     * Get the search cache
     */
    SearchResultCache& get_cache() { return *cache_; }

    /**
     * Shutdown the search cache
     */
    void shutdown();

    /**
     * Check if the cache is initialized
     */
    bool is_initialized() const { return initialized_; }

private:
    SearchCacheManager();
    ~SearchCacheManager();
    SearchCacheManager(const SearchCacheManager&) = delete;
    SearchCacheManager& operator=(const SearchCacheManager&) = delete;

    std::unique_ptr<SearchResultCache> cache_;
    bool initialized_;
    mutable std::mutex mutex_;
};

} // namespace Kademlia

#endif // __SEARCH_CACHE__
