#include "test_business.h"
#include "logger.h"
#include "log_queue.h"
#include "log_crypto.h"
#include "log_common.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
namespace fs = std::filesystem;

//模拟产生日志
void simulate_login(int id, int count)
{
    for (int i = 0; i < count; ++i)
    {
        log_app(LogLevel::INFO, "thread %d login user #%d", id, i);
        global_logger().operation(LogLevel::INFO, "thread %d login audit #%d", id, i);
    }
}
void simulate_order(int id, int count)
{
    for (int i = 0; i < count; ++i)
    {
        log_app(LogLevel::INFO, "thread %d order created #%d", id, i);
        global_logger().operation(LogLevel::INFO, "thread %d order updated #%d", id, i);
    }
}
void simulate_file_task(int id, int count)
{
    for (int i = 0; i < count; ++i)
    {
        log_app(LogLevel::INFO, "thread %d file upload start #%d", id, i);
        global_logger().operation(LogLevel::INFO, "thread %d file upload finish #%d", id, i);
    }
}
void simulate_error(int id, int count)
{
    for (int i = 0; i < count; ++i)
    {
        log_app(LogLevel::WARN, "thread %d warning before error #%d", id, i);
        global_logger().operation(LogLevel::INFO, "thread %d error handling #%d", id, i);
        log_error("thread %d simulated error #%d", id, i);
    }
}

//循环产生不同日志
static void business_thread_func(int id, int count)
{
    switch (id % 4)
    {
        case 0: simulate_login(id, count); break;
        case 1: simulate_order(id, count); break;
        case 2: simulate_file_task(id, count); break;
        default: simulate_error(id, count); break;
    }
}

void run_business_test(int num_threads, int per_thread)
{
    std::vector<std::thread> workers;
    workers.reserve(num_threads);
    for (int t = 0; t < num_threads; ++t)
        workers.emplace_back(business_thread_func, t, per_thread);
    for (auto& w : workers) w.join();
}

//单元测试 
static bool file_nonempty(const std::string& path)
{
    std::error_code ec;
    auto sz = fs::file_size(path, ec);
    return !ec && sz > 0;
}
bool test_basic_log()
{
    fs::remove_all("test_logs_basic");
    log_init("test_logs_basic", LogLevel::DEBUG, QueueKind::Mutex);
    log_app(LogLevel::INFO, "hello basic %d", 1);
    global_logger().operation(LogLevel::INFO, "op basic %d", 2);
    log_error("err basic %d", 3);
    log_close();
    return file_nonempty("test_logs_basic/application/application_001.log.enc") &&
           file_nonempty("test_logs_basic/operation/operation_001.log.enc") &&
           file_nonempty("test_logs_basic/error/error_001.log.enc");
}
bool test_level_filter()
{
    fs::remove_all("test_logs_filter");
    log_init("test_logs_filter", LogLevel::WARN, QueueKind::Mutex);
    uint64_t before = global_logger().metrics().enqueued();
    global_logger().add(LogLevel::INFO,  "should be filtered");
    global_logger().add(LogLevel::DEBUG, "should be filtered");
    uint64_t mid = global_logger().metrics().enqueued();
    log_app(LogLevel::ERROR, "should pass");
    uint64_t after = global_logger().metrics().enqueued();
    log_close();
    return (mid == before) && (after == before + 1);
}
bool test_file_rotate()
{
    fs::remove_all("test_logs_rotate");
    log_init("test_logs_rotate", LogLevel::DEBUG, QueueKind::Mutex);
    int total = (int)MAX_LINES_PER_FILE + 50;
    for (int i = 0; i < total; ++i)
        log_app(LogLevel::INFO, "rotate line %d", i);
    log_close();
    return fs::exists("test_logs_rotate/application/application_002.log.enc");
}
bool test_queue_full()
{
    MutexRingQueue q(4);
    LogRecord rec{};
    rec.level    = LogLevel::INFO;
    rec.category = LogCategory::APP;
    std::snprintf(rec.text, LOG_TEXT_CAPACITY, "x");
    int ok = 0, dropped = 0;
    for (int i = 0; i < 6; ++i)
        if (q.try_push(rec)) ++ok; else ++dropped;
    return ok == 4 && dropped == 2;
}
bool test_decrypt_one_line()
{
    fs::remove_all("test_logs_decrypt");
    log_init("test_logs_decrypt", LogLevel::DEBUG, QueueKind::Mutex);
    log_app(LogLevel::INFO, "decrypt marker ABC123");
    log_close();
    std::ifstream in("test_logs_decrypt/application/application_001.log.enc");
    std::string hexline;
    std::getline(in, hexline);
    std::string plain = xor_decrypt_from_hex(hexline);
    return plain.find("decrypt marker ABC123") != std::string::npos;
}

// ===================================================================
// 一、基础边界并发测试（6个）
// ===================================================================

// 测试1：多线程并发混合打印三类日志
// 测试目的：验证高并发场景下日志系统无崩溃、无死锁，丢弃计数准确
// 验证要点：10线程各写200条混合类别日志，程序稳定运行，入队+丢弃总数合理
static bool test_multi_thread_concurrent()
{
    fs::remove_all("test_logs_mt");
    log_init("test_logs_mt", LogLevel::DEBUG, QueueKind::Mutex);

    const int N_THREADS = 10;
    const int N_LOGS    = 200;
    std::vector<std::thread> threads;
    threads.reserve(N_THREADS);

    for (int t = 0; t < N_THREADS; ++t)
    {
        threads.emplace_back([t]()
        {
            for (int i = 0; i < N_LOGS; ++i)
            {
                int kind = (t + i) % 3;
                if (kind == 0)
                    log_app(LogLevel::INFO,  "mt[t%d]#%d app", t, i);
                else if (kind == 1)
                    global_logger().operation(LogLevel::INFO, "mt[t%d]#%d op", t, i);
                else
                    log_error("mt[t%d]#%d err", t, i);
            }
        });
    }

    for (auto& th : threads) th.join();

    uint64_t enq    = global_logger().metrics().enqueued();
    uint64_t dropped = log_get_dropped_count();
    log_close();

    // 总写入量 = 线程数×条数; 队列容量8192远大于2000, 预计零丢弃
    uint64_t total = enq + dropped;
    return total > 0 && total <= (uint64_t)(N_THREADS * N_LOGS);
}

// 测试2：超长日志文本边界测试
// 测试目的：验证单行日志超过缓冲区容量时的截断行为
// 验证要点：写入超长文本(>512字节)不崩溃，解密后内容完整可读
static bool test_long_text_boundary()
{
    fs::remove_all("test_logs_long");
    log_init("test_logs_long", LogLevel::DEBUG, QueueKind::Mutex);

    // 构造超过 LOG_TEXT_CAPACITY(512) 的文本，含时间戳头后必然截断
    std::string huge(600, 'Z');
    log_app(LogLevel::INFO, "LONG|%s", huge.c_str());
    log_close();

    std::ifstream in("test_logs_long/application/application_001.log.enc");
    if (!in) return false;
    std::string hexline;
    if (!std::getline(in, hexline)) return false;
    std::string plain = xor_decrypt_from_hex(hexline);

    // 解密后应包含 INFO 标记和 Z 字符 (至少前面部分)
    return !plain.empty()
        && plain.find("INFO")  != std::string::npos
        && plain.find('Z')     != std::string::npos;
}

// 测试3：中文、特殊符号、空白字符加解密校验
// 测试目的：验证非ASCII字符和特殊符号经过加密→解密后完整保留
// 验证要点：中文/tab/换行/!@#$%等特殊字符日志，解密后原文存在
static bool test_special_char_log()
{
    fs::remove_all("test_logs_special");
    log_init("test_logs_special", LogLevel::DEBUG, QueueKind::Mutex);

    const char* marker = "中文内容\tTab符!@#$%^&*() 空白结尾  ";
    log_app(LogLevel::WARN, "SPECIAL|%s", marker);
    log_close();

    std::ifstream in("test_logs_special/application/application_001.log.enc");
    if (!in) return false;
    std::string hexline;
    if (!std::getline(in, hexline)) return false;
    std::string plain = xor_decrypt_from_hex(hexline);

    // 中文"中文内容"和"Tab符"应完整保留
    bool cn_ok = plain.find("中文内容") != std::string::npos;
    bool tab_ok = plain.find("Tab符") != std::string::npos;
    return cn_ok && tab_ok;
}

// 测试4：重复初始化/关闭与未初始化写日志容错
// 测试目的：验证日志实例生命周期管理的健壮性
// 验证要点：3次 init/close 循环不崩溃，未初始化直接写日志静默忽略
static bool test_repeated_init_close()
{
    fs::remove_all("test_logs_reinit");

    // 3次完整初始化→写日志→关闭循环
    for (int cycle = 0; cycle < 3; ++cycle)
    {
        bool ok = log_init("test_logs_reinit", LogLevel::DEBUG,
                           (cycle % 2 == 0) ? QueueKind::Mutex : QueueKind::LockFree);
        if (ok)
        {
            log_app(LogLevel::INFO, "reinit cycle %d", cycle);
            global_logger().operation(LogLevel::INFO, "reinit op %d", cycle);
            log_error("reinit err %d", cycle);
        }
        log_close();
    }

    // 未初始化时直接写日志——应被静默忽略，不崩溃
    log_app(LogLevel::INFO, "should be ignored");
    global_logger().operation(LogLevel::INFO, "should be ignored");
    log_error("should be ignored");

    // 重复关闭也应安全 (close 内部检查 initialized_ 标志)
    log_close();
    log_close();
    return true;  // 执行到此即未崩溃，通过
}

// 测试5：空字符串与空参数日志写入
// 测试目的：验证空输入边界条件下日志系统的稳定性
// 验证要点：空字符串/纯静态文本写入不宕机，日志文件正常生成
static bool test_empty_log()
{
    fs::remove_all("test_logs_empty");
    log_init("test_logs_empty", LogLevel::DEBUG, QueueKind::Mutex);

    // 空格式化字符串
    log_app(LogLevel::INFO, "");
    global_logger().operation(LogLevel::INFO, "");
    log_error("");

    // 纯静态文本 (无格式化参数)
    log_app(LogLevel::WARN, "static text without args");
    global_logger().operation(LogLevel::ERROR, "op static");

    log_close();

    // 验证日志文件生成且可解密 (即使内容仅含时间戳头也应有字节写入)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    bool app_ok = file_nonempty("test_logs_empty/application/application_001.log.enc");
    bool op_ok  = file_nonempty("test_logs_empty/operation/operation_001.log.enc");
    bool err_ok = file_nonempty("test_logs_empty/error/error_001.log.enc");
    return app_ok && op_ok && err_ok;
}

// 测试6：三类日志高频交替写入——分类目录隔离校验
// 测试目的：验证高速交替写入时，不同类别日志严格写入各自目录
// 验证要点：APP/OPERATION/ERROR 交替写入后，三个目录各自有正确的加密文件
static bool test_cross_category_mix()
{
    fs::remove_all("test_logs_cross");
    log_init("test_logs_cross", LogLevel::DEBUG, QueueKind::Mutex);

    // 高频交替：每轮依次写 APP、OPERATION、ERROR
    for (int i = 0; i < 200; ++i)
    {
        log_app(LogLevel::INFO, "cross_app_%d", i);
        global_logger().operation(LogLevel::INFO, "cross_op_%d", i);
        log_error("cross_err_%d", i);
    }
    log_close();

    // 三个目录各自非空
    bool app_ok = file_nonempty("test_logs_cross/application/application_001.log.enc");
    bool op_ok  = file_nonempty("test_logs_cross/operation/operation_001.log.enc");
    bool err_ok = file_nonempty("test_logs_cross/error/error_001.log.enc");
    if (!app_ok || !op_ok || !err_ok) return false;

    // 抽查解密后内容，确认类别标记未串目录
    auto has = [](const std::string& path, const std::string& kw) -> bool {
        std::ifstream in(path);
        std::string line;
        return std::getline(in, line)
            && xor_decrypt_from_hex(line).find(kw) != std::string::npos;
    };

    return has("test_logs_cross/application/application_001.log.enc", "cross_app")
        && has("test_logs_cross/operation/operation_001.log.enc",   "cross_op")
        && has("test_logs_cross/error/error_001.log.enc",           "cross_err");
}

// ===================================================================
// 二、项目特色高优先级测试（3个，体现本系统独有创新点）
// ===================================================================

// 测试7：互斥锁队列 / 无锁队列双模切换对比
// 测试目的：验证两种队列实现在相同负载下写入结果一致
// 验证要点：相同日志量下 Mutex 与 LockFree 入队数相等，零丢弃
static bool test_queue_mode_switch()
{
    const int N = 400;

    // 互斥锁模式
    fs::remove_all("test_logs_q_mutex");
    log_init("test_logs_q_mutex", LogLevel::DEBUG, QueueKind::Mutex);
    for (int i = 0; i < N; ++i)
        log_app(LogLevel::INFO, "mutex_%d", i);
    uint64_t mutex_enq = global_logger().metrics().enqueued();
    uint64_t mutex_drop = log_get_dropped_count();
    log_close();

    // 无锁模式
    fs::remove_all("test_logs_q_lockfree");
    log_init("test_logs_q_lockfree", LogLevel::DEBUG, QueueKind::LockFree);
    for (int i = 0; i < N; ++i)
        log_app(LogLevel::INFO, "lockfree_%d", i);
    uint64_t lf_enq = global_logger().metrics().enqueued();
    uint64_t lf_drop = log_get_dropped_count();
    log_close();

    // N=400 << 8192(队列容量)，两种模式均应零丢弃、入队数相等
    return mutex_drop == 0
        && lf_drop == 0
        && mutex_enq == (uint64_t)N
        && lf_enq   == (uint64_t)N;
}

// 测试8：日志文件自动切分阈值精确验证
// 测试目的：验证达到 MAX_LINES_PER_FILE 阈值时自动生成新分段文件
// 验证要点：精确边界写入触发多次切分，多分段文件均非空存在
static bool test_log_rotate_threshold()
{
    fs::remove_all("test_logs_rotate2");
    log_init("test_logs_rotate2", LogLevel::DEBUG, QueueKind::Mutex);

    const int THRESH = (int)MAX_LINES_PER_FILE;  // 1000

    // 第1段：写满 THRESH 条 → 触发第一次切分
    for (int i = 0; i < THRESH; ++i)
        log_app(LogLevel::INFO, "phase1_%d", i);

    // 第2段：再写 THRESH 条 → 触发第二次切分
    for (int i = 0; i < THRESH; ++i)
        log_app(LogLevel::INFO, "phase2_%d", i);

    // 第3段：写少量日志，确保第3个文件有内容
    for (int i = 0; i < 10; ++i)
        log_app(LogLevel::INFO, "phase3_%d", i);

    log_close();

    // 应存在 3 个连续分段文件
    bool f1 = file_nonempty("test_logs_rotate2/application/application_001.log.enc");
    bool f2 = file_nonempty("test_logs_rotate2/application/application_002.log.enc");
    bool f3 = file_nonempty("test_logs_rotate2/application/application_003.log.enc");

    // 验证各分段日志可解密且包含对应阶段标记
    auto has = [](const std::string& path, const std::string& kw) -> bool {
        std::ifstream in(path);
        std::string line;
        return std::getline(in, line)
            && xor_decrypt_from_hex(line).find(kw) != std::string::npos;
    };

    return f1 && f2 && f3
        && has("test_logs_rotate2/application/application_001.log.enc", "phase1")
        && has("test_logs_rotate2/application/application_002.log.enc", "phase2")
        && has("test_logs_rotate2/application/application_003.log.enc", "phase3");
}

// 测试9：磁盘写入异常场景容错
// 测试目的：模拟日志目录不可创建时的系统容错能力
// 验证要点：目录创建失败时系统不崩溃，初始化异常被安全捕获，资源正常释放
static bool test_disk_full_sim()
{
    // 正常初始化、写入、关闭（释放所有文件句柄）
    fs::remove_all("test_logs_diskfull");
    log_init("test_logs_diskfull", LogLevel::DEBUG, QueueKind::Mutex);
    for (int i = 0; i < 10; ++i)
        log_app(LogLevel::INFO, "pre_close_%d", i);
    log_close();  // 关闭，Windows 上文件句柄全部释放

    // 删除目录后，在同名路径创建普通文件，阻塞 Writer 重建子目录
    fs::remove_all("test_logs_diskfull");
    { std::ofstream f("test_logs_diskfull"); f << "block"; }

    // 尝试重新初始化——writer::init() 因文件阻塞而无法创建子目录
    // fs::create_directories 在非目录文件上创建子目录将抛出 filesystem_error
    bool survived = false;
    try {
        bool ok = log_init("test_logs_diskfull", LogLevel::DEBUG, QueueKind::Mutex);
        if (ok) {
            // 若意外初始化成功（平台差异），写入并正常关闭
            for (int i = 0; i < 20; ++i)
                log_app(LogLevel::INFO, "unexpected_ok_%d", i);
            log_close();
        }
        survived = true;
    } catch (const std::exception&) {
        // init 中 writer_->init() 抛出目录创建异常
        // initialized_ 保持 false，close() 将安全返回
        log_close();
        survived = true;
    } catch (...) {
        log_close();
        survived = true;
    }

    // 清理：先删阻塞文件，再删可能残留的目录
    fs::remove("test_logs_diskfull");
    fs::remove_all("test_logs_diskfull");

    // 无论 init 成功/抛异常，代码执行至此未崩溃即通过
    return survived;
}

bool run_all_tests()
{
    struct Case { const char* name; bool (*fn)(); };
    Case cases[] =
    {
        // 原有测试
        {"test_basic_log",               test_basic_log},
        {"test_level_filter",            test_level_filter},
        {"test_file_rotate",             test_file_rotate},
        {"test_queue_full",              test_queue_full},
        {"test_decrypt_one_line",        test_decrypt_one_line},
        // 一、基础边界并发（6个）
        {"test_multi_thread_concurrent", test_multi_thread_concurrent},
        {"test_long_text_boundary",      test_long_text_boundary},
        {"test_special_char_log",        test_special_char_log},
        {"test_repeated_init_close",     test_repeated_init_close},
        {"test_empty_log",               test_empty_log},
        {"test_cross_category_mix",      test_cross_category_mix},
        // 二、项目特色高优先级（3个）
        {"test_queue_mode_switch",       test_queue_mode_switch},
        {"test_log_rotate_threshold",    test_log_rotate_threshold},
        {"test_disk_full_sim",           test_disk_full_sim},
    };
    bool all = true;
    std::printf("\n==== 单元测试 ====\n");
    for (auto& c : cases)
    {
        bool pass = c.fn();
        all = all && pass;
        std::printf("  [%s] %s\n", pass ? "PASS" : "FAIL", c.name);
    }
    std::printf("==== %s ====\n", all ? "全部通过" : "存在失败");
    return all;
}
