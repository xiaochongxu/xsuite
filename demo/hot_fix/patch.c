#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include "service.h"

extern uint32_t g_heartbeat_count;

void heart_beat_fix(int log_fd)
{
    struct timespec time;
    struct tm nowTime;
    clock_gettime(CLOCK_REALTIME, &time);
    localtime_r(&time.tv_sec, &nowTime);

    char log[1024] = { 0 };
    snprintf(log, 1024, "[%04d-%02d-%02d %02d:%02d:%02d][heartbeat_version_HOTFIX!][0x%08x]\r\n",
        nowTime.tm_year + 1900, nowTime.tm_mon + 1, nowTime.tm_mday, 
        nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec, g_heartbeat_count++);
    write(log_fd, log, strlen(log));
    fsync(log_fd);
}