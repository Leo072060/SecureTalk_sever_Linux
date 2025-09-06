#include "logger.h"

LogEvent::LogEvent(const char *file, int32_t line, std::chrono::steady_clock::time_point elapse, std::thread::id threadId, uint32_t fiberId,
                   std::chrono::system_clock::time_point time, const std::string &content)
    : m_file(file), m_line(line), m_elapse(elapse), m_threadId(threadId), m_fiberId(fiberId), m_time(time),
      m_content(content)
{
}

LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern) {}

std::string LogFormatter::format(LogLevel level, const std::shared_ptr<LogEvent> &event) const
{
    if (!event) return "";

    std::ostringstream ss;
    const char        *str = m_pattern.c_str();

    while (*str)
    {
        if (*str == '%' && *(str + 1))
        {
            ++str;
            switch (*str)
            {
            case 'd': {
                std::time_t t = std::chrono::system_clock::to_time_t(event->m_time);

                std::tm tm_time;
                localtime_r(&t, &tm_time);

                ss << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
                break;
            }
            case 'p': {
                switch (level)
                {
                case LogLevel::DEBUG:
                    ss << "\033[37m[DEBUG]\033[0m ";
                    break;
                case LogLevel::INFO:
                    ss << "\033[32m[INFO]\033[0m ";
                    break;
                case LogLevel::WARN:
                    ss << "\033[33m[WARN]\033[0m ";
                    break;
                case LogLevel::ERROR:
                    ss << "\033[31m[ERROR]\033[0m ";
                    break;
                case LogLevel::FATAL:
                    ss << "\033[41m[FATAL]\033[0m ";
                    break;
                }
                break;
            }
            case 't':
                ss << event->m_threadId;
                break;
            case 'f':
                ss << event->m_file;
                break;
            case 'l':
                ss << event->m_line;
                break;
            case 'm':
                ss << event->m_content;
                break;
            case 'n':
                ss << "\n";
                break;
            case '%':
                ss << "%";
                break;
            default:
                ss << "%" << *str;
                break;
            }
        }
        else
        {
            ss << *str;
        }
        ++str;
    }

    return ss.str();
}

LogAppender::LogAppender(LogLevel level, const LogFormatter &formatter) : m_level(level), m_formatter(formatter) {}

void LogAppender::log(LogLevel level, const std::shared_ptr<LogEvent> &event) const
{
    if (level >= m_level)
    {
        std::string formattedLog = m_formatter.format(level, event);
        logToDest(formattedLog);
    }
}

std::mutex ConsoleLogAppender::m_mutex;

ConsoleLogAppender::ConsoleLogAppender(LogLevel level, const LogFormatter &formatter) : LogAppender(level, formatter) {}

void ConsoleLogAppender::logToDest(const std::string &log) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::cout << log;
}

void FileLogAppender::logToDest(const std::string &log) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::ofstream               ofs(m_filename, std::ios::app);
    if (ofs.is_open())
    {
        ofs << log;
        ofs.close();
    }
}

FileLogAppender::FileLogAppender(const std::string &filename, LogLevel level, const LogFormatter &formatter)
    : LogAppender(level, formatter), m_filename(filename)
{
}

Logger::Logger(const std::string &name, LogLevel level) : m_name(name), m_level(level) {}

void Logger::log(LogLevel level, const std::shared_ptr<LogEvent> &event) const
{
    for (const auto &appender : m_appenders)
    {
        if (appender)
        {
            appender->log(level, event);
        }
    }
}

void Logger::addAppender(const std::shared_ptr<LogAppender> &appender)
{
    if (appender)
    {
        m_appenders.push_back(appender);
    }
}