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

#include "BootstrapManager.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <thread>
#include <future>

namespace Kademlia {

// BootstrapNode implementation
void BootstrapNode::record_success(double latency)
{
    success_count++;
    last_seen = std::chrono::steady_clock::now();
    last_used = last_seen;

    // Update latency with exponential moving average
    if (latency_ms == 0.0) {
        latency_ms = latency;
    } else {
        latency_ms = 0.7 * latency_ms + 0.3 * latency;
    }

    // Update success rate
    uint32_t total = success_count + failure_count;
    success_rate = static_cast<double>(success_count) / total;
}

void BootstrapNode::record_failure()
{
    failure_count++;
    last_used = std::chrono::steady_clock::now();

    // Update success rate
    uint32_t total = success_count + failure_count;
    success_rate = static_cast<double>(success_count) / total;
}

double BootstrapNode::calculate_quality_score() const
{
    // Calculate quality score based on multiple factors
    double score = 0.0;

    // Success rate (40% weight)
    score += 0.4 * success_rate;

    // Latency (30% weight, lower is better)
    double latency_score = 1.0 / (1.0 + latency_ms / 1000.0);
    score += 0.3 * latency_score;

    // Contact count (20% weight, more is better)
    double contact_score = std::min(1.0, static_cast<double>(contact_count) / 100.0);
    score += 0.2 * contact_score;

    // Recency (10% weight, more recent is better)
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::hours>(now - last_seen).count();
    double recency_score = std::max(0.0, 1.0 - age / 8760.0); // Decay over a year
    score += 0.1 * recency_score;

    return score;
}

// BootstrapManager implementation
BootstrapManager::BootstrapManager()
    : max_bootstrap_nodes_(100)
    , parallel_bootstrap_count_(4)
    , initialized_(false)
    , bootstrapping_(false)
    , successful_bootstraps_(0)
    , failed_bootstraps_(0)
{
}

BootstrapManager::~BootstrapManager()
{
    shutdown();
}

void BootstrapManager::initialize(size_t max_bootstrap_nodes, size_t parallel_bootstrap_count)
{
    std::lock_guard<std::mutex> lock(mutex_);

    max_bootstrap_nodes_ = max_bootstrap_nodes;
    parallel_bootstrap_count_ = parallel_bootstrap_count;
    initialized_ = true;
}

void BootstrapManager::shutdown()
{
    std::lock_guard<std::mutex> lock(mutex_);

    bootstrap_nodes_.clear();
    initialized_ = false;
    bootstrapping_ = false;
}

void BootstrapManager::add_bootstrap_node(const BootstrapNode& node)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Check if node already exists
    auto it = std::find_if(bootstrap_nodes_.begin(), bootstrap_nodes_.end(),
        [&node](const BootstrapNode& existing) {
            return existing.node_id == node.node_id;
        });

    if (it != bootstrap_nodes_.end()) {
        // Update existing node
        *it = node;
    } else {
        // Add new node
        bootstrap_nodes_.push_back(node);

        // Enforce maximum size
        if (bootstrap_nodes_.size() > max_bootstrap_nodes_) {
            sort_bootstrap_nodes();
            bootstrap_nodes_.pop_back();
        }
    }
}

void BootstrapManager::add_bootstrap_node(
    const CUInt128& node_id,
    uint32_t ip,
    uint16_t port,
    uint16_t tcp_port,
    uint8_t version)
{
    BootstrapNode node;
    node.node_id = node_id;
    node.ip = ip;
    node.port = port;
    node.tcp_port = tcp_port;
    node.version = version;
    node.last_seen = std::chrono::steady_clock::now();

    add_bootstrap_node(node);
}

void BootstrapManager::remove_bootstrap_node(const CUInt128& node_id)
{
    std::lock_guard<std::mutex> lock(mutex_);

    bootstrap_nodes_.erase(
        std::remove_if(bootstrap_nodes_.begin(), bootstrap_nodes_.end(),
            [&node_id](const BootstrapNode& node) {
                return node.node_id == node_id;
            }),
        bootstrap_nodes_.end());
}

std::vector<BootstrapNode> BootstrapManager::get_best_bootstrap_nodes(size_t count) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<BootstrapNode> result = bootstrap_nodes_;

    // Sort by quality score
    std::sort(result.begin(), result.end(),
        [](const BootstrapNode& a, const BootstrapNode& b) {
            return a.calculate_quality_score() > b.calculate_quality_score();
        });

    // Return top N nodes
    if (result.size() > count) {
        result.resize(count);
    }

    return result;
}

bool BootstrapManager::bootstrap(ProgressCallback callback)
{
    if (!initialized_) {
        return false;
    }

    if (bootstrapping_.exchange(true)) {
        return false; // Already bootstrapping
    }

    auto nodes = get_best_bootstrap_nodes(parallel_bootstrap_count_);
    if (nodes.empty()) {
        bootstrapping_ = false;
        return false;
    }

    bool success = false;
    std::vector<std::future<bool>> futures;

    // Start parallel bootstrap attempts
    for (auto& node : nodes) {
        futures.push_back(std::async(std::launch::async,
            [this, &node]() {
                return bootstrap_from_node(node);
            }));
    }

    // Wait for all attempts to complete
    for (size_t i = 0; i < futures.size(); ++i) {
        if (callback) {
            callback(i + 1, futures.size(), "Bootstrapping from node " + std::to_string(i + 1));
        }

        bool node_success = futures[i].get();
        update_node_stats(nodes[i].node_id, node_success, nodes[i].latency_ms);

        if (node_success) {
            success = true;
            successful_bootstraps_++;
        } else {
            failed_bootstraps_++;
        }
    }

    bootstrapping_ = false;
    return success;
}

void BootstrapManager::update_node_stats(const CUInt128& node_id, bool success, double latency_ms)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(bootstrap_nodes_.begin(), bootstrap_nodes_.end(),
        [&node_id](const BootstrapNode& node) {
            return node.node_id == node_id;
        });

    if (it != bootstrap_nodes_.end()) {
        if (success) {
            it->record_success(latency_ms);
        } else {
            it->record_failure();
        }
    }
}

bool BootstrapManager::load_bootstrap_nodes(const std::string& filename)
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    bootstrap_nodes_.clear();

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        BootstrapNode node;

        // Parse line: node_id ip port tcp_port version success_count failure_count latency_ms contact_count
        std::string node_id_str;
        if (!(iss >> node_id_str >> node.ip >> node.port >> node.tcp_port 
              >> node.version >> node.success_count >> node.failure_count 
              >> node.latency_ms >> node.contact_count)) {
            continue;
        }

        // Parse node ID
        node.node_id.SetValueBE((const uint8_t*)node_id_str.c_str());

        // Calculate success rate
        uint32_t total = node.success_count + node.failure_count;
        node.success_rate = total > 0 ? 
            static_cast<double>(node.success_count) / total : 0.0;

        bootstrap_nodes_.push_back(node);
    }

    return true;
}

bool BootstrapManager::save_bootstrap_nodes(const std::string& filename) const
{
    std::lock_guard<std::mutex> lock(mutex_);

    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    // Write header
    file << "# Bootstrap nodes for aMule Kademlia\n";
    file << "# Format: node_id ip port tcp_port version success_count failure_count latency_ms contact_count\n";

    for (const auto& node : bootstrap_nodes_) {
        // Write node ID as hex string
        wxString node_id_str = node.node_id.ToHexString();

        file << node_id_str.mb_str() << " "
             << node.ip << " "
             << node.port << " "
             << node.tcp_port << " "
             << static_cast<int>(node.version) << " "
             << node.success_count << " "
             << node.failure_count << " "
             << node.latency_ms << " "
             << node.contact_count << "\n";
    }

    return true;
}

BootstrapManager::BootstrapStats BootstrapManager::get_stats() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    BootstrapStats stats;
    stats.total_nodes = bootstrap_nodes_.size();
    stats.successful_bootstraps = successful_bootstraps_.load();
    stats.failed_bootstraps = failed_bootstraps_.load();

    // Count active nodes (seen in last 24 hours)
    auto now = std::chrono::steady_clock::now();
    stats.active_nodes = std::count_if(bootstrap_nodes_.begin(), bootstrap_nodes_.end(),
        [&now](const BootstrapNode& node) {
            auto age = std::chrono::duration_cast<std::chrono::hours>(now - node.last_seen).count();
            return age < 24;
        });

    // Calculate averages
    if (!bootstrap_nodes_.empty()) {
        double total_latency = 0.0;
        double total_success_rate = 0.0;

        for (const auto& node : bootstrap_nodes_) {
            total_latency += node.latency_ms;
            total_success_rate += node.success_rate;
        }

        stats.average_latency_ms = total_latency / bootstrap_nodes_.size();
        stats.average_success_rate = total_success_rate / bootstrap_nodes_.size();
    }

    return stats;
}

void BootstrapManager::cleanup_stale_nodes(uint32_t max_age_days)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();

    bootstrap_nodes_.erase(
        std::remove_if(bootstrap_nodes_.begin(), bootstrap_nodes_.end(),
            [&now, max_age_days](const BootstrapNode& node) {
                auto age = std::chrono::duration_cast<std::chrono::hours>(now - node.last_seen).count();
                return age > (max_age_days * 24);
            }),
        bootstrap_nodes_.end());
}

void BootstrapManager::set_parallel_bootstrap_count(size_t count)
{
    std::lock_guard<std::mutex> lock(mutex_);
    parallel_bootstrap_count_ = count;
}

void BootstrapManager::sort_bootstrap_nodes()
{
    std::sort(bootstrap_nodes_.begin(), bootstrap_nodes_.end(),
        [](const BootstrapNode& a, const BootstrapNode& b) {
            return a.calculate_quality_score() > b.calculate_quality_score();
        });
}

bool BootstrapManager::bootstrap_from_node(BootstrapNode& node)
{
    // This is a placeholder - actual implementation would connect to the node
    // and perform the bootstrap operation
    // For now, we'll just simulate a successful bootstrap

    // Simulate network latency
    std::this_thread::sleep_for(std::chrono::milliseconds(100 + rand() % 200));

    // Simulate success rate based on node's quality
    double quality = node.calculate_quality_score();
    return (rand() / static_cast<double>(RAND_MAX)) < quality;
}

// BootstrapManagerAccessor implementation
BootstrapManagerAccessor::BootstrapManagerAccessor()
    : initialized_(false)
{
}

BootstrapManagerAccessor::~BootstrapManagerAccessor()
{
    if (initialized_) {
        shutdown();
    }
}

BootstrapManagerAccessor& BootstrapManagerAccessor::instance()
{
    static BootstrapManagerAccessor instance;
    return instance;
}

void BootstrapManagerAccessor::initialize(size_t max_bootstrap_nodes, size_t parallel_bootstrap_count)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return;
    }

    manager_ = std::make_unique<BootstrapManager>();
    manager_->initialize(max_bootstrap_nodes, parallel_bootstrap_count);
    initialized_ = true;
}

void BootstrapManagerAccessor::shutdown()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        return;
    }

    manager_.reset();
    initialized_ = false;
}

} // namespace Kademlia
