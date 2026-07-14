#pragma once

#include <cstddef>
#include <cstdint>

enum class LogLevel : int {
    DEBUG = 0,
    INFO  = 1,
    WARN  = 2,
    ERROR = 3,
};

enum class LogCategory : int {
    APP       = 0,
    OPERATION = 1,
    ERROR     = 2,
};

inline constexpr std::size_t LOG_TEXT_CAPACITY  = 512;

// Fixed-size queues keep the original capacity so they remain useful baselines.
inline constexpr std::size_t LOG_QUEUE_CAPACITY = 8192;

// Adaptive blocking queue defaults.
inline constexpr std::size_t ADAPTIVE_QUEUE_INITIAL_CAPACITY = 131072;
inline constexpr std::size_t ADAPTIVE_QUEUE_MAX_CAPACITY     = 524288;
inline constexpr std::size_t ADAPTIVE_QUEUE_GROW_PERCENT     = 80;

// The writer adjusts the real batch size at runtime; BATCH_SIZE is the minimum.
inline constexpr std::size_t BATCH_SIZE     = 256;
inline constexpr std::size_t MAX_BATCH_SIZE = 2048;

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
