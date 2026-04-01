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

#ifndef __PARALLEL_SEARCH__
#define __PARALLEL_SEARCH__

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <queue>

namespace Kademlia {

/**
 * Thread-safe task queue for parallel search operations
 */
class SearchTaskQueue {
public:
    using Task = std::function<void()>;

    SearchTaskQueue(size_t max_threads = 4);
    ~SearchTaskQueue();

    void enqueue(Task task);
    void wait_for_completion();
    void shutdown();

private:
    void worker_thread();

    std::vector<std::thread> workers_;
    std::queue<Task> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
    std::atomic<int> active_tasks_;
    std::condition_variable completion_cv_;
    size_t max_threads_;
};

/**
 * Parallel search executor for Kademlia operations
 */
class ParallelSearchExecutor {
public:
    ParallelSearchExecutor();
    ~ParallelSearchExecutor();

    /**
     * Execute search tasks in parallel across routing zones
     * @param tasks Vector of search tasks to execute
     * @return true if all tasks completed successfully
     */
    bool execute_parallel(const std::vector<std::function<void()>>& tasks);

    /**
     * Execute search tasks with results aggregation
     * @param tasks Vector of tasks that return results
     * @return Aggregated results from all tasks
     */
    template<typename T>
    std::vector<T> execute_with_results(const std::vector<std::function<T()>>& tasks);

    /**
     * Get the number of worker threads
     */
    size_t get_worker_count() const { return worker_count_; }

    /**
     * Set the number of worker threads
     */
    void set_worker_count(size_t count);

private:
    std::unique_ptr<SearchTaskQueue> task_queue_;
    size_t worker_count_;
    mutable std::mutex mutex_;
};

/**
 * Parallel search coordinator for managing concurrent searches
 */
class ParallelSearchCoordinator {
public:
    static ParallelSearchCoordinator& instance();

    /**
     * Initialize the parallel search system
     */
    void initialize(size_t worker_count = 4);

    /**
     * Shutdown the parallel search system
     */
    void shutdown();

    /**
     * Get the executor for parallel operations
     */
    ParallelSearchExecutor& get_executor() { return *executor_; }

    /**
     * Check if parallel search is enabled
     */
    bool is_enabled() const { return enabled_; }

    /**
     * Enable or disable parallel search
     */
    void set_enabled(bool enabled) { enabled_ = enabled; }

private:
    ParallelSearchCoordinator();
    ~ParallelSearchCoordinator();
    ParallelSearchCoordinator(const ParallelSearchCoordinator&) = delete;
    ParallelSearchCoordinator& operator=(const ParallelSearchCoordinator&) = delete;

    std::unique_ptr<ParallelSearchExecutor> executor_;
    bool enabled_;
    bool initialized_;
    mutable std::mutex mutex_;
};

} // namespace Kademlia

#endif // __PARALLEL_SEARCH__
