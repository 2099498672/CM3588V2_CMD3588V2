#include "util/Log.h"
#include <cstdarg>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "util/theradpoolv1/thread_pool.h"

thread_pool* log_thread = new thread_pool(1);   

// 全局日志级别控制
static LogLevel currentLogLevel = defaultLogLevel;

// 设置日志级别
void SetLogLevel(LogLevel level) {
    currentLogLevel = level;
}

// 获取当前时间字符串
std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// 日志输出辅助函数
static void log_output(LogLevel level, const char* TAG, const char* format, va_list args) {
    // 检查日志级别是否需要输出
    if (level < currentLogLevel) {
        return;
    }
    
    // 获取日志级别字符串
    const char* level_str;
    switch (level) {
        case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
        case LOG_LEVEL_INFO:  level_str = "INFO ";  break;
        case LOG_LEVEL_WARN:  level_str = "WARN ";  break;
        case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
        default:              level_str = "UNKNOWN";
    }
    
    // 输出日志头部
    std::cout << "[" << get_current_time() << "] [" << level_str << "] [" << TAG << "] ";
    
    // 输出日志内容
    vprintf(format, args);
    std::cout << std::endl;
}

// 各日志级别的实现
void LogDebug(const char* TAG, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_output(LOG_LEVEL_DEBUG, TAG, format, args);
    va_end(args);
}

void LogInfo(const char* TAG, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_output(LOG_LEVEL_INFO, TAG, format, args);
    va_end(args);
}

void LogWarn(const char* TAG, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_output(LOG_LEVEL_WARN, TAG, format, args);
    va_end(args);
}

void LogError(const char* TAG, const char* format, ...) {
    va_list args;
    va_start(args, format);
    log_output(LOG_LEVEL_ERROR, TAG, format, args);
    va_end(args);
}

void log_output_str(LogLevel level, const char* TAG, const std::string& msg) {
    if (level < currentLogLevel) {
        return;
    }

    const char* level_str;
    switch (level) {
        case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
        case LOG_LEVEL_INFO:  level_str = "INFO ";  break;
        case LOG_LEVEL_WARN:  level_str = "WARN ";  break;
        case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
        default:              level_str = "UNKNOWN";
    }

    std::cout << "[" << get_current_time()
              << "] [" << level_str
              << "] [" << TAG << "] "
              << msg << std::endl;
}

void log_thread_safe(LogLevel level, const char* TAG, const char* format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    std::string msg(buffer);

    log_thread->submit([level, TAG, msg]() {
        log_output_str(level, TAG, msg);
    });
}

