// test.cpp

#include "logger.h"

int main()
{
    Logger logger;

    Logger::Config cfg;          // ① init 接收的是 Config 结构体，不是字符串
    cfg.dir = "./logs";
    logger.init(cfg);

    logger.add(LogLevel::INFO, "hello");        // ② enum class 必须写 LogLevel:: 前缀
    logger.add(LogLevel::ERROR, "id=%d", 100);  // ③ add 支持 printf 风格格式化参数

    logger.close();
    return 0;
}