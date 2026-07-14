// test.cpp

#include "logger.h"

int main()
{
    Logger logger;

    Logger::Config cfg;          // ① init 接收的是 Config 结构体，不是字符串
    cfg.dir = "./logs";
    logger.init(cfg);

    logger.add(LogLevel::INFO, "hello");        // ② enum class 必须写 LogLevel:: 前缀
    logger.add(LogLevel::ERROR, "id=%d", 100);  // ③ add 改成 app（头文件修正后的名字）

    logger.close();
    return 0;
}