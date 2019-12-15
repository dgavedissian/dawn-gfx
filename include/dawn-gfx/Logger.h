/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include <fmt/format.h>

namespace dw {
enum class LogLevel { Debug, Info, Warning, Error };

class DW_API Logger {
public:
virtual ~Logger() = default;

virtual void log(LogLevel level, const std::string& value) = 0;
template <typename... Args> void debug(const std::string& format, const Args&... args);
template <typename... Args> void info(const std::string& format, const Args&... args);
template <typename... Args> void warn(const std::string& format, const Args&... args);
template <typename... Args> void error(const std::string& format, const Args&... args);
};

template <typename... Args> void Logger::debug(const std::string& format, const Args&... args) {
    log(LogLevel::Debug, fmt::format(format, args...));
}

template <typename... Args> void Logger::info(const std::string& format, const Args&... args) {
    log(LogLevel::Info, fmt::format(format, args...));
}

template <typename... Args> void Logger::warn(const std::string& format, const Args&... args) {
    log(LogLevel::Warning, fmt::format(format, args...));
}

template <typename... Args> void Logger::error(const std::string& format, const Args&... args) {
    log(LogLevel::Error, fmt::format(format, args...));
}
}  // namespace dw
