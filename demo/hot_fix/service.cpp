#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "service.h"

using namespace std;
static bool g_load_patch_status = false;
static char g_command_line[64] = { 0 };

void load_patch(int sig)
{
    if (g_load_patch_status) {
        return;
    }


}

void unload_patch(int sig)
{
    if (!g_load_patch_status) {
        return;
    }


}

void command()
{
    while (true) {
        memset(g_command_line, 0, sizeof(g_command_line));

        cout << "Input#";
        fgets(g_command_line, sizeof(g_command_line), stdin);        
        
        if (strncmp(g_command_line, "load", strlen("load")) == 0) {
            raise(SIGNAL_LOAD_PATCH);
        }

        if (strncmp(g_command_line, "unload", strlen("unload")) == 0) {
            raise(SIGNAL_UNLOAD_PATCH);
        }

        if (strncmp(g_command_line, "quit", strlen("quit")) == 0) {
            exit(0);
        }        
    }
}

void heart_beat(int log_fd)
{
    timespec time;
    tm nowTime;
    clock_gettime(CLOCK_REALTIME, &time);  //获取相对于1970到现在的秒数
    localtime_r(&time.tv_sec, &nowTime);

    char log[1024] = { 0 };
    snprintf(log, 1024, "[%d-%d-%d %d:%d:%d][heartbeat]\r\n",
        nowTime.tm_year + 1900, nowTime.tm_mon + 1, nowTime.tm_mday, 
        nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec);
    write(log_fd, log, strlen(log));
    fsync(log_fd);
}

void *service(void*)
{
    int log_fd = open("./output", O_WRONLY | O_CREAT | O_APPEND, S_IRWXO);
    if (log_fd < 0) {
        cout << "open log failed![errno:"<< errno << "]" << endl;
        exit(-1);
    }

    while (true) {
        sleep(2);
        heart_beat(log_fd);
    }
    close(log_fd);
    return nullptr;
}

int main()
{
    signal(SIGNAL_LOAD_PATCH, load_patch);
    signal(SIGNAL_UNLOAD_PATCH, unload_patch); 

    pthread_t tid;
    pthread_create(&tid, nullptr, service, nullptr);

    command();
    return XSUITE_SUCCESS;
}