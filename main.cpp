#include "logger.h"

namespace
{
    void WriteLog(tinylogger::Logger *logger)
    {
        LOG(logger, DEBUG) << "This is DEBUG" << 5;
        LOG(logger, INFO) << "This is INFO" << 5;
        LOG(logger, WARNING) << "This is WARNING" << 5;
        LOG(logger, ERROR) << "This is ERROR" << 5;
        LOG(logger, FATAL) << "This is FATAL" << 5;
    }

    void WriteLogLoop(tinylogger::Logger *logger)
    {
        for (int i = 0; i < 100; i++)
        {
            WriteLog(logger);
        }
    }
}

void Test_StdoutLogger_singleThread()
{
    tinylogger::StdoutLogger logger(tinylogger::LogLevel::DEBUG);
    WriteLog(&logger);
}

void Test_StdoutLogger_MultiThread()
{
    tinylogger::StdoutLogger logger(tinylogger::LogLevel::DEBUG);

    std::thread threads[5];
    
    for (auto &thread : threads) {
        thread = std::thread(WriteLogLoop, &logger);
    }
    
    for (auto &thread : threads) {
        thread.join();
    }
}

void Test_FileLogger_SingleThread()
{
    tinylogger::FileLogger logger("log.txt", tinylogger::LogLevel::DEBUG);

    WriteLog(&logger);
}

void Test_FileLogger_MultiThread()
{
    tinylogger::FileLogger logger("log.txt", tinylogger::LogLevel::DEBUG, 10 * 1024);

    std::thread threads[30];
    
    int i = 0;
    for (auto &thread : threads) {
        thread = std::thread(WriteLogLoop, &logger);
        std::cout << ++i << "thread created" << std::endl;
    }
    
    for (auto &thread : threads) {
        thread.join();
    }
}

int main(int argc, char *argv[]) {
  Test_StdoutLogger_singleThread();
  Test_StdoutLogger_MultiThread();
  // Test_FileLogger_SingleThread();
  Test_FileLogger_MultiThread();
}