#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define SHARED_PAGES_SIZE sizeof("Hello daddy") + 2 // +2 for null terminator and turn
int
main(void)
{
    int turn = 1; //parent process
    int pid;
    if (pid = fork() < 0) {
        printf("Fork failed\n");
        return 1;
    }
    else if (pid == 0) //child process
    {
        
    }
    else //parent process
    {
        void* buf = malloc(SHARED_PAGES_SIZE);
        if (buf == 0) {
            printf("Memory allocation failed\n");
            return 1;
        }
        // mapping buf to child process
        if (map_shared_pages(pid, buf, SHARED_PAGES_SIZE) < 0) {
            printf("Mapping failed\n");
            free(buf);
            return 1;
        }
        

    }
    
    char *buf = malloc(SHARED_PAGES_SIZE);
    int target_pid = 5;

    if (map_shared_pages(target_pid, buf, 4096) < 0) {
        printf("Mapping failed\n");
    }
    return 0;
}