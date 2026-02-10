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

#include "ParallelSearch.h"
#include <stdexcept>

namespace Kademlia {

// SearchTaskQueue implementation
SearchTaskQueue::SearchTaskQueue(size_t max_threads)
    : stop_(false), active_tasks_(0), max_threads_(max_threads)
{
    for (size_t i = 0; i < max_threads_; ++i) {
        workers_.emplace_back(&SearchTaskQueue::worker_thread, this);
    }
}

SearchTaskQueue::~SearchTaskQueue()
{
    shutdown();
}

void SearchTaskQueue::enqueue(Task task)
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (stop_) {
            throw std::runtime_error("enqueue on stopped SearchTaskQueue");
        }
        tasks_.push(task);
        active_tasks_++;
    }
    condition_.notify_one();
}

void SearchTaskQueue::wait_for_completion()
{
    std::unique_lock<std::mutex> lock(queue_mutex_);
    completion_cv_.wait(lock, [this] { return active_tasks_ == 0 && tasks_.empty(); });
}

void SearchTaskQueue::shutdown()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
}

void SearchTaskQueue::worker_thread()
{
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this] { return stop_ || !tasks_.empty(); });

            if (stop_ && tasks_.empty()) {
                return;
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        // Execute the task
        task();

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            active_tasks_--;
            if (active_tasks_ == 0 && tasks_.empty()) {
                completion_cv_.notify_all();
            }
        }
    }
}

// ParallelSearchExecutor implementation
ParallelSearchExecutor::ParallelSearchExecutor()
    : worker_count_(4)
{
    task_queue_ = std::make_unique<SearchTaskQueue>(worker_count_);
}

ParallelSearchExecutor::~ParallelSearchExecutor()
{
    if (task_queue_) {
        task_queue_->shutdown();
    }
}

bool ParallelSearchExecutor::execute_parallel(const std::vector<std::function<void()>>& tasks)
{
    if (tasks.empty()) {
        return true;
    }

    try {
        for (const auto& task : tasks) {
            task_queue_->enqueue(task);
        }
        task_queue_->wait_for_completion();
        return true;
    } catch (...) {
        return false;
    }
}

template<typename T>
std::vector<T> ParallelSearchExecutor::execute_with_results(const std::vector<std::function<T()>>& tasks)
{
    std::vector<T> results(tasks.size());
    std::vector<std::exception_ptr> exceptions(tasks.size());
    std::mutex results_mutex;

    std::vector<std::function<void()>> wrapper_tasks;
    for (size_t i = 0; i < tasks.size(); ++i) {
        wrapper_tasks.push_back([i, &tasks, &results, &exceptions, &results_mutex]() {
            try {
                results[i] = tasks[i]();
            } catch (...) {
                std::lock_guard<std::mutex> lock(results_mutex);
                exceptions[i] = std::current_exception();
            }
        });
    }

    if (!execute_parallel(wrapper_tasks)) {
        throw std::runtime_error("Failed to execute parallel tasks");
    }

    // Check for exceptions
    for (const auto& exc : exceptions) {
        if (exc) {
            std::rethrow_exception(exc);
        }
    }

    return results;
}

// Explicit template instantiation for common types
template std::vector<int> ParallelSearchExecutor::execute_with_results<int>(
    const std::vector<std::function<int()>>&);
template std::vector<uint32_t> ParallelSearchExecutor::execute_with_results<uint32_t>(
    const std::vector<std::function<uint32_t()>>&);
template std::vector<bool> ParallelSearchExecutor::execute_with_results<bool>(
    const std::vector<std::function<bool()>>&);

void ParallelSearchExecutor::set_worker_count(size_t count)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (count == worker_count_) {
        return;
    }

    task_queue_->shutdown();
    worker_count_ = count;
    task_queue_ = std::make_unique<SearchTaskQueue>(worker_count_);
}

// ParallelSearchCoordinator implementation
ParallelSearchCoordinator::ParallelSearchCoordinator()
    : enabled_(true), initialized_(false)
{
}

ParallelSearchCoordinator::~ParallelSearchCoordinator()
{
    if (initialized_) {
        shutdown();
    }
}

ParallelSearchCoordinator& ParallelSearchCoordinator::instance()
{
    static ParallelSearchCoordinator instance;
    return instance;
}

void ParallelSearchCoordinator::initialize(size_t worker_count)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (initialized_) {
        return;
    }

    executor_ = std::make_unique<ParallelSearchExecutor>();
    executor_->set_worker_count(worker_count);
    initialized_ = true;
}

void ParallelSearchCoordinator::shutdown()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!initialized_) {
        return;
    }

    executor_.reset();
    initialized_ = false;
}

} // namespace Kademlia
