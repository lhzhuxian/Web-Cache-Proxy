#include <errno.h>
#include <execinfo.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <sstream>
#include <string>
#include <iostream>
#include "log.h"


namespace log {
#define ARRAYSIZE(a) (sizeof(a) / sizeof(*(a)))
#define MAX_PATH_LEN    1024
#define MAX_FILENAME_LEN 256


static DEVICE_TYPE  g_device_type     = DEV_FILE;
static LOG_PRIORITY g_log_priority    = LOG_PRIORITY_DEBUG;
static uint32_t     g_max_file_size   = 10; // 10M
static uint32_t     g_min_file_size   = 1;  // 1M
static uint32_t     g_max_roll_num    = 10;
static uint32_t     g_min_roll_num    = 1;
static char         g_file_path[1024] = "./log";

static const char* g_device_str[]   = { "STDOUT", "FILE" };
static const char* g_priority_str[] = { "TRACE", "DEBUG", "INFO", "ERROR", "FATAL" };

class LogFile {
public:
    static void WriteLog(const char* buff, uint32_t len);
    static void WriteError(const char* buff, uint32_t len);
    static void Close();
    static void Flush();

private:
    static const char* GetFileName(const char* type, int index);
    static int OpenFile(FILE **file, const char* mode, const char* type, int index);
    static uint32_t GetLatestRollNum(const char* type);
    static FILE* LogFD(uint32_t len);
    static FILE* ErrorFD(uint32_t len);


private:
    static FILE*    m_log;
    static FILE*    m_error;
    static uint32_t m_roll_idx_log;
    static uint32_t m_roll_idx_error;
};


int MakeDir(const char* path)
{
    if (!path) {
        return -1;
    }

    if (access(path, F_OK | W_OK) == 0) {
        return 0;
    }
    
    if (mkdir(path, 0755) != 0) {
        fprintf(stderr, "mkdir %s failed(%s)\n", path, strerror(errno));
        return -1;
    }

    return 0;
}

int MakeDirs(const char* path)
{
    if (!path) {
        return -1;
    }

    int len = strlen(path);
    char tmp[MAX_PATH_LEN] = {0};
    snprintf(tmp, ARRAYSIZE(tmp), "%s", path);

    for (int i = 0; i < len; i++) {
        if (tmp[i] != '/') {
            continue;
        }
        if (0 == i) {
            continue;
        }

        tmp[i] = '\0';
        if (MakeDir(tmp) != 0) {
            return -1;
        }
        tmp[i] = '/';
    }

    return MakeDir(path);
}

const char* GetTimeStamp()
{
    static char buff[64] = {0};
    static struct timeval tv_now;
    static time_t now;
    static struct tm tm_now;
    static struct tm* p_tm_now;

    gettimeofday(&tv_now, NULL);
    now = (time_t)tv_now.tv_sec;
    p_tm_now = localtime_r(&now, &tm_now);

    snprintf(buff, ARRAYSIZE(buff), "%04d%02d%02d %02d:%02d:%02d.%06d",
        1900 + p_tm_now->tm_year,
        p_tm_now->tm_mon + 1,
        p_tm_now->tm_mday,
        p_tm_now->tm_hour,
        p_tm_now->tm_min,
        p_tm_now->tm_sec,
        static_cast<int>(tv_now.tv_usec));

    return buff;
}

const char* GetSelfName()
{
    static char filename[MAX_FILENAME_LEN] = {0};
    if (0 == filename[0]) {
        char path[64]   = {0};
        char link[MAX_PATH_LEN] = {0};

        snprintf(path, ARRAYSIZE(path), "/proc/%d/exe", getpid());
        readlink(path, link, ARRAYSIZE(link));

        strncpy(filename, strrchr(link, '/') + 1, ARRAYSIZE(filename));
    }

    return filename;
}


FILE*    LogFile::m_log             = NULL;
FILE*    LogFile::m_error           = NULL;
uint32_t LogFile::m_roll_idx_log    = 0;
uint32_t LogFile::m_roll_idx_error  = 0;

const char* LogFile::GetFileName(const char* type, int index)
{
    static char file_name[MAX_PATH_LEN] = {0};
    memset(file_name, 0, ARRAYSIZE(file_name));
    int len = snprintf(file_name, ARRAYSIZE(file_name),
        "%s/%s.%s", g_file_path, GetSelfName(), type);
    if (index != 0) {
        snprintf(file_name + len, ARRAYSIZE(file_name) - len, ".%d", index);
    }
    return file_name;
}

int LogFile::OpenFile(FILE **file, const char* mode, const char* type, int index)
{
    *file = fopen(GetFileName(type, index), mode);
    if (!(*file)) {
        fprintf(stderr, "fopen %s:%s failed %s\n", GetFileName(type, index), mode, strerror(errno));
    }
    return ((*file != NULL) ? 0 : -1);
}

uint32_t LogFile::GetLatestRollNum(const char* type)
{
    uint32_t latest_idx = 0;
    time_t latest_tm    = 0;
    off_t file_size     = g_max_file_size * 1024 * 1024 + 1;
    struct stat file_info;

    for (uint32_t i = 0; i < g_max_roll_num; i++) {
        const char* file_name = GetFileName(type, i);
        if (access(file_name, F_OK | W_OK) != 0) {
            break;
        }

        if (stat(file_name, &file_info) != 0) {
            continue;
        }

        if (file_info.st_mtime >= latest_tm
            && file_info.st_size <= file_size) {
            latest_tm = file_info.st_mtime;
            file_size = file_info.st_size;
            latest_idx = i;
        }
    }
    return latest_idx;
}

FILE* LogFile::LogFD(uint32_t len)
{
  if (!m_log) {
    printf("log file location %s \n", g_file_path);
      if (MakeDirs(g_file_path) == 0) {
	m_roll_idx_log = GetLatestRollNum("log");
	OpenFile(&m_log, "a", "log", m_roll_idx_log);
      }
  }
  if (!m_log) {
      return NULL;
  }
  
  if (ftell(m_log) + len < g_max_file_size * 1024 * 1024) {
    return m_log;
    }

    fflush(m_log);
    fclose(m_log);
    m_log = NULL;

    ++m_roll_idx_log;
    m_roll_idx_log = m_roll_idx_log % g_max_roll_num;
    OpenFile(&m_log, "w", "log", m_roll_idx_log);

    return m_log;
}

FILE* LogFile::ErrorFD(uint32_t len)
{
    if (!m_error) {
        if (MakeDirs(g_file_path) == 0) {
            m_roll_idx_error = GetLatestRollNum("error");
            OpenFile(&m_error, "a", "error", m_roll_idx_error);
        }
    }
    if (!m_error) {
        return NULL;
    }

    if (ftell(m_error) + len < g_max_file_size * 1024 * 1024) {
        return m_error;
    }

    fflush(m_error);
    fclose(m_error);
    m_error = NULL;

    ++m_roll_idx_error;
    m_roll_idx_error = m_roll_idx_error % g_max_roll_num;
    OpenFile(&m_error, "w", "error", m_roll_idx_error);

    return m_error;
}

void LogFile::WriteLog(const char* buff, uint32_t len)
{
    if (!buff) {
        return;
    }

    FILE* logfd = LogFD(len);
    if (!logfd) {
        return;
    }

    fwrite(buff, len, 1, logfd);
}

void LogFile::WriteError(const char* buff, uint32_t len)
{
    if (!buff) {
        return;
    }

    FILE* errfd = ErrorFD(len);
    if (!errfd) {
        return;
    }

    fwrite(buff, len, 1, errfd);
}

void LogFile::Close()
{
    if (m_log) {
        fflush(m_log);
        fclose(m_log);
        m_log = NULL;
    }

    if (m_error) {
        fflush(m_error);
        fclose(m_error);
        m_error = NULL;
    }
}

void LogFile::Flush()
{
    if (m_log) {
        fflush(m_log);
    }

    if (m_error) {
        fflush(m_error);
    }
}

void Log::Write(LOG_PRIORITY pri, const char* file, uint32_t line,const char* function, const char* fmt, ...)
{
    if (pri < g_log_priority) {
        return;
    }

    static char buff[4096] = {0};
    int pre_len = 0;
	//pre_len = snprintf(buff, ARRAYSIZE(buff), "[%s][%d][(%s:%d)(%s)][%s] ",GetTimeStamp(),getpid(),file,line,function,g_priority_str[pri]);
	if (pre_len < 0) {
		pre_len = 0;
	}
   

    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buff + pre_len, ARRAYSIZE(buff) - pre_len, fmt, ap);
    va_end(ap);
    if (len < 0) {
        len = 0;
    }

    uint32_t tail = len + pre_len;
    if (tail > (ARRAYSIZE(buff) - 2)) {
        tail = ARRAYSIZE(buff) - 2;
    }
    buff[tail++] = '\n';
    buff[tail] = '\0';

    if (DEV_STDOUT == g_device_type) {
        fprintf(stdout, "%s", buff);
        return;
    }

    if (pri >= LOG_PRIORITY_ERROR) {
        LogFile::WriteError(buff, tail);
    }

    LogFile::WriteLog(buff, tail);
}

void Log::Close()
{
    LogFile::Close();
}

void Log::Flush()
{
    LogFile::Flush();
}

int Log::SetOutputDevice(DEVICE_TYPE device)
{
    if (device < DEV_STDOUT || device > DEV_FILE) {
        return -1;
    }
    g_device_type = device;
    return 0;
}

int Log::SetOutputDevice(const std::string& device)
{
    for (uint32_t i = 0; i < ARRAYSIZE(g_device_str); i++) {
        if (g_device_str[i] == device) {
            g_device_type = static_cast<DEVICE_TYPE>(i);
            return 0;
        }
    }
    return -1;
}

int Log::SetLogPriority(LOG_PRIORITY priority)
{
    if (priority < LOG_PRIORITY_TRACE || priority > LOG_PRIORITY_FATAL) {
        return -1;
    }
    g_log_priority = priority;
    return 0;
}

int Log::SetLogPriority(const std::string& priority)
{
    for (uint32_t i = 0; i < ARRAYSIZE(g_priority_str); i++) {
        if (g_priority_str[i] == priority) {
            g_log_priority = static_cast<LOG_PRIORITY>(i);
            return 0;
        }
    }
    return -1;
}

uint32_t Log::SetMaxFileSize(uint32_t max_size_Mbytes)
{
    if (max_size_Mbytes < g_min_file_size) {
        g_max_file_size = g_min_file_size;
        return g_max_file_size;
    }
    g_max_file_size = max_size_Mbytes;
    return g_max_file_size;
}

uint32_t Log::SetMaxRollNum(uint32_t num)
{
    if (num < g_min_roll_num) {
        g_max_roll_num = g_min_roll_num;
        return g_max_roll_num;
    }
    g_max_roll_num = num;
    return g_max_roll_num;
}

int Log::SetFilePath(const std::string& file_path)
{
    if (file_path.length() > ARRAYSIZE(g_file_path) || file_path.empty()) {
        return -1;
    }
    std::cout << file_path.c_str() << '\n';
    if (MakeDirs(file_path.c_str()) != 0) {
        return -2;
    }

    strncpy(g_file_path, file_path.c_str(), ARRAYSIZE(g_file_path));

    Close();
    return 0;
}


} // namespace log


