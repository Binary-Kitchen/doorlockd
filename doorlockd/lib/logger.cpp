// (c) 2015 Ralf Ramsauer - Logger class from the sdfs project

#include <iostream>
#include <cstdarg>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <map>
#include <mutex>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "config.h"
#include "logger.h"

using std::map;
using std::string;
using std::mutex;
using std::lock_guard;
using std::move;

using std::cerr;
using std::endl;

#ifdef USE_COLORIZED_LOGS
#ifdef _WIN32 // If Windows, Use Windows Terminal coloring
const static map<LogLevel, WORD> colorAttribute = {
    {LogLevel::error,   FOREGROUND_RED },
    {LogLevel::warning, FOREGROUND_RED | FOREGROUND_GREEN },
    {LogLevel::notice,  FOREGROUND_BLUE },
    {LogLevel::info,    FOREGROUND_GREEN },
    {LogLevel::debug,   FOREGROUND_BLUE | FOREGROUND_RED },
    {LogLevel::debug2,  FOREGROUND_BLUE | FOREGROUND_RED },
};
#else // Use ANSI Escape sequences
const static map<LogLevel, string> prefix_ansicolor = {
    {LogLevel::error,   "\x1b[1m\x1b[31m" },
    {LogLevel::warning, "\x1b[1m\x1b[33m" },
    {LogLevel::notice,  "\x1b[1m\x1b[34m" },
    {LogLevel::info,    "\x1b[1m\x1b[32m" },
    {LogLevel::debug,   "\x1b[1m\x1b[35m" },
    {LogLevel::debug2,  "\x1b[1m\x1b[36m" },
};

const static string suffix_ansireset = "\x1b[0m";
#endif
#endif

const static map<LogLevel, string> logLevel = {
    {LogLevel::error,   "ERROR  " },
    {LogLevel::warning, "WARNING" },
    {LogLevel::notice,  "NOTICE " },
    {LogLevel::info,    "INFO   " },
    {LogLevel::debug,   "DEBUG  " },
    {LogLevel::debug2,  "DEBUG2 " },
};

Logger::Logger(const LogLevel level) :
    _level(level),
    _ostreamMutex()
{
}

Logger::~Logger()
{
}

Logger &Logger::get()
{
    static Logger l(DEFAULT_LOG_LEVEL);
    return l;
}

void Logger::operator ()(const std::string &message, const LogLevel level) const
{
    if(level > _level)
    {
        return;
    }

    std::ostringstream prefix;

    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);

#if defined(USE_COLORIZED_LOGS) && !defined(_WIN32)
    prefix << prefix_ansicolor.at(level);
#endif

    // GCC does not support put_time :-(
    /*stringstream ss;
    ss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
    prefix = "[" + ss.str() + "] ";*/

    const size_t BUFFER_SIZE = 80;
    char timeBuffer[BUFFER_SIZE];
    std::strftime(timeBuffer, BUFFER_SIZE, "%Y-%m-%d %H:%M:%S", &tm);
    prefix << "[" << timeBuffer << "] -- " << logLevel.at(level) << " :: ";

    // Critical section
    {
        lock_guard<mutex> lock(_ostreamMutex);

#if defined(USE_COLORIZED_LOGS) && !defined(_WIN32)
        cerr << prefix.str() << message << suffix_ansireset << endl;
        cerr.flush();
#elif defined(USE_COLORIZED_LOGS) && defined(_WIN32)

        // taken from GTEST
        const HANDLE stdout_handle = GetStdHandle(STD_ERROR_HANDLE);

        // Gets the current text color.
        CONSOLE_SCREEN_BUFFER_INFO buffer_info;
        GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
        const WORD old_color_attrs = buffer_info.wAttributes;

        // We need to flush the stream buffers into the console before each
        // SetConsoleTextAttribute call lest it affect the text that is already
        // printed but has not yet reached the console.
        cerr.flush();
        SetConsoleTextAttribute(stdout_handle,
                                colorAttribute.at(level) | FOREGROUND_INTENSITY);

        cerr << prefix.str() << message << endl;
        cerr.flush();

        // Restores the text color.
        SetConsoleTextAttribute(stdout_handle, old_color_attrs);

#else
        cerr << prefix.str() << message << endl;
        cerr.flush();
#endif
    }
}

void Logger::operator ()(const std::ostringstream &message, const LogLevel level) const
{
    (*this)(message.str(), level);
}

void Logger::operator ()(const LogLevel level, const char* format, ...) const
{
    if(level > _level)
    {
        return;
    }

    va_list argp;
    char* message = nullptr;

    // determine buffer length
    va_start(argp, format);
    int size = vsnprintf(nullptr, 0, format, argp) + 1;
    va_end(argp);

    if (size >= 0)
    {
        message = (char*)malloc(size);
        if (message == nullptr)
        {
            (*this)("[LOGGER] CRITICAL: MEMORY ALLOCATION ERROR",
            LogLevel::error);
        }

        va_start(argp,format);
        vsnprintf(message, size, format, argp);
        va_end(argp);

        (*this)(string(message), level);

        free(message);
    } else {
        (*this)("[LOGGER] CRITICAL: VSNPRINTF ERROR",
        LogLevel::error);
    }
}

void Logger::level(const LogLevel level)
{
    _level = level;
}

LogLevel Logger::level() const
{
    return _level;
}

void foo(void) {
}
