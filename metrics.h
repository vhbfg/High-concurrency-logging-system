#pragma once

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <string>

class Metrics {
public:
    Metrics() { reset(); }

    void reset();
    void record_latency(uint64_t ns);
    void record_blocked(uint64_t ns);
    void record_queue_state(std::size_t queue_size, std::size_t active_capacity);

    void inc_enqueued() { enqueued_.fetch_add(1, std::memory_order_relaxed); }
    void inc_dropped()  { dropped_.fetch_add(1, std::memory_order_relaxed); }
    void inc_written()  { written_.fetch_add(1, std::memory_order_relaxed); }
    void inc_rejected_after_close() {
        rejected_after_close_.fetch_add(1, std::memory_order_relaxed);
    }
    void inc_max_capacity_hit() {
        max_capacity_hit_count_.fetch_add(1, std::memory_order_relaxed);
    }

    uint64_t enqueued() const { return enqueued_.load(std::memory_order_relaxed); }
    uint64_t dropped()  const { return dropped_.load(std::memory_order_relaxed); }
    uint64_t written()  const { return written_.load(std::memory_order_relaxed); }
    uint64_t rejected_after_close() const {
        return rejected_after_close_.load(std::memory_order_relaxed);
    }
    uint64_t blocked_count() const {
        return blocked_count_.load(std::memory_order_relaxed);
    }
    uint64_t blocked_ns() const {
        return blocked_ns_.load(std::memory_order_relaxed);
    }
    uint64_t queue_peak_size() const {
        return queue_peak_size_.load(std::memory_order_relaxed);
    }
    uint64_t active_capacity() const {
        return active_capacity_.load(std::memory_order_relaxed);
    }
    uint64_t max_capacity_hit_count() const {
        return max_capacity_hit_count_.load(std::memory_order_relaxed);
    }

    struct Summary {
        uint64_t enqueued = 0, dropped = 0, written = 0, rejected_after_close = 0;
        uint64_t blocked_count = 0, blocked_ns = 0;
        uint64_t queue_peak_size = 0, active_capacity = 0, max_capacity_hit_count = 0;
        double   drop_rate = 0.0;
        uint64_t p50_ns = 0, p90_ns = 0, p99_ns = 0, avg_ns = 0;
    };

    Summary compute() const;
    void    report(const std::string& tag) const;

private:
    static constexpr int BUCKETS = 64;

    std::atomic_uint_fast64_t enqueued_{0}, dropped_{0}, written_{0};
    std::atomic_uint_fast64_t rejected_after_close_{0};
    std::atomic_uint_fast64_t blocked_count_{0}, blocked_ns_{0};
    std::atomic_uint_fast64_t queue_peak_size_{0}, active_capacity_{0};
    std::atomic_uint_fast64_t max_capacity_hit_count_{0};
    std::atomic_uint_fast64_t latency_sum_{0}, latency_cnt_{0};
    std::atomic_uint_fast64_t hist_[BUCKETS];
};
