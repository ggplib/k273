#pragma once

// std includes
#include <mutex>
#include <string>
#include <vector>

#include <stdio.h>
#include <stdarg.h>

namespace K273 {

    using LogLevel = int;

    class Logger;

    ///////////////////////////////////////////////////////////////////////////
    // null interface class

    class LogHandlerBase {
    public:
        enum { PrefixLength = 13 };

    public:
        LogHandlerBase(LogLevel level);
        virtual ~LogHandlerBase();

    public:
        virtual void onReport(Logger& logger, std::string& timestamp,
                              LogLevel level, const char* pt_msg) = 0;
        virtual void cleanup();

    public:
        LogLevel level;
    };

    ///////////////////////////////////////////////////////////////////////////

    class FileLogHandler: public LogHandlerBase {
    public:
        FileLogHandler(LogLevel level, const std::string& filename, bool coloured);
        virtual ~FileLogHandler();

    public:
        void onReport(Logger& logger, std::string& timestamp,
                      LogLevel level, const char* pt_msg);
        void cleanup();

    private:
        FILE* fp;
        bool coloured;
    };

    ///////////////////////////////////////////////////////////////////////////

    class ConsoleLogHandler: public LogHandlerBase {
    public:
        ConsoleLogHandler(LogLevel level);
        virtual ~ConsoleLogHandler();

    public:
        void onReport(Logger& logger, std::string& timestamp,
                      LogLevel level, const char* pt_msg);

    private:
        bool coloured;
    };

    ///////////////////////////////////////////////////////////////////////////
    // the logger class itself

    class Logger {
    public:
        // don't log
        static constexpr LogLevel LOG_NONE = 1;

        // critical conditions
        static constexpr LogLevel LOG_CRITICAL = 2;

        // error conditions
        static constexpr LogLevel LOG_ERROR = 3;

        // warning conditions
        static constexpr LogLevel LOG_WARNING = 4;

        // informational
        static constexpr LogLevel LOG_INFO = 5;

        // debug-level
        static constexpr LogLevel LOG_DEBUG = 6;

        // verbose debug-level
        static constexpr LogLevel LOG_VERBOSE = 7;

    public:
        Logger();
        ~Logger();

    public:
        void fileLogging(const std::string& msg, LogLevel level=Logger::LOG_VERBOSE);
        void consoleLogging(LogLevel level=Logger::LOG_VERBOSE);

        // admin api
        void addHandler(LogHandlerBase* handle);
        void removeHandler(LogHandlerBase* handle);

        void cleanup();

        // logging api
        void makeLog(LogLevel level, const char* fmt, va_list args);
        void makeLog(LogLevel level, const std::string& msg);

    private:
        void doLog(LogLevel level, const char* pt_msg);

    private:
        int seconds;
        int useconds;
        std::string seconds_str;
        std::string timestamp_str;
        bool update_timestamp;

        LogHandlerBase* file_handler;
        LogHandlerBase* console_handler;

        std::vector <LogHandlerBase*> handlers;
        std::mutex mutex;
    };

    ///////////////////////////////////////////////////////////////////////////

    void loggerSetup(const std::string& filename, LogLevel console_level);

    void addLogHandler(LogHandlerBase* handler);
    void removeLogHandler(LogHandlerBase* handler);

    ///////////////////////////////////////////////////////////////////////////

    // logging helpers
    void l_critical(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
    void l_critical(const std::string& msg);

    void l_error(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
    void l_error(const std::string& msg);

    void l_warning(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
    void l_warning(const std::string& msg);

    void l_info(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
    void l_info(const std::string& msg);

    void l_debug(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
    void l_debug(const std::string& msg);

    void l_verbose(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
    void l_verbose(const std::string& msg);
}
