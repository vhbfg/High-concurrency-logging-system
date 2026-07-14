#pragma once

#include "logger.h"

#include <cstdint>
#include <string>
#include <vector>

struct BenchConfig {
    int         threads    = 8;
    int         per_thread = 50000;
    int         warmup     = 0;
    QueueKind   queue      = QueueKind::AdaptiveBlocking;
    std::size_t queue_capacity = 0;
    std::size_t max_queue_capacity = 0;
    std::string dir        = "bench_logs";
};

struct BenchResult {
    std::string queue;
    int         threads    = 0;
    double      producer_seconds = 0.0;
    double      total_seconds = 0.0;
    double      business_per_sec = 0.0;
    double      enqueue_per_sec = 0.0;
    double      write_per_sec = 0.0;
    uint64_t    p50_ns = 0, p90_ns = 0, p99_ns = 0, avg_ns = 0;
    double      drop_rate  = 0.0;
    uint64_t    enqueued = 0, dropped = 0, written = 0;
    uint64_t    blocked_count = 0, blocked_ns = 0;
    uint64_t    queue_peak_size = 0, active_capacity = 0, max_capacity_hit_count = 0;
    uint64_t    rejected_after_close = 0;
};

BenchResult bench_run(const BenchConfig& cfg);
void        bench_compare(const std::vector<int>& thread_counts, int per_thread);
