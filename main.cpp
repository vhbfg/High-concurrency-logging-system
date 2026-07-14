#include "logger.h"
#include "test_business.h"
#include "benchmark.h"
#include "log_crypto.h"

#include <clocale>
#include <cstdio>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

static void init_console_utf8() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::setlocale(LC_ALL, ".UTF8");
}

static void demo_decrypt_preview() {
    std::ifstream in("logs/application/application_001.log.enc");
    if (!in) return;

    std::string line;
    if (std::getline(in, line)) {
        std::printf("\n[decrypt preview] encrypted: %.40s...\n", line.c_str());
        std::printf("[decrypt preview] plain:     %s\n",
                    xor_decrypt_from_hex(line).c_str());
    }
}

int main(int argc, char** argv) {
    init_console_utf8();

    std::string mode = (argc > 1) ? argv[1] : "run";

    if (mode == "bench") {
        bench_compare({1, 4, 8, 16}, 20000);
        return 0;
    }
    if (mode == "test") {
        return run_all_tests() ? 0 : 1;
    }

    std::printf("==== High-concurrency async logging system ====\n");
    log_init("logs", LogLevel::DEBUG, QueueKind::AdaptiveBlocking);

    std::printf("Starting 5 business threads, 1000 iterations each...\n");
    run_business_test(5, 1000);

    std::printf("dropped=%llu blocked_count=%llu blocked_ms=%.3f\n",
                (unsigned long long)log_get_dropped_count(),
                (unsigned long long)log_get_blocked_count(),
                (double)log_get_blocked_ns() / 1000000.0);

    log_close();
    std::printf("logger closed; queued logs have been drained.\n");

    demo_decrypt_preview();
    run_all_tests();
    return 0;
}
