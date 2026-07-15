# 高并发异步日志系统（C++ 实现）

一个本地 C++ 异步日志库：业务线程写日志请求 → 进入异步队列 → 后台线程批量出队、分类、XOR 加密、写入 `.log.enc` 文件，并支持关闭收尾与丢弃统计。

提供 **三套队列实现**：`MutexRingQueue`（加锁循环队列）、`LockFreeRingQueue`（无锁 MPMC 队列）、`AdaptiveBlockingQueue`（自适应扩容 + 背压阻塞队列），配套压测框架量化三者在不同并发度下的吞吐量与延迟差异。

## 核心功能

- **多业务线程**模拟真实业务场景（登录、下单、文件上传、异常处理），产生日志请求。
- **后台消费线程**批量出队日志，按类别（application / operation / error）写入不同文件。
- **等级过滤与格式化**在入队前完成，降低队列锁竞争。
- **XOR 加密**：日志以十六进制密文写入 `.log.enc` 文件，支持解密还原验证。
- **文件自动切分**：单文件超过 1000 行自动轮转，生成 `*_001.log.enc`、`*_002.log.enc` …。
- **关闭收尾（graceful shutdown）**：停止接收新日志后，将队列中剩余日志全部刷盘再关闭。
- **三种队列实现**：
  | 队列 | 特点 |
  | --- | --- |
  | `MutexRingQueue` | 固定容量 + `std::mutex`，简单可靠的基线 |
  | `LockFreeRingQueue` | 固定容量 + CAS 无锁（Vyukov bounded MPMC），极致入队性能 |
  | `AdaptiveBlockingQueue` | 动态扩容（初始 128K → 最大 512K），满时阻塞生产者而非丢弃 |
- **压测框架**：统一负载对比三套队列，输出吞吐、延迟（P50/P90/P99）、丢弃率、阻塞时长等指标。
- **9 个单元测试**：覆盖基础写日志、等级过滤、文件切分、队列满丢弃、队列扩容、背压阻塞、自适应零丢弃、动态批处理、密文解密。

## 目录结构

| 文件 | 说明 |
| --- | --- |
| `log_common.h` | 公共类型（LogLevel / LogCategory / LogRecord）与常量 |
| `log_queue.h` | 队列抽象接口 `IQueue` + `MutexRingQueue` + `AdaptiveBlockingQueue` |
| `log_queue_lockfree.h` | 无锁队列 `LockFreeRingQueue`（Vyukov bounded MPMC 算法） |
| `log_crypto.h` / `log_crypto.cpp` | XOR 加密 / 十六进制编解码 |
| `metrics.h` / `metrics.cpp` | 原子计数器 + 延迟直方图，计算 P50/P90/P99 |
| `log_writer.h` / `log_writer.cpp` | 后台写盘：按类别分文件、加密、按行数切分 |
| `logger.h` / `logger.cpp` | 日志系统核心 + 全局便捷接口 |
| `test_business.h` / `test_business.cpp` | 模拟业务线程 + 9 个单元测试 |
| `benchmark.h` / `benchmark.cpp` | 压测框架，驱动三套队列跑相同负载并对比 |
| `main.cpp` | 程序入口：`run` / `test` / `bench` 三种模式 |
| `Makefile` | 编译与运行命令 |
| `optimization_plan.md` | 性能优化方案文档 |

## 日志输出位置

| 模式 | 输出目录 | 说明 |
| --- | --- | --- |
| `make run` | `logs/application/` `logs/operation/` `logs/error/` | 基础演示日志 |
| `make test` | `test_logs_basic/` `test_logs_filter/` `test_logs_rotate/` … | 单元测试日志 |
| `make bench` | `bench_logs/` | 压测对比日志 |

> 所有日志文件均以 `.log.enc` 扩展名保存为 XOR 加密的十六进制文本。

## 快速开始

```bash
make            # 编译，产物为 bin/logsys
make run        # 基础功能演示（5 线程 × 1000 条）+ 单元测试
make test       # 只跑单元测试
make bench      # 三套队列（Mutex / LockFree / AdaptiveBlocking）压测对比
make clean      # 清理所有编译产物、日志文件和可执行文件
```

## 编译命令

```bash
make
```

编译产物输出到 `bin/logsys`（Windows 上为 `bin/logsys.exe`）。

编译器要求：**GCC 8+** 或 **Clang 7+**（需要 C++17 与 `<filesystem>` 支持）。

## 运行命令

```bash
make run
```

输出示例：

```
==== High-concurrency async logging system ====
Starting 5 business threads, 1000 iterations each...
dropped=0 blocked_count=0 blocked_ms=0.000
logger closed; queued logs have been drained.

[decrypt preview] encrypted: 5A3F2C1B...（XOR 密文）
[decrypt preview] plain:     2025-07-14 10:30:01 [INFO] thread 0 login user #0

==== Unit Tests ====
  [PASS] test_basic_log
  [PASS] test_level_filter
  [PASS] test_file_rotate
  [PASS] test_queue_full
  [PASS] test_queue_grow
  [PASS] test_backpressure
  [PASS] test_adaptive_no_drop
  [PASS] test_dynamic_batch
  [PASS] test_decrypt_one_line
==== ALL PASS ====
```

## 测试方法

```bash
make test
```

测试覆盖 9 个用例：基本写日志、等级过滤、文件切分、队列满丢弃、队列扩容、背压阻塞、自适应零丢弃、动态批处理大小、XOR 密文解密验证。

## 设计要点

- **模型**：多生产者单消费者（MPSC）——多业务线程生产 → 单后台线程消费。
- **锁的粒度**：`MutexRingQueue` 入队/出队持锁，但后台线程出队后立即释放，`fopen/fwrite/fflush` 等磁盘 I/O 不持有队列锁，避免阻塞生产者。
- **无锁对照**：`LockFreeRingQueue` 基于 Vyukov bounded MPMC 算法，每槽序号 + CAS 保证线程安全，容量取 2 的幂用位与取模。
- **自适应阻塞**：`AdaptiveBlockingQueue` 在负载超过 80% 容量时自动翻倍扩容，达到最大容量后阻塞生产者（而非丢弃），适合零丢失场景。
- **动态批处理**：后台消费者根据当前队列深度自适应调整每次出队的批次大小（256～2048），减少锁竞争次数。
- **等级过滤与格式化在入队前完成**：避免多线程争用同一缓冲区，且过滤掉的低等级日志根本不进入队列。
- **关闭收尾**：`log_close()` 设置停止标志 → 唤醒后台线程 → 排空队列剩余日志 → 关闭所有文件句柄。
- **加密**：仅在后台写盘线程调用 `xor_encrypt_to_hex()`，对生产者路径零开销；`xor_decrypt_from_hex()` 可还原验证。

## 压测结论（示例，机器不同数值会变）

无锁队列在各并发度下 **入队延迟与尾延迟（P99）明显更低、吞吐更高**；但由于瓶颈在单消费者磁盘写入，生产者更快反而会更快填满队列，**丢弃率略高**。

自适应阻塞队列在高负载下扩容并阻塞生产者，实现 **零丢弃**，但代价是尾延迟升高（被阻塞的生产者等待消费者腾出空间）。

这正说明高并发日志系统的真正瓶颈往往在消费端 I/O，而非入队路径。
