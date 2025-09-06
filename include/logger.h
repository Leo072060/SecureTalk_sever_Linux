#ifndef LOGGER_H
#define LOGGER_H

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>

// Log level
enum class LogLevel
{
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL
};

// Log event
class LogEvent
{
  public:
    LogEvent(const char *file, int32_t line, std::chrono::steady_clock::time_point elapse, std::thread::id threadId,
             uint32_t fiberId, std::chrono::system_clock::time_point time, const std::string &content);

    ~LogEvent() = default;

  public:
    const char                           *m_file = nullptr;
    int32_t                               m_line = 0;
    std::chrono::steady_clock::time_point m_elapse;
    std::thread::id                       m_threadId;
    uint32_t                              m_fiberId = 0;
    std::chrono::system_clock::time_point m_time;
    std::string                           m_content;
};

// Log formatter
class LogFormatter
{
  public:
    LogFormatter(const std::string &pattern);
    ~LogFormatter() = default;

    std::string format(LogLevel level, const std::shared_ptr<LogEvent> &event) const;

  private:
    std::string m_pattern;
};

// Log appender
class LogAppender
{
  public:
    LogAppender(LogLevel            level     = LogLevel::DEBUG,
                const LogFormatter &formatter = LogFormatter("%d [%p] [%t] %f:%l %m%n"));
    virtual ~LogAppender() = default;

    void log(LogLevel level, const std::shared_ptr<LogEvent> &event) const;

  protected:
    virtual void logToDest(const std::string &log) const = 0;

  private:
    LogLevel     m_level;
    LogFormatter m_formatter;
};

// ConsoleLogAppender
class ConsoleLogAppender : public LogAppender
{
  public:
    ConsoleLogAppender(LogLevel level, const LogFormatter &formatter);
    ~ConsoleLogAppender() override = default;

  protected:
    void logToDest(const std::string &log) const override;

  private:
    static std::mutex m_mutex;
};

// FileLogAppender
class FileLogAppender : public LogAppender
{
  public:
    FileLogAppender(const std::string &filename, LogLevel level, const LogFormatter &formatter);
    ~FileLogAppender() override = default;

  protected:
    void logToDest(const std::string &log) const override;

  private:
    std::string        m_filename;
    mutable std::mutex m_mutex;
};

// Logger
class Logger
{
  public:
    Logger(const std::string &name = "root", LogLevel level = LogLevel::DEBUG);
    void log(LogLevel level, const std::shared_ptr<LogEvent> &event) const;

    void addAppender(const std::shared_ptr<LogAppender> &appender);

  private:
    std::string                               m_name;
    LogLevel                                  m_level;
    std::vector<std::shared_ptr<LogAppender>> m_appenders;
};

#define LOG_DEBUG(logger, content)                                                                                     \
    logger.log(LogLevel::DEBUG,                                                                                        \
               std::make_shared<LogEvent>(__FILE__, __LINE__, std::chrono::steady_clock::now(),                        \
                                          std::this_thread::get_id(), 0, std::chrono::system_clock::now(), content))

#define LOG_INFO(logger, content)                                                                                      \
    logger.log(LogLevel::INFO,                                                                                         \
               std::make_shared<LogEvent>(__FILE__, __LINE__, std::chrono::steady_clock::now(),                        \
                                          std::this_thread::get_id(), 0, std::chrono::system_clock::now(), content))

#define LOG_WARN(logger, content)                                                                                      \
    logger.log(LogLevel::WARN,                                                                                         \
               std::make_shared<LogEvent>(__FILE__, __LINE__, std::chrono::steady_clock::now(),                        \
                                          std::this_thread::get_id(), 0, std::chrono::system_clock::now(), content))

#define LOG_ERROR(logger, content)                                                                                     \
    logger.log(LogLevel::ERROR,                                                                                        \
               std::make_shared<LogEvent>(__FILE__, __LINE__, std::chrono::steady_clock::now(),                        \
                                          std::this_thread::get_id(), 0, std::chrono::system_clock::now(), content))

#define LOG_FATAL(logger, content)                                                                                     \
    logger.log(LogLevel::FATAL,                                                                                        \
               std::make_shared<LogEvent>(__FILE__, __LINE__, std::chrono::steady_clock::now(),                        \
                                          std::this_thread::get_id(), 0, std::chrono::system_clock::now(), content))

#endif // LOGGER_H