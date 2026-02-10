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

#include "SearchCache.h"
#include <algorithm>

namespace Kademlia {

// SearchResultCache implementation
SearchResultCache::SearchResultCache(size_t max_size, uint32_t ttl_seconds)
    : max_size_(max_size)
    , ttl_seconds_(ttl_seconds)
    , hits_(0)
    , misses_(0)
    , expired_removed_(0)
{
}

SearchResultCache::~SearchResultCache()
{
    std::lock_guard<std::mutex> lock(mutex_);
    cache_.clear();
    lru_list_.clear();
}

bool SearchResultCache::has_cached_result(const CUInt128& target_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_.find(target_id);
    if (it == cache_.end()) {
        return false;
    }

    if (is_expired(*it->second)) {
        return false;
    }

    return true;
}

std::shared_ptr<CachedSearchResult> SearchResultCache::get_cached_result(const CUInt128& target_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_.find(target_id);
    if (it == cache_.end()) {
        misses_++;
        return nullptr;
    }

    if (is_expired(*it->second)) {
        expired_removed_++;
        lru_list_.remove(target_id);
        cache_.erase(it);
        misses_++;
        return nullptr;
    }

    // Update access statistics
    it->second->access_count++;
    it->second->last_access = std::chrono::steady_clock::now();

    // Move to front of LRU list
    touch(target_id);

    hits_++;
    return it->second;
}

void SearchResultCache::cache_result(const CachedSearchResult& result)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if we need to evict entries
    if (cache_.size() >= max_size_) {
        evict_lru();
    }

    // Create a copy of the result with current timestamp
    auto cached = std::make_shared<CachedSearchResult>(result);
    cached->timestamp = std::chrono::steady_clock::now();
    cached->last_access = cached->timestamp;
    cached->access_count = 1;

    // Add to cache
    cache_[result.target_id] = cached;
    lru_list_.push_front(result.target_id);
}

void SearchResultCache::cache_result(
    const CUInt128& target_id,
    const std::string& search_term,
    const std::vector<CUInt128>& file_ids,
    const std::vector<std::string>& file_names)
{
    CachedSearchResult result;
    result.target_id = target_id;
    result.search_term = search_term;
    result.file_ids = file_ids;
    result.file_names = file_names;
    result.result_count = file_ids.size();

    cache_result(result);
}

void SearchResultCache::invalidate(const CUInt128& target_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    lru_list_.remove(target_id);
    cache_.erase(target_id);
}

void SearchResultCache::clear()
{
    std::lock_guard<std::mutex> lock(mutex_);

    cache_.clear();
    lru_list_.clear();
    hits_ = 0;
    misses_ = 0;
    expired_removed_ = 0;
}

void SearchResultCache::cleanup_expired()
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_.begin();
    while (it != cache_.end()) {
        if (is_expired(*it->second)) {
            lru_list_.remove(it->first);
            it = cache_.erase(it);
            expired_removed_++;
        } else {
            ++it;
        }
    }
}

SearchResultCache::CacheStats SearchResultCache::get_stats() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    CacheStats stats;
    stats.size = cache_.size();
    stats.max_size = max_size_;
    stats.hits = hits_;
    stats.misses = misses_;
    stats.hit_rate = (hits_ + misses_ > 0) ? 
        static_cast<double>(hits_) / (hits_ + misses_) : 0.0;
    stats.expired_removed = expired_removed_;

    return stats;
}

void SearchResultCache::set_max_size(size_t max_size)
{
    std::lock_guard<std::mutex> lock(mutex_);

    max_size_ = max_size;

    // Evict entries if necessary
    while (cache_.size() > max_size_) {
        evict_lru();
    }
}

void SearchResultCache::set_ttl(uint32_t ttl_seconds)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ttl_seconds_ = ttl_seconds;
}

bool SearchResultCache::is_expired(const CachedSearchResult& entry) const
{
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - entry.timestamp).count();
    return age >= ttl_seconds_;
}

void SearchResultCache::evict_lru()
{
    if (lru_list_.empty()) {
        return;
    }

    // Remove the least recently used entry (back of the list)
    CUInt128 target_id = lru_list_.back();
    lru_list_.pop_back();
    cache_.erase(target_id);
}

void SearchResultCache::touch(const CUInt128& target_id)
{
    // Move to front of LRU list
    lru_list_.remove(target_id);
    lru_list_.push_front(target_id);
}

// SearchCacheManager implementation
SearchCacheManager::SearchCacheManager()
    : initialized_(false)
{
}

SearchCacheManager::~SearchCacheManager()
{
    if (initialized_) {
        shutdown();
    }
}

SearchCacheManager& SearchCacheManager::instance()
{
    static SearchCacheManager instance;
    return instance;
}

void SearchCacheManager::initialize(size_t max_size, uint32_t ttl_seconds)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return;
    }

    cache_ = std::make_unique<SearchResultCache>(max_size, ttl_seconds);
    initialized_ = true;
}

void SearchCacheManager::shutdown()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        return;
    }

    cache_.reset();
    initialized_ = false;
}

} // namespace Kademlia
