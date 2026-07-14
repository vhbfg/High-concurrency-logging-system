#include "benchmark.h"
#include "log_common.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using clk = std::chrono::steady_clock;

static const char* queue_kind_name(QueueKind kind) {
    switch (kind) {
        case QueueKind::Mutex: return "mutex";
        case QueueKind::LockFree: return "lockfree";
        case QueueKind::AdaptiveBlocking: return "adaptive";
    }
    return "unknown";
}

BenchResult bench_run(const BenchConfig& cfg) {
    fs::remove_all(cfg.dir);

    Logger logger;
    Logger::Config lc;
    lc.dir       = cfg.dir;
    lc.min_level = LogLevel::DEBUG;
    lc.queue     = cfg.queue;
    lc.queue_capacity = cfg.queue_capacity;
    lc.max_queue_capacity = cfg.max_queue_capacity;
    logger.init(lc);

    for (int i = 0; i < cfg.warmup; ++i) {
        logger.app(LogLevel::INFO, "warmup %d", i);
    }

    auto produce = [&](int id) {
        for (int i = 0; i < cfg.per_thread; ++i) {
            logger.app(LogLevel::INFO, "t%d bench message #%d payload", id, i);
        }
    };

    auto t0 = clk::now();
    std::vector<std::thread> workers;
    workers.reserve(cfg.threads);
    for (int t = 0; t < cfg.threads; ++t) {
        workers.emplace_back(produce, t);
    }
    for (auto& w : workers) w.join();
    auto t1 = clk::now();

    logger.close();
    auto t2 = clk::now();

    double producer_seconds = std::chrono::duration<double>(t1 - t0).count();
    double total_seconds = std::chrono::duration<double>(t2 - t0).count();

    auto s = logger.metrics().compute();
    BenchResult r;
    r.queue      = queue_kind_name(cfg.queue);
    r.threads    = cfg.threads;
    r.producer_seconds = producer_seconds;
    r.total_seconds = total_seconds;
    r.enqueued   = s.enqueued;
    r.dropped    = s.dropped;
    r.written    = s.written;
    r.rejected_after_close = s.rejected_after_close;
    r.drop_rate  = s.drop_rate;
    r.p50_ns     = s.p50_ns;
    r.p90_ns     = s.p90_ns;
    r.p99_ns     = s.p99_ns;
    r.avg_ns     = s.avg_ns;
    r.blocked_count = s.blocked_count;
    r.blocked_ns = s.blocked_ns;
    r.queue_peak_size = s.queue_peak_size;
    r.active_capacity = s.active_capacity;
    r.max_capacity_hit_count = s.max_capacity_hit_count;

    double produced = (double)cfg.threads * (double)cfg.per_thread;
    r.business_per_sec = producer_seconds > 0 ? produced / producer_seconds : 0.0;
    r.enqueue_per_sec = producer_seconds > 0 ? (double)s.enqueued / producer_seconds : 0.0;
    r.write_per_sec = total_seconds > 0 ? (double)s.written / total_seconds : 0.0;
    return r;
}

static void print_header() {
    std::printf("\n%-9s %7s %8s %12s %12s %12s %9s %10s %9s %8s %8s %8s\n",
                "queue", "threads", "total_s", "business/s", "enqueue/s",
                "write/s", "p99(ns)", "drop%", "block_ms", "peak", "cap", "hits");
    std::printf("-------------------------------------------------------------------------------------------------------------------\n");
}

static void print_row(const BenchResult& r) {
    std::printf("%-9s %7d %8.3f %12.0f %12.0f %12.0f %9llu %9.2f %10.3f %8llu %8llu %8llu\n",
                r.queue.c_str(), r.threads, r.total_seconds,
                r.business_per_sec, r.enqueue_per_sec, r.write_per_sec,
                (unsigned long long)r.p99_ns, r.drop_rate * 100.0,
                (double)r.blocked_ns / 1000000.0,
                (unsigned long long)r.queue_peak_size,
                (unsigned long long)r.active_capacity,
                (unsigned long long)r.max_capacity_hit_count);
}

void bench_compare(const std::vector<int>& thread_counts, int per_thread) {
    std::printf("Benchmark: per-thread=%d, fixed_queue_capacity=%zu, "
                "adaptive_initial=%zu, adaptive_max=%zu, file_lines=%zu\n",
                per_thread, LOG_QUEUE_CAPACITY,
                ADAPTIVE_QUEUE_INITIAL_CAPACITY, ADAPTIVE_QUEUE_MAX_CAPACITY,
                MAX_LINES_PER_FILE);
    print_header();
    for (int tc : thread_counts) {
        for (QueueKind kind : {QueueKind::Mutex,
                               QueueKind::LockFree,
                               QueueKind::AdaptiveBlocking}) {
            BenchConfig cfg;
            cfg.threads    = tc;
            cfg.per_thread = per_thread;
            cfg.queue      = kind;
            cfg.dir        = "bench_logs";
            print_row(bench_run(cfg));
        }
        std::printf("-------------------------------------------------------------------------------------------------------------------\n");
    }
}
