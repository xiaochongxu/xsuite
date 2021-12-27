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

#define PAGE_SIZE  0x1000
#define PAGE_MASK  ~(PAGE_SIZE - 1)

uint32_t g_heartbeat_count = 0;
uint8_t  g_lock = 0;
uint8_t  g_instruction_roll[12] = { 0 };
void    *g_patch_handle = NULL;
void    *g_old_func = &heart_beat;
char     g_command_line[64] = { 0 };

void load_patch(int sig)
{
    if (g_patch_handle) {
        g_lock--;
        return;
    }

    uint8_t result = 0;
    g_patch_handle = dlopen("./libhot_fix.patch.so", RTLD_LAZY | RTLD_GLOBAL);
    if (g_patch_handle == NULL) {
        g_lock--;
        return;
    }

    do {
        void *fix_func = dlsym(g_patch_handle, "heart_beat_fix");
        if (fix_func == NULL) {
            break;
        }

        mprotect((void*)((uintptr_t)g_old_func & PAGE_MASK), PAGE_SIZE * 8, PROT_READ|PROT_WRITE|PROT_EXEC);

        memset(g_old_func + 0,  0x48, 1);
        memset(g_old_func + 1,  0xb8, 1);
        memcpy(g_old_func + 2,  &fix_func, 8);
        memset(g_old_func + 10, 0xff, 1);
        memset(g_old_func + 11, 0xe0, 1);
        result = 1;
    } while(0);

    if (result == 0) {
        printf("reason: %s \r\n", dlerror());
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

    memcpy(g_old_func, g_instruction_roll, sizeof(g_instruction_roll));
    dlclose(g_patch_handle);
    g_patch_handle = NULL;
    g_lock--;
}

void command()
{
    while (1) {
        memset(g_command_line, 0, sizeof(g_command_line));

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

void heart_beat()
{
    struct timespec time;
    struct tm nowTime;
    clock_gettime(CLOCK_REALTIME, &time);
    localtime_r(&time.tv_sec, &nowTime);

    char log[1024] = { 0 };
    snprintf(log, 1024, "\r\n[%04d-%02d-%02d %02d:%02d:%02d][0x%08x][heartbeat_version_ERROR!] Input# ",
        nowTime.tm_year + 1900, nowTime.tm_mon + 1, nowTime.tm_mday, 
        nowTime.tm_hour, nowTime.tm_min, nowTime.tm_sec, 
        g_heartbeat_count++);
    printf("%s", log);
    fflush(stdout);
}

void *service(void*)
{
    while (1) {
        sleep(2);
        if (g_lock == 0) {
            heart_beat();
        }
    }
    return NULL;
}

int main()
{
    memcpy(g_instruction_roll, g_old_func, sizeof(g_instruction_roll));

    signal(SIGNAL_LOAD_PATCH, load_patch);
    signal(SIGNAL_UNLOAD_PATCH, unload_patch); 

    pthread_t tid;
    pthread_create(&tid, NULL, service, NULL);

    command();
    return XSUITE_SUCCESS;
}