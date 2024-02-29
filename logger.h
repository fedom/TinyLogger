#ifndef BASE_LOGGER_H_
#define BASE_LOGGER_H_

#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#define HAVE_LOCALTIME_R

namespace tinylogger {

#define LOG(logger, LEVEL) \
  logger->Stream(tinylogger::LogLevel::LEVEL, __FILE__, __LINE__)

enum class LogLevel { DEBUG = 0, INFO = 1, WARNING = 2, ERROR = 3, FATAL = 4 };

class Logger {
 public:
  Logger(LogLevel log_level) : log_level_(log_level) {}
  virtual ~Logger() = default;

  virtual void WriteLog(const std::string msg, LogLevel level) = 0;

  LogLevel GetLogLevel() const { return log_level_; }

  class LogStream {
   public:
    LogStream(Logger *logger, LogLevel level, const char *file, int lineno)
        : logger_(logger),
          level_(level),
          file_(file),
          lineno_(lineno),
          tid_(std::this_thread::get_id()) {}

    ~LogStream() { logger_->Log(level_, *this); }

    LogStream(const LogStream &) = delete;
    LogStream &operator=(const LogStream &) = delete;

    LogStream(LogStream &&other) = default;
    LogStream &operator=(LogStream &&other) = default;

    template <typename T>
    LogStream &operator<<(T t) {
      oss_ << t;
      return *this;
    }

    std::string File() const { return file_; }
    int Lineno() const { return lineno_; }
    std::ostringstream &Stream() { return oss_; }
    std::thread::id Tid() const { return tid_; }

   private:
    std::ostringstream oss_;
    Logger *logger_;
    LogLevel level_;
    std::string file_;
    int lineno_;
    std::thread::id tid_;
  };

  LogStream Stream(LogLevel level, const char *file, int lineno) {
    return LogStream(this, level, file, lineno);
  }

 private:
  const char *GetLogLevelName(const LogLevel level) const {
    switch (level) {
      case LogLevel::DEBUG:
        return "DEBUG";
      case LogLevel::INFO:
        return "INFO";
      case LogLevel::WARNING:
        return "WARNING";
      case LogLevel::ERROR:
        return "ERROR";
      case LogLevel::FATAL:
        return "FATAL";
    }
  }

  void Log(LogLevel level, LogStream &ls) {
    if (level < log_level_) return;

    std::time_t current_time = time(nullptr);
    struct tm time_info;

#ifdef HAVE_LOCALTIME_R
    localtime_r(&current_time, &time_info);
#else  // For windows
    localtime_s(&time_info, &current_time);
#endif

    std::ostringstream format_message;

    format_message << "[" << GetLogLevelName(level)[0] << " "
                   << std::put_time(&time_info, "%Y-%m-%d %H:%M:%S ")
                   << ls.Tid() << " " << ls.File() << ":" << ls.Lineno() << "]";

    format_message << ls.Stream().str();

    {
      // For thread safety, we need to hold a lock to avoid multiple threads
      // interleaving on one line
      std::lock_guard<std::mutex> lock(mutex_);
      WriteLog(format_message.str(), level);
    }
  }

  LogLevel log_level_;
  std::mutex mutex_;
};

class StdoutLogger : public Logger {
 public:
  StdoutLogger(LogLevel level) : Logger(level) {}
  virtual ~StdoutLogger() {}

  virtual void WriteLog(const std::string msg, LogLevel level) {
    std::cout << GetLogLevelColorStr(level) << msg << ResetColorStr()
              << std::endl;
  }

 private:
  static const char *GetLogLevelColorStr(const LogLevel level) {
    switch (level) {
      case LogLevel::DEBUG:
        return "";
      case LogLevel::INFO:
        return "";
      case LogLevel::WARNING:
        return "\033[33m";  // Yellow
      case LogLevel::ERROR:
      case LogLevel::FATAL:
        return "\033[31;1m";  // Red
    }
  }

  static const char *ResetColorStr() { return "\033[0m"; }
};

class FileLogger : public Logger {
 public:
  constexpr static size_t default_size_limit = 1 * 1024 * 1024;  // 1M

  FileLogger(const std::string &file_name_prefix, LogLevel level,
             size_t size_limit = default_size_limit)
      : Logger(level),
        file_name_prefix_(file_name_prefix),
        ofs_(GenFileName(file_name_prefix), std::ios::app),
        size_limit_(size_limit),
        cur_size_(0) {
    if (!ofs_.good()) {
      throw std::runtime_error("Log file open failed");
    }
    srand(time(nullptr));
  }

  virtual ~FileLogger() { ofs_.close(); }

  // This function runs under lock protect
  virtual void WriteLog(const std::string msg, LogLevel level) {
    if (cur_size_ + msg.size() > size_limit_) {
      std::cout << "size_reach limit! cur_size_:" << cur_size_
                << ", msg size:" << msg.size() << ", size_limit:" << size_limit_
                << std::endl;
      ofs_.close();
      cur_size_ = 0;
      ofs_.open(GenFileName(file_name_prefix_), std::ios::app);
      if (!ofs_.good()) {
        throw std::runtime_error("Log file open failed");
      }
    }
    cur_size_ += msg.size();
    ofs_ << msg << std::endl;
  }

 private:
  std::string GenFileName(const std::string &name_prefix) {
    auto now = std::chrono::system_clock::now();

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch());

    std::time_t now_t = std::chrono::system_clock::to_time_t(now);

    int milli_part = dur.count() % 1000;
    struct tm time_info;

#ifdef HAVE_LOCALTIME_R
    localtime_r(&now_t, &time_info);
#else  // For windows
    localtime_s(&time_info, &now_t);
#endif

    std::ostringstream oss;

    // We assume file_size_limit won't be reached within 1 second
    oss << name_prefix << "_" << std::put_time(&time_info, "%Y%m%d_%H%M%S")
        << "." << std::setw(3) << std::setfill('0') << milli_part << "." << rand();
    return oss.str();
  }

  std::ofstream ofs_;
  std::string file_name_prefix_;
  size_t size_limit_;
  size_t cur_size_;
};

}  // namespace tinylogger

#endif  // BASE_LOGGER_H_