/*
 * Dawn Graphics
 * Written by David Avedissian (c) 2017-2020 (git@dga.dev)
 */
#pragma once

#include <fmt/format.h>
#include <string>

namespace dw {
namespace gfx {
enum class LogLevel { Debug, Info, Warning, Error };

class DW_API Logger {
public:
    virtual ~Logger() = default;

    virtual void log(LogLevel level, const std::string& value) const = 0;
    template <typename... Args> void debug(const std::string& format, const Args&... args) const;
    template <typename... Args> void info(const std::string& format, const Args&... args) const;
    template <typename... Args> void warn(const std::string& format, const Args&... args) const;
    template <typename... Args> void error(const std::string& format, const Args&... args) const;
};

template <typename... Args> void Logger::debug(const std::string& format, const Args&... args) const {
    log(LogLevel::Debug, fmt::format(format, args...));
}

template <typename... Args> void Logger::info(const std::string& format, const Args&... args) const {
    log(LogLevel::Info, fmt::format(format, args...));
}

template <typename... Args> void Logger::warn(const std::string& format, const Args&... args) const {
    log(LogLevel::Warning, fmt::format(format, args...));
}

template <typename... Args> void Logger::error(const std::string& format, const Args&... args) const {
    log(LogLevel::Error, fmt::format(format, args...));
}
}
}  // namespace dw
