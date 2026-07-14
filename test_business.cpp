#include "test_business.h"
#include "logger.h"
#include "log_queue.h"
#include "log_crypto.h"
#include "log_common.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

void simulate_login(int id, int count) {
    for (int i = 0; i < count; ++i) {
        log_app(LogLevel::INFO, "thread %d login user #%d", id, i);
        log_operation(LogLevel::INFO, "thread %d login audit #%d", id, i);
    }
}

void simulate_order(int id, int count) {
    for (int i = 0; i < count; ++i) {
        log_app(LogLevel::INFO, "thread %d order created #%d", id, i);
        log_operation(LogLevel::INFO, "thread %d order updated #%d", id, i);
    }
}

void simulate_file_task(int id, int count) {
    for (int i = 0; i < count; ++i) {
        log_app(LogLevel::INFO, "thread %d file upload start #%d", id, i);
        log_operation(LogLevel::INFO, "thread %d file upload finish #%d", id, i);
    }
}

void simulate_error(int id, int count) {
    for (int i = 0; i < count; ++i) {
        log_app(LogLevel::WARN, "thread %d warning before error #%d", id, i);
        log_operation(LogLevel::INFO, "thread %d error handling #%d", id, i);
        log_error("thread %d simulated error #%d", id, i);
    }
}

static void business_thread_func(int id, int count) {
    switch (id % 4) {
        case 0: simulate_login(id, count); break;
        case 1: simulate_order(id, count); break;
        case 2: simulate_file_task(id, count); break;
        default: simulate_error(id, count); break;
    }
}

void run_business_test(int num_threads, int per_thread) {
    std::vector<std::thread> workers;
    workers.reserve(num_threads);
    for (int t = 0; t < num_threads; ++t) {
        workers.emplace_back(business_thread_func, t, per_thread);
    }
    for (auto& w : workers) w.join();
}

static LogRecord sample_record(const char* text = "x") {
    LogRecord rec{};
    rec.level = LogLevel::INFO;
    rec.category = LogCategory::APP;
    std::snprintf(rec.text, LOG_TEXT_CAPACITY, "%s", text);
    return rec;
}

static bool file_nonempty(const std::string& path) {
    std::error_code ec;
    auto sz = fs::file_size(path, ec);
    return !ec && sz > 0;
}

bool test_basic_log() {
    fs::remove_all("test_logs_basic");
    log_init("test_logs_basic", LogLevel::DEBUG, QueueKind::AdaptiveBlocking);
    log_app(LogLevel::INFO, "hello basic %d", 1);
    log_operation(LogLevel::INFO, "op basic %d", 2);
    log_error("err basic %d", 3);
    log_close();
    return file_nonempty("test_logs_basic/application/application_001.log.enc") &&
           file_nonempty("test_logs_basic/operation/operation_001.log.enc") &&
           file_nonempty("test_logs_basic/error/error_001.log.enc");
}

bool test_level_filter() {
    fs::remove_all("test_logs_filter");
    log_init("test_logs_filter", LogLevel::WARN, QueueKind::AdaptiveBlocking);
    uint64_t before = global_logger().metrics().enqueued();
    log_app(LogLevel::INFO, "should be filtered");
    log_app(LogLevel::DEBUG, "should be filtered");
    uint64_t mid = global_logger().metrics().enqueued();
    log_app(LogLevel::ERROR, "should pass");
    uint64_t after = global_logger().metrics().enqueued();
    log_close();
    return (mid == before) && (after == before + 1);
}

bool test_file_rotate() {
    fs::remove_all("test_logs_rotate");
    log_init("test_logs_rotate", LogLevel::DEBUG, QueueKind::AdaptiveBlocking);
    int total = (int)MAX_LINES_PER_FILE + 50;
    for (int i = 0; i < total; ++i) {
        log_app(LogLevel::INFO, "rotate line %d", i);
    }
    log_close();
    return fs::exists("test_logs_rotate/application/application_002.log.enc");
}

bool test_queue_full() {
    MutexRingQueue q(4);
    LogRecord rec = sample_record();
    int ok = 0, dropped = 0;
    for (int i = 0; i < 6; ++i) {
        if (q.try_push(rec)) ++ok;
        else ++dropped;
    }
    return ok == 4 && dropped == 2;
}

bool test_queue_grow() {
    AdaptiveBlockingQueue q(16, 1024);
    LogRecord rec = sample_record();
    for (int i = 0; i < 20; ++i) {
        if (!q.try_push(rec)) return false;
    }
    return q.capacity() > 16 && q.max_capacity() == 1024 && q.approx_size() == 20;
}

bool test_backpressure() {
    AdaptiveBlockingQueue q(2, 4);
    std::atomic<bool> running{true};
    LogRecord rec = sample_record();
    QueuePushStats stats;

    for (int i = 0; i < 4; ++i) {
        if (!q.push_wait(rec, running, nullptr)) return false;
    }

    std::atomic<bool> done{false};
    bool push_ok = false;
    std::thread producer([&] {
        push_ok = q.push_wait(rec, running, &stats);
        done.store(true, std::memory_order_release);
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    bool was_blocked = !done.load(std::memory_order_acquire);

    LogRecord out[1];
    std::size_t popped = q.pop_batch(out, 1);
    producer.join();
    running.store(false);

    return was_blocked && popped == 1 && push_ok && stats.blocked &&
           stats.blocked_ns > 0 && q.approx_size() == 4;
}

bool test_adaptive_no_drop() {
    fs::remove_all("test_logs_adaptive_no_drop");
    Logger logger;
    Logger::Config cfg;
    cfg.dir = "test_logs_adaptive_no_drop";
    cfg.queue = QueueKind::AdaptiveBlocking;
    cfg.queue_capacity = 8;
    cfg.max_queue_capacity = 256;
    logger.init(cfg);

    constexpr int threads = 4;
    constexpr int per_thread = 200;
    std::vector<std::thread> workers;
    for (int t = 0; t < threads; ++t) {
        workers.emplace_back([&logger, t] {
            for (int i = 0; i < per_thread; ++i) {
                logger.app(LogLevel::INFO, "adaptive t%d line %d", t, i);
            }
        });
    }
    for (auto& w : workers) w.join();
    logger.close();

    auto s = logger.metrics().compute();
    uint64_t expected = (uint64_t)threads * (uint64_t)per_thread;
    return s.enqueued == expected && s.written == expected && s.dropped == 0 &&
           s.rejected_after_close == 0;
}

bool test_dynamic_batch() {
    return Logger::dynamic_batch_size(0) == 256 &&
           Logger::dynamic_batch_size(4095) == 256 &&
           Logger::dynamic_batch_size(4096) == 512 &&
           Logger::dynamic_batch_size(32768) == 1024 &&
           Logger::dynamic_batch_size(131072) == 2048;
}

bool test_decrypt_one_line() {
    fs::remove_all("test_logs_decrypt");
    log_init("test_logs_decrypt", LogLevel::DEBUG, QueueKind::AdaptiveBlocking);
    log_app(LogLevel::INFO, "decrypt marker ABC123");
    log_close();
    std::ifstream in("test_logs_decrypt/application/application_001.log.enc");
    std::string hexline;
    std::getline(in, hexline);
    std::string plain = xor_decrypt_from_hex(hexline);
    return plain.find("decrypt marker ABC123") != std::string::npos;
}

bool run_all_tests() {
    struct Case {
        const char* name;
        bool (*fn)();
    };

    Case cases[] = {
        {"test_basic_log",        test_basic_log},
        {"test_level_filter",     test_level_filter},
        {"test_file_rotate",      test_file_rotate},
        {"test_queue_full",       test_queue_full},
        {"test_queue_grow",       test_queue_grow},
        {"test_backpressure",     test_backpressure},
        {"test_adaptive_no_drop", test_adaptive_no_drop},
        {"test_dynamic_batch",    test_dynamic_batch},
        {"test_decrypt_one_line", test_decrypt_one_line},
    };

    bool all = true;
    std::printf("\n==== Unit Tests ====\n");
    for (auto& c : cases) {
        bool pass = c.fn();
        all = all && pass;
        std::printf("  [%s] %s\n", pass ? "PASS" : "FAIL", c.name);
    }
    std::printf("==== %s ====\n", all ? "ALL PASS" : "FAILED");
    return all;
}
