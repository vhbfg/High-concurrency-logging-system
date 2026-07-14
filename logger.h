#pragma once

#include "log_common.h"
#include "log_queue.h"
#include "log_writer.h"
#include "metrics.h"

#include <atomic>
#include <condition_variable>
#include <cstdarg>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

enum class QueueKind {
    Mutex,
    LockFree,
    AdaptiveBlocking,
};

class Logger {
public:
    struct Config {
        std::string dir       = "logs";
        LogLevel    min_level = LogLevel::DEBUG;
        QueueKind   queue     = QueueKind::AdaptiveBlocking;

        // For fixed queues this is the ring capacity. For AdaptiveBlocking it
        // is the initial capacity. A zero value means "use the default".
        std::size_t queue_capacity = 0;
        std::size_t max_queue_capacity = 0;
    };

    Logger() = default;
    ~Logger() { close(); }

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;

    bool init(const Config& cfg);
    void close();

    void app(LogLevel level, const char* fmt, ...);
    void operation(LogLevel level, const char* fmt, ...);
    void error(const char* fmt, ...);
    void vwrite(LogLevel level, LogCategory cat, const char* fmt, va_list ap);

    uint64_t dropped_count() const { return metrics_.dropped(); }
    uint64_t blocked_count() const { return metrics_.blocked_count(); }
    uint64_t blocked_ns() const { return metrics_.blocked_ns(); }
    uint64_t rejected_after_close_count() const {
        return metrics_.rejected_after_close();
    }
    Metrics& metrics() { return metrics_; }

    static std::size_t dynamic_batch_size(std::size_t queue_size);

private:
    void thread_func();

    std::unique_ptr<IQueue>    queue_;
    std::unique_ptr<LogWriter> writer_;
    Metrics                    metrics_;
    LogLevel                   min_level_ = LogLevel::DEBUG;
    std::atomic<bool>          running_{false};
    bool                       initialized_ = false;
    std::thread                thread_;
    std::mutex                 wake_mtx_;
    std::condition_variable    wake_cv_;
};

bool     log_init(const char* dir, LogLevel min_level = LogLevel::DEBUG,
                  QueueKind kind = QueueKind::AdaptiveBlocking);
void     log_app(LogLevel level, const char* fmt, ...);
void     log_operation(LogLevel level, const char* fmt, ...);
void     log_error(const char* fmt, ...);
void     log_close();
uint64_t log_get_dropped_count();
uint64_t log_get_blocked_count();
uint64_t log_get_blocked_ns();
Logger&  global_logger();
