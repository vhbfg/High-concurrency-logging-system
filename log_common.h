#pragma once
#include <cstddef>
#include <cstdint>

// 日志等级
enum class LogLevel : int {
    DEBUG = 0,
    INFO  = 1,
    WARN  = 2,
    ERROR = 3,
};

// 日志类别
enum class LogCategory : int {
    APP       = 0,   // 应用日志
    OPERATION = 1,   // 操作日志
    ERROR     = 2,   // 错误日志
};

inline constexpr std::size_t   LOG_TEXT_CAPACITY  = 512;
inline constexpr std::size_t   LOG_QUEUE_CAPACITY = 131072;
inline constexpr std::size_t   BATCH_SIZE         = 64;
inline constexpr std::size_t   MAX_LINES_PER_FILE = 1000;
inline constexpr unsigned char LOG_XOR_KEY        = 0x5A;

struct LogRecord {
    LogLevel    level;
    LogCategory category;
    char        text[LOG_TEXT_CAPACITY];
};

inline const char* level_name(LogLevel lv) {
    switch (lv) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
    }
    return "UNKNOWN";
}

inline const char* category_name(LogCategory c) {
    switch (c) {
        case LogCategory::APP:       return "application";
        case LogCategory::OPERATION: return "operation";
        case LogCategory::ERROR:     return "error";
    }
    return "unknown";
}
