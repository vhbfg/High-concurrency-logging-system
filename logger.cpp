#include "logger.h"
#include "log_queue_lockfree.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <vector>

using clock_type = std::chrono::steady_clock;

static std::size_t configured_or_default(std::size_t configured,
                                         std::size_t default_value) {
    return configured ? configured : default_value;
}

bool Logger::init(const Config& cfg) {
    if (initialized_) return false;

    min_level_ = cfg.min_level;
    metrics_.reset();

    switch (cfg.queue) {
        case QueueKind::LockFree:
            queue_ = std::make_unique<LockFreeRingQueue>(
                configured_or_default(cfg.queue_capacity, LOG_QUEUE_CAPACITY));
            break;
        case QueueKind::Mutex:
            queue_ = std::make_unique<MutexRingQueue>(
                configured_or_default(cfg.queue_capacity, LOG_QUEUE_CAPACITY));
            break;
        case QueueKind::AdaptiveBlocking: {
            std::size_t initial = configured_or_default(
                cfg.queue_capacity, ADAPTIVE_QUEUE_INITIAL_CAPACITY);
            std::size_t max_cap = configured_or_default(
                cfg.max_queue_capacity, ADAPTIVE_QUEUE_MAX_CAPACITY);
            queue_ = std::make_unique<AdaptiveBlockingQueue>(initial, max_cap);
            break;
        }
    }

    metrics_.record_queue_state(0, queue_->capacity());

    writer_ = std::make_unique<LogWriter>(cfg.dir);
    writer_->init();

    running_.store(true, std::memory_order_release);
    initialized_ = true;
    thread_ = std::thread(&Logger::thread_func, this);
    return true;
}

void Logger::close() {
    if (!initialized_) return;

    running_.store(false, std::memory_order_release);
    if (queue_) queue_->notify_all();
    wake_cv_.notify_all();

    if (thread_.joinable()) thread_.join();
    writer_->close_all();
    queue_.reset();
    initialized_ = false;
}

void Logger::vwrite(LogLevel level, LogCategory cat, const char* fmt, va_list ap) {
    if (!initialized_) return;
    if ((int)level < (int)min_level_) return;

    LogRecord rec{};
    rec.level    = level;
    rec.category = cat;

    std::time_t now = std::time(nullptr);
    std::tm tmv;
#ifdef _WIN32
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif

    char ts[24];
    std::strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", &tmv);

    int off = std::snprintf(rec.text, LOG_TEXT_CAPACITY,
                            "%s [%s] ", ts, level_name(level));
    if (off < 0 || off >= (int)LOG_TEXT_CAPACITY) {
        off = (int)LOG_TEXT_CAPACITY - 1;
    }
    std::vsnprintf(rec.text + off, LOG_TEXT_CAPACITY - (std::size_t)off, fmt, ap);

    QueuePushStats push_stats;
    auto t0 = clock_type::now();
    bool ok = queue_->push_wait(rec, running_, &push_stats);
    auto t1 = clock_type::now();

    uint64_t ns = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
        t1 - t0).count();
    metrics_.record_latency(ns);
    metrics_.record_queue_state(push_stats.queue_size, push_stats.active_capacity);

    if (push_stats.blocked) {
        metrics_.record_blocked(push_stats.blocked_ns);
    }
    if (push_stats.max_capacity_hit) {
        metrics_.inc_max_capacity_hit();
    }

    if (ok) {
        metrics_.inc_enqueued();
        wake_cv_.notify_one();
    } else if (!running_.load(std::memory_order_acquire)) {
        metrics_.inc_rejected_after_close();
    } else {
        metrics_.inc_dropped();
    }
}

void Logger::app(LogLevel level, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vwrite(level, LogCategory::APP, fmt, ap);
    va_end(ap);
}

void Logger::operation(LogLevel level, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vwrite(level, LogCategory::OPERATION, fmt, ap);
    va_end(ap);
}

void Logger::error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vwrite(LogLevel::ERROR, LogCategory::ERROR, fmt, ap);
    va_end(ap);
}

std::size_t Logger::dynamic_batch_size(std::size_t queue_size) {
    if (queue_size < 4096) return BATCH_SIZE;
    if (queue_size < 32768) return 512 > BATCH_SIZE ? 512 : BATCH_SIZE;
    if (queue_size < 131072) return 1024 > BATCH_SIZE ? 1024 : BATCH_SIZE;
    return MAX_BATCH_SIZE;
}

void Logger::thread_func() {
    std::vector<LogRecord> batch(MAX_BATCH_SIZE);
    auto last_flush = clock_type::now();

    auto flush_periodically = [&] {
        auto now = clock_type::now();
        if (now - last_flush >= std::chrono::milliseconds(10)) {
            writer_->flush_all();
            last_flush = now;
        }
    };

    while (running_.load(std::memory_order_acquire)) {
        std::size_t max_batch = dynamic_batch_size(queue_->approx_size());
        std::size_t n = queue_->pop_batch(batch.data(), max_batch);
        if (n == 0) {
            writer_->flush_all();
            last_flush = clock_type::now();
            std::unique_lock<std::mutex> lk(wake_mtx_);
            wake_cv_.wait_for(lk, std::chrono::milliseconds(5));
            continue;
        }

        for (std::size_t i = 0; i < n; ++i) {
            writer_->write(batch[i]);
            metrics_.inc_written();
        }
        flush_periodically();
    }

    std::size_t n;
    while ((n = queue_->pop_batch(batch.data(), MAX_BATCH_SIZE)) > 0) {
        for (std::size_t i = 0; i < n; ++i) {
            writer_->write(batch[i]);
            metrics_.inc_written();
        }
    }
    writer_->flush_all();
}

Logger& global_logger() {
    static Logger inst;
    return inst;
}

bool log_init(const char* dir, LogLevel min_level, QueueKind kind) {
    Logger::Config cfg;
    cfg.dir       = dir ? dir : "logs";
    cfg.min_level = min_level;
    cfg.queue     = kind;
    return global_logger().init(cfg);
}

void log_app(LogLevel level, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    global_logger().vwrite(level, LogCategory::APP, fmt, ap);
    va_end(ap);
}

void log_operation(LogLevel level, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    global_logger().vwrite(level, LogCategory::OPERATION, fmt, ap);
    va_end(ap);
}

void log_error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    global_logger().vwrite(LogLevel::ERROR, LogCategory::ERROR, fmt, ap);
    va_end(ap);
}

void log_close() { global_logger().close(); }
uint64_t log_get_dropped_count() { return global_logger().dropped_count(); }
uint64_t log_get_blocked_count() { return global_logger().blocked_count(); }
uint64_t log_get_blocked_ns() { return global_logger().blocked_ns(); }
