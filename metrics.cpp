#include "metrics.h"

#include <cstdio>

static int bucket_of(uint64_t ns) {
    if (ns == 0) return 0;
    int b = 63 - __builtin_clzll(ns);
    if (b < 0) b = 0;
    if (b > 63) b = 63;
    return b;
}

static void atomic_max(std::atomic_uint_fast64_t& target, uint64_t value) {
    uint64_t current = target.load(std::memory_order_relaxed);
    while (current < value &&
           !target.compare_exchange_weak(
               current, value, std::memory_order_relaxed, std::memory_order_relaxed)) {
    }
}

void Metrics::reset() {
    enqueued_.store(0);
    dropped_.store(0);
    written_.store(0);
    rejected_after_close_.store(0);
    blocked_count_.store(0);
    blocked_ns_.store(0);
    queue_peak_size_.store(0);
    active_capacity_.store(0);
    max_capacity_hit_count_.store(0);
    latency_sum_.store(0);
    latency_cnt_.store(0);
    for (int i = 0; i < BUCKETS; ++i) {
        hist_[i].store(0, std::memory_order_relaxed);
    }
}

void Metrics::record_latency(uint64_t ns) {
    hist_[bucket_of(ns)].fetch_add(1, std::memory_order_relaxed);
    latency_sum_.fetch_add(ns, std::memory_order_relaxed);
    latency_cnt_.fetch_add(1, std::memory_order_relaxed);
}

void Metrics::record_blocked(uint64_t ns) {
    blocked_count_.fetch_add(1, std::memory_order_relaxed);
    blocked_ns_.fetch_add(ns, std::memory_order_relaxed);
}

void Metrics::record_queue_state(std::size_t queue_size, std::size_t active_capacity) {
    atomic_max(queue_peak_size_, (uint64_t)queue_size);
    active_capacity_.store((uint64_t)active_capacity, std::memory_order_relaxed);
}

Metrics::Summary Metrics::compute() const {
    Summary s;
    s.enqueued = enqueued();
    s.dropped  = dropped();
    s.written  = written();
    s.rejected_after_close = rejected_after_close();
    s.blocked_count = blocked_count();
    s.blocked_ns = blocked_ns();
    s.queue_peak_size = queue_peak_size();
    s.active_capacity = active_capacity();
    s.max_capacity_hit_count = max_capacity_hit_count();

    uint64_t attempts = s.enqueued + s.dropped;
    s.drop_rate = attempts ? (double)s.dropped / (double)attempts : 0.0;

    uint64_t cnt = latency_cnt_.load(std::memory_order_relaxed);
    uint64_t sum = latency_sum_.load(std::memory_order_relaxed);
    s.avg_ns = cnt ? sum / cnt : 0;

    auto percentile = [&](double p) -> uint64_t {
        if (cnt == 0) return 0;
        uint64_t target = (uint64_t)(p * (double)cnt);
        if (target == 0) target = 1;
        uint64_t acc = 0;
        for (int b = 0; b < BUCKETS; ++b) {
            acc += hist_[b].load(std::memory_order_relaxed);
            if (acc >= target) return (b == 0) ? 0 : (1ull << b);
        }
        return 1ull << 63;
    };

    s.p50_ns = percentile(0.50);
    s.p90_ns = percentile(0.90);
    s.p99_ns = percentile(0.99);
    return s;
}

void Metrics::report(const std::string& tag) const {
    Summary s = compute();
    std::printf("[metrics %s] enqueued=%llu dropped=%llu written=%llu "
                "rejected_after_close=%llu drop=%.3f%% avg=%lluns "
                "p50=%lluns p90=%lluns p99=%lluns blocked=%llu "
                "blocked_ms=%.3f queue_peak=%llu active_capacity=%llu "
                "max_capacity_hits=%llu\n",
                tag.c_str(),
                (unsigned long long)s.enqueued,
                (unsigned long long)s.dropped,
                (unsigned long long)s.written,
                (unsigned long long)s.rejected_after_close,
                s.drop_rate * 100.0,
                (unsigned long long)s.avg_ns,
                (unsigned long long)s.p50_ns,
                (unsigned long long)s.p90_ns,
                (unsigned long long)s.p99_ns,
                (unsigned long long)s.blocked_count,
                (double)s.blocked_ns / 1000000.0,
                (unsigned long long)s.queue_peak_size,
                (unsigned long long)s.active_capacity,
                (unsigned long long)s.max_capacity_hit_count);
}
