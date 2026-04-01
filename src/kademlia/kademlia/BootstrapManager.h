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

#ifndef __BOOTSTRAP_MANAGER__
#define __BOOTSTRAP_MANAGER__

#include "../utils/UInt128.h"
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <chrono>
#include <atomic>
#include <functional>

namespace Kademlia {

/**
 * Bootstrap node information
 */
struct BootstrapNode {
    CUInt128 node_id;
    uint32_t ip;
    uint16_t port;
    uint16_t tcp_port;
    uint8_t version;
    std::chrono::steady_clock::time_point last_seen;
    std::chrono::steady_clock::time_point last_used;
    uint32_t success_count;
    uint32_t failure_count;
    double success_rate;
    double latency_ms;
    uint32_t contact_count;

    BootstrapNode()
        : ip(0)
        , port(0)
        , tcp_port(0)
        , version(0)
        , success_count(0)
        , failure_count(0)
        , success_rate(0.0)
        , latency_ms(0.0)
        , contact_count(0)
    {
    }

    /**
     * Update success statistics
     */
    void record_success(double latency);

    /**
     * Update failure statistics
     */
    void record_failure();

    /**
     * Calculate quality score (higher is better)
     */
    double calculate_quality_score() const;
};

/**
 * Bootstrap manager for intelligent node selection and parallel bootstrap
 */
class BootstrapManager {
public:
    /**
     * Callback type for bootstrap progress
     */
    using ProgressCallback = std::function<void(uint32_t completed, uint32_t total, const std::string& message)>;

    /**
     * Constructor
     */
    BootstrapManager();

    /**
     * Destructor
     */
    ~BootstrapManager();

    /**
     * Initialize the bootstrap manager
     * @param max_bootstrap_nodes Maximum number of bootstrap nodes to track
     * @param parallel_bootstrap_count Number of parallel bootstrap attempts
     */
    void initialize(size_t max_bootstrap_nodes = 100, size_t parallel_bootstrap_count = 4);

    /**
     * Shutdown the bootstrap manager
     */
    void shutdown();

    /**
     * Add a bootstrap node
     * @param node The node to add
     */
    void add_bootstrap_node(const BootstrapNode& node);

    /**
     * Add a bootstrap node with parameters
     */
    void add_bootstrap_node(
        const CUInt128& node_id,
        uint32_t ip,
        uint16_t port,
        uint16_t tcp_port,
        uint8_t version);

    /**
     * Remove a bootstrap node
     * @param node_id The ID of the node to remove
     */
    void remove_bootstrap_node(const CUInt128& node_id);

    /**
     * Get the best bootstrap nodes
     * @param count Number of nodes to return
     * @return Vector of best bootstrap nodes
     */
    std::vector<BootstrapNode> get_best_bootstrap_nodes(size_t count) const;

    /**
     * Perform parallel bootstrap
     * @param callback Optional callback for progress updates
     * @return true if bootstrap was successful
     */
    bool bootstrap(ProgressCallback callback = nullptr);

    /**
     * Update node statistics after bootstrap attempt
     * @param node_id The node ID
     * @param success Whether the attempt was successful
     * @param latency_ms Latency in milliseconds
     */
    void update_node_stats(const CUInt128& node_id, bool success, double latency_ms);

    /**
     * Load bootstrap nodes from file
     * @param filename Path to the file
     * @return true if successful
     */
    bool load_bootstrap_nodes(const std::string& filename);

    /**
     * Save bootstrap nodes to file
     * @param filename Path to the file
     * @return true if successful
     */
    bool save_bootstrap_nodes(const std::string& filename) const;

    /**
     * Get bootstrap statistics
     */
    struct BootstrapStats {
        size_t total_nodes;
        size_t active_nodes;
        size_t successful_bootstraps;
        size_t failed_bootstraps;
        double average_latency_ms;
        double average_success_rate;
    };

    BootstrapStats get_stats() const;

    /**
     * Cleanup stale bootstrap nodes
     * @param max_age_days Maximum age in days
     */
    void cleanup_stale_nodes(uint32_t max_age_days = 30);

    /**
     * Set the number of parallel bootstrap attempts
     */
    void set_parallel_bootstrap_count(size_t count);

    /**
     * Get the number of parallel bootstrap attempts
     */
    size_t get_parallel_bootstrap_count() const { return parallel_bootstrap_count_; }

private:
    /**
     * Sort bootstrap nodes by quality score
     */
    void sort_bootstrap_nodes();

    /**
     * Perform bootstrap attempt on a single node
     * @param node The node to bootstrap from
     * @return true if successful
     */
    bool bootstrap_from_node(BootstrapNode& node);

    mutable std::mutex mutex_;
    std::vector<BootstrapNode> bootstrap_nodes_;
    size_t max_bootstrap_nodes_;
    size_t parallel_bootstrap_count_;
    std::atomic<bool> initialized_;
    std::atomic<bool> bootstrapping_;

    // Statistics
    std::atomic<uint32_t> successful_bootstraps_;
    std::atomic<uint32_t> failed_bootstraps_;
};

/**
 * Singleton accessor for the bootstrap manager
 */
class BootstrapManagerAccessor {
public:
    static BootstrapManagerAccessor& instance();

    /**
     * Initialize the bootstrap manager
     */
    void initialize(size_t max_bootstrap_nodes = 100, size_t parallel_bootstrap_count = 4);

    /**
     * Get the bootstrap manager
     */
    BootstrapManager& get_manager() { return *manager_; }

    /**
     * Shutdown the bootstrap manager
     */
    void shutdown();

    /**
     * Check if the bootstrap manager is initialized
     */
    bool is_initialized() const { return initialized_; }

private:
    BootstrapManagerAccessor();
    ~BootstrapManagerAccessor();
    BootstrapManagerAccessor(const BootstrapManagerAccessor&) = delete;
    BootstrapManagerAccessor& operator=(const BootstrapManagerAccessor&) = delete;

    std::unique_ptr<BootstrapManager> manager_;
    bool initialized_;
    mutable std::mutex mutex_;
};

} // namespace Kademlia

#endif // __BOOTSTRAP_MANAGER__
