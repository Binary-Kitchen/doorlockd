#ifndef LOGGER_H
#define LOGGER_H

#include <fstream>
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
    void operator() (const std::string &message, const LogLevel level = LogLevel::debug);

    /// Log a ostringstream
    void operator() (const std::ostringstream &message, const LogLevel level = LogLevel::debug);

    /// Log a format string
    void operator() (const LogLevel level, const char* format, ...);

    /// Set minimum required log level to generate output
    void level(const LogLevel level);
    /// Get minimum required log level to generate output
    LogLevel level() const;

    /// Getter/Setter for console output
    bool console() const;
    void console(const bool active);

    /// Getter/Setter for logfile output
    bool logFile() const;
    void logFile(const bool active);
    void setLogFile(const std::string &logFile);

private:

    /**
     * @brief Constructor
     * @param level Minimum required log level to generate output
     */
    Logger(const LogLevel level);
    ~Logger();

    bool _consoleActive = { true };
    bool _logFileActive = { false };

    std::ofstream _logFile = {};

    LogLevel _level;
    mutable std::mutex _ostreamMutex;
};
#endif
