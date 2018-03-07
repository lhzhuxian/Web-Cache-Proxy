#pragma once
#include <stdint.h>

namespace log {
typedef enum {
    DEV_STDOUT,
    DEV_FILE,
} DEVICE_TYPE;

typedef enum {
    LOG_PRIORITY_TRACE,
    LOG_PRIORITY_DEBUG,
    LOG_PRIORITY_INFO,
    LOG_PRIORITY_ERROR,
    LOG_PRIORITY_FATAL,
} LOG_PRIORITY;

class Log {
public:
    static int SetOutputDevice(DEVICE_TYPE device);

    static int SetOutputDevice(const std::string& device);

    static int SetLogPriority(LOG_PRIORITY priority);

    static int SetLogPriority(const std::string& priority);

    static uint32_t SetMaxFileSize(uint32_t max_size_Mbytes);

    static uint32_t SetMaxRollNum(uint32_t num);

    static int SetFilePath(const std::string& file_path);

    static void Write(LOG_PRIORITY pri, const char* file, uint32_t line,const char* function, const char* fmt, ...);

    static void Close();

    static void Flush();
};

} // namespace log


#define LOG_FATAL(fmt, ...) log::Log::Write(log::LOG_PRIORITY_FATAL, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); 
#define LOG_ERROR(fmt, ...) log::Log::Write(log::LOG_PRIORITY_ERROR, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); 
#define LOG_INFO(fmt, ...)  log::Log::Write(log::LOG_PRIORITY_INFO,  __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); 
#define LOG_DEBUG(fmt, ...) log::Log::Write(log::LOG_PRIORITY_DEBUG, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__);
#define LOG_TRACE(fmt, ...) log::Log::Write(log::LOG_PRIORITY_TRACE, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__);

