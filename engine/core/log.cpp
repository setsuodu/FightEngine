#include "core/log.h"
#include <cstdarg>
#include <cstdio>
#include <windows.h>
namespace eng {
static std::deque<LogEntry> g_entries;
static void Push(LogLevel lv, const char* fmt, va_list ap) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    OutputDebugStringA(buf); OutputDebugStringA("\n");
    g_entries.push_back({lv, buf});
    if (g_entries.size() > 2000) g_entries.pop_front();
}
void LogInfo(const char* f, ...)  { va_list a; va_start(a, f); Push(LogLevel::Info,  f, a); va_end(a); }
void LogWarn(const char* f, ...)  { va_list a; va_start(a, f); Push(LogLevel::Warn,  f, a); va_end(a); }
void LogError(const char* f, ...) { va_list a; va_start(a, f); Push(LogLevel::Error, f, a); va_end(a); }
const std::deque<LogEntry>& LogEntries() { return g_entries; }
}
