#pragma once

#include "log_common.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <vector>

struct QueuePushStats {
    bool        blocked = false;
    uint64_t    blocked_ns = 0;
    bool        max_capacity_hit = false;
    std::size_t queue_size = 0;
    std::size_t active_capacity = 0;
};

class IQueue {
public:
    virtual ~IQueue() = default;

    // Non-blocking push. Fixed-size baseline queues return false when full.
    virtual bool try_push(const LogRecord& rec) = 0;

    // Blocking push used by AdaptiveBlockingQueue. Baseline queues keep the old
    // try-once behavior through this default implementation.
    virtual bool push_wait(const LogRecord& rec,
                           std::atomic<bool>& running,
                           QueuePushStats* stats = nullptr) {
        if (!running.load(std::memory_order_acquire)) {
            if (stats) {
                stats->queue_size = approx_size();
                stats->active_capacity = capacity();
            }
            return false;
        }

        bool ok = try_push(rec);
        if (stats) {
            stats->queue_size = approx_size();
            stats->active_capacity = capacity();
            stats->max_capacity_hit = !ok && capacity() >= max_capacity();
        }
        return ok;
    }

    virtual std::size_t pop_batch(LogRecord* out, std::size_t max) = 0;
    virtual std::size_t approx_size() const = 0;
    virtual std::size_t capacity() const = 0;
    virtual std::size_t max_capacity() const { return capacity(); }
    virtual const char* name() const = 0;
    virtual void notify_all() {}
};

class MutexRingQueue : public IQueue {
public:
    explicit MutexRingQueue(std::size_t cap = LOG_QUEUE_CAPACITY)
        : buf_(cap ? cap : 1), cap_(cap ? cap : 1) {}

    bool try_push(const LogRecord& rec) override {
        std::lock_guard<std::mutex> lk(mtx_);
        if (count_ == cap_) return false;
        buf_[rear_] = rec;
        rear_ = (rear_ + 1) % cap_;
        ++count_;
        return true;
    }

    std::size_t pop_batch(LogRecord* out, std::size_t max) override {
        std::lock_guard<std::mutex> lk(mtx_);
        std::size_t n = 0;
        while (n < max && count_ > 0) {
            out[n++] = buf_[front_];
            front_ = (front_ + 1) % cap_;
            --count_;
        }
        return n;
    }

    std::size_t approx_size() const override {
        std::lock_guard<std::mutex> lk(mtx_);
        return count_;
    }

    std::size_t capacity() const override { return cap_; }
    const char* name() const override { return "mutex"; }

private:
    std::vector<LogRecord> buf_;
    std::size_t            cap_;
    std::size_t            front_ = 0, rear_ = 0, count_ = 0;
    mutable std::mutex     mtx_;
};

class AdaptiveBlockingQueue : public IQueue {
public:
    explicit AdaptiveBlockingQueue(
        std::size_t initial_capacity = ADAPTIVE_QUEUE_INITIAL_CAPACITY,
        std::size_t max_capacity = ADAPTIVE_QUEUE_MAX_CAPACITY)
        : active_capacity_(initial_capacity ? initial_capacity : 1),
          max_capacity_(max_capacity ? max_capacity : active_capacity_) {
        if (max_capacity_ < active_capacity_) max_capacity_ = active_capacity_;
    }

    bool try_push(const LogRecord& rec) override {
        std::lock_guard<std::mutex> lk(mtx_);
        grow_if_needed_unlocked();
        if (queue_.size() >= active_capacity_) return false;
        queue_.push_back(rec);
        return true;
    }

    bool push_wait(const LogRecord& rec,
                   std::atomic<bool>& running,
                   QueuePushStats* stats = nullptr) override {
        using clock = std::chrono::steady_clock;

        bool blocked = false;
        bool max_hit = false;
        clock::time_point block_start;

        std::unique_lock<std::mutex> lk(mtx_);
        if (!running.load(std::memory_order_acquire)) {
            fill_stats_unlocked(stats, blocked, 0, max_hit);
            return false;
        }

        while (queue_.size() >= active_capacity_) {
            grow_if_needed_unlocked();
            if (queue_.size() < active_capacity_) break;

            max_hit = true;
            if (!blocked) {
                blocked = true;
                block_start = clock::now();
            }

            not_full_cv_.wait(lk, [&] {
                return !running.load(std::memory_order_acquire) ||
                       queue_.size() < active_capacity_;
            });

            if (!running.load(std::memory_order_acquire)) {
                uint64_t blocked_ns = blocked
                    ? (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
                          clock::now() - block_start).count()
                    : 0;
                fill_stats_unlocked(stats, blocked, blocked_ns, max_hit);
                return false;
            }
        }

        queue_.push_back(rec);
        uint64_t blocked_ns = blocked
            ? (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
                  clock::now() - block_start).count()
            : 0;
        fill_stats_unlocked(stats, blocked, blocked_ns, max_hit);
        return true;
    }

    std::size_t pop_batch(LogRecord* out, std::size_t max) override {
        std::unique_lock<std::mutex> lk(mtx_);
        std::size_t n = 0;
        while (n < max && !queue_.empty()) {
            out[n++] = queue_.front();
            queue_.pop_front();
        }
        lk.unlock();

        if (n > 0) not_full_cv_.notify_all();
        return n;
    }

    std::size_t approx_size() const override {
        std::lock_guard<std::mutex> lk(mtx_);
        return queue_.size();
    }

    std::size_t capacity() const override {
        std::lock_guard<std::mutex> lk(mtx_);
        return active_capacity_;
    }

    std::size_t max_capacity() const override { return max_capacity_; }
    const char* name() const override { return "adaptive"; }

    void notify_all() override {
        not_full_cv_.notify_all();
    }

private:
    void grow_if_needed_unlocked() {
        if (active_capacity_ >= max_capacity_) return;
        if (queue_.size() * 100 < active_capacity_ * ADAPTIVE_QUEUE_GROW_PERCENT) {
            return;
        }

        std::size_t next = active_capacity_ * 2;
        if (next < active_capacity_) next = max_capacity_;
        if (next > max_capacity_) next = max_capacity_;
        active_capacity_ = next;
    }

    void fill_stats_unlocked(QueuePushStats* stats,
                             bool blocked,
                             uint64_t blocked_ns,
                             bool max_hit) const {
        if (!stats) return;
        stats->blocked = blocked;
        stats->blocked_ns = blocked_ns;
        stats->max_capacity_hit = max_hit;
        stats->queue_size = queue_.size();
        stats->active_capacity = active_capacity_;
    }

    std::deque<LogRecord>      queue_;
    std::size_t                active_capacity_;
    std::size_t                max_capacity_;
    mutable std::mutex         mtx_;
    std::condition_variable    not_full_cv_;
};
