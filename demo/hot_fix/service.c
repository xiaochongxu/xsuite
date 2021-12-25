#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/mman.h>
#include "service.h"

#define PAGE_SHIFT 12
#define PAGE_SIZE  1 << 12
#define PAGE_MASK  ~(PAGE_SIZE - 1)

uint32_t g_heartbeat_count = 0;
uint8_t g_lock = 0;
char g_command_line[64] = { 0 };
void*g_patch_handle = NULL;

void load_patch(int sig)
{
    if (g_patch_handle) {
        g_lock--;
        return;
    }

    uint8_t result = 0;
    void *g_patch_handle = dlopen("./libhot_fix.patch.so", RTLD_LAZY | RTLD_GLOBAL);
    if (g_patch_handle == NULL) {
        g_lock--;
        return;
    }

    do {
        void *old_func = &heart_beat;
        void *fix_func = dlsym(g_patch_handle, "heart_beat_fix");
        if ((old_func == NULL) || (fix_func == NULL)) {
            break;
        }

        mprotect((void*)((uintptr_t)old_func & PAGE_MASK), PAGE_SIZE * 16, PROT_READ|PROT_WRITE|PROT_EXEC);

        memset(old_func, 0x48, 1);
        memset(old_func + 1, 0xb8, 1);
        memcpy(old_func + 2, &fix_func, 8);
        memset(old_func + 10, 0xff, 1);
        memset(old_func + 11, 0xe0, 1);
        result = 1;
    } while(0);

    if (result == 0) {
        printf("%s\r\n", dlerror());
        dlclose(g_patch_handle);
        g_patch_handle = NULL;
    }
    g_lock--;
}

void unload_patch(int sig)
{
    if (!g_patch_handle) {
        g_lock--;
        return;
    }

    
    g_lock--;
}

void command()
{
    while (1) {
        memset(g_command_line, 0, sizeof(g_command_line));
        printf("Input#");

        fgets(g_command_line, sizeof(g_command_line), stdin);        
        
        if (strncmp(g_command_line, "load", strlen("load")) == 0) {
            ++g_lock;
            raise(SIGNAL_LOAD_PATCH);
        }

        if (strncmp(g_command_line, "unload", strlen("unload")) == 0) {
            ++g_lock;
            raise(SIGNAL_UNLOAD_PATCH);
        }

        if (strncmp(g_command_line, "quit", strlen("quit")) == 0) {
            exit(0);
        }        
    }
}

void heart_beat(int log_fd)
{
    struct timespec time;
    struct tm nowTime;
    clock_gettime(CLOCK_REALTIME, &time);
    localtime_r(&time.tv_sec, &nowTime);

    char log[1024] = { 0 };
    snprintf(log, 1024, "[%04d-%02d-%02d %02d:%02d:%02d][heartbeat_version_ERROR!][0x%08x]\r\n",
        nowTime.tm_year + 1900, nowTime.tm_mon + 1, nowTime.tm_mday, 
        nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec, g_heartbeat_count++);
    write(log_fd, log, strlen(log));
    fsync(log_fd);
}

void *service(void*)
{
    int log_fd = open("./output", O_WRONLY | O_CREAT | O_APPEND, S_IRWXO);
    if (log_fd < 0) {
        printf("open log failed![errno:%d]\r\n", errno);
        exit(-1);
    }

    while (1) {
        sleep(2);
        if (g_lock == 0) {
            heart_beat(log_fd);
        }
    }
    close(log_fd);
    return NULL;
}

int main()
{
    signal(SIGNAL_LOAD_PATCH, load_patch);
    signal(SIGNAL_UNLOAD_PATCH, unload_patch); 

    pthread_t tid;
    pthread_create(&tid, NULL, service, NULL);

    command();
    return XSUITE_SUCCESS;
}