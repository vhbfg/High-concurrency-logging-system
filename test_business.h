#pragma once

void simulate_login(int id, int count);
void simulate_order(int id, int count);
void simulate_file_task(int id, int count);
void simulate_error(int id, int count);

void run_business_test(int num_threads = 5, int per_thread = 1000);

bool test_basic_log();
bool test_level_filter();
bool test_file_rotate();
bool test_queue_full();
bool test_queue_grow();
bool test_backpressure();
bool test_adaptive_no_drop();
bool test_dynamic_batch();
bool test_decrypt_one_line();

bool run_all_tests();
