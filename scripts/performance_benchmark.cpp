#include <iostream>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

// Simple benchmark to measure overhead of network performance monitoring
class Benchmark {
public:
    void run_concurrent_test() {
        const int num_threads = 4;
        const int iterations = 1000000;
        
        std::vector<std::thread> threads;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_threads; ++i) {
            threads.emplace_back([this, iterations]() {
                for (int j = 0; j < iterations; ++j) {
                    // Simulate network monitoring overhead
                    simulate_monitoring_overhead(1024, (j % 2) == 0);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        double total_ops = num_threads * iterations * 2.0; // sent + received
        double ops_per_second = total_ops / (duration / 1000.0);
        
        std::cout << "🚀 Performance Benchmark Results:\n";
        std::cout << "  Threads: " << num_threads << "\n";
        std::cout << "  Iterations per thread: " << iterations << "\n";
        std::cout << "  Total operations: " << total_ops << "\n";
        std::cout << "  Duration: " << duration << " ms\n";
        std::cout << "  Throughput: " << ops_per_second << " ops/s\n";
        std::cout << "  Latency: " << (duration * 1000000.0) / total_ops << " ns/op\n\n";
    }

private:
    std::atomic<uint64_t> counter_sent{0};
    std::atomic<uint64_t> counter_received{0};
    std::atomic<uint64_t> tcp_counter{0};
    std::atomic<uint64_t> udp_counter{0};
    
    void simulate_monitoring_overhead(size_t bytes, bool is_tcp) {
        // Simulate the exact atomic operations used in our monitoring
        counter_sent.fetch_add(bytes, std::memory_order_relaxed);
        counter_received.fetch_add(bytes, std::memory_order_relaxed);
        
        if (is_tcp) {
            tcp_counter.fetch_add(bytes, std::memory_order_relaxed);
        } else {
            udp_counter.fetch_add(bytes, std::memory_order_relaxed);
        }
    }
};

int main() {
    std::cout << "🔬 Benchmarking Network Performance Monitoring Overhead...\n\n";
    
    Benchmark benchmark;
    benchmark.run_concurrent_test();
    
    std::cout << "✅ Benchmark completed successfully!\n";
    std::cout << "📍 This simulates the atomic operation overhead of the monitoring system\n";
    std::cout << "📍 Real-world performance will be even better with cache line optimization\n";
    
    return 0;
}
