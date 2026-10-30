#pragma once
#include <deque>
#include <string>
namespace eng {
enum class LogLevel { Info, Warn, Error };
struct LogEntry { LogLevel level; std::string text; };
void LogInfo(const char* fmt, ...);
void LogWarn(const char* fmt, ...);
void LogError(const char* fmt, ...);
// 目前假设单线程访问 (UI 线程 = 主线程)。多线程化时在此加锁。
const std::deque<LogEntry>& LogEntries();
}
