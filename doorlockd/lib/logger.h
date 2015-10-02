#ifndef LOGGER_H
#define LOGGER_H

#include <ostream>
#include <string>
#include <sstream>
#include <mutex>

/**
 * @brief The LogLevel enum
 */
enum class LogLevel : unsigned char
{
    off,     /// disable logging
    error,   /// error conditions
    warning, /// warning conditions
    notice,  /// normal but significant conditions
    info,    /// informational messages
    debug,   /// debug-level messages
    debug2   /// more debug-level messages
};

/**
 * @brief The Logger class
 *
 * The logger class is a thread-safe class which is used for formatting and forwarding
 * log messages.
 */
class Logger final
{
public:
    static Logger &get();

    /// Log a string
    void operator() (const std::string &message, const LogLevel level = LogLevel::debug) const;

    /// Log a ostringstream
    void operator() (const std::ostringstream &message, const LogLevel level = LogLevel::debug) const;

    /// Log a format string
    void operator() (const LogLevel level, const char* format, ...) const;

    /// Set minimum required log level to generate output
    void level(const LogLevel level);
    /// Get minimum required log level to generate output
    LogLevel level() const;

private:

    /**
     * @brief Constructor
     * @param level Minimum required log level to generate output
     */
    Logger(const LogLevel level);
    ~Logger();

    LogLevel _level;
    mutable std::mutex _ostreamMutex;
};
#endif
