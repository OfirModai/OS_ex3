#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
//#include "kernel/memlayout.h"
//#include <stdint.h>

#define SHMEM_SIZE 4000               
#define MAX_MESSAGE_LENGTH 30         // max message length
#define ENTITY MAX_MESSAGE_LENGTH + 4 // 4 bytes header

int write_message(void *parent_va)
{
    sleep(1); // make some undeterministic
    uint32 child_pid = getpid();
    char prefix[] = "Hello from child "; // can be changed to whatever
    uint64 start = map_shared_pages((uint64)parent_va, SHMEM_SIZE);
    if (start == 0)
    {
        printf("Child Process %d: map_shared_pages failed\n", child_pid);
        exit(1);
    }
    uint64 va = start;
    // ensuring current position with added value don't overflow
    for (; va + ENTITY < start + SHMEM_SIZE; va += ENTITY)
    {
        va = (va + 3) & ~3; // Align to 4 bytes
        if (__sync_val_compare_and_swap((uint32 *)va, 0, child_pid) == 0)
        {
            char *buf = (char *)va;
            uint16 message_length = strlen(prefix) + 1; // +1 for null terminator
            if (message_length > MAX_MESSAGE_LENGTH)
            {
                exit(1);
            }
            memcpy(buf + 2, &message_length, sizeof(uint16));
            strcpy(buf + 4, prefix);
            exit(0);
        }
    }
    exit(1); // No space left to write the message
}
int main(int argc, char *argv[])
{
    int n_processes = 1;
    void *parent_va = malloc(SHMEM_SIZE);
    if (argc > 1)
    {
        n_processes = atoi(argv[1]);
    }
    for (int i = 0; i < n_processes; i++)
    {
        int pid = fork();
        if (pid < 0)
        {
            printf("Fork failed\n");
            exit(1);
        }
        else if (pid == 0)
        {
            // Child process
            exit(write_message(parent_va));
        }
    }
    sleep(n_processes); // Give child processes time to write
    for (uint64 va = (uint64)parent_va; va < (uint64)parent_va + SHMEM_SIZE; va += ENTITY)
    {
        va = (va + 3) & ~3; // Align to 4 bytes
        uint16 child_pid = *(uint16 *)va;
        if (child_pid == 0)
        {
            printf("done reading messages\n");
            break; // No more messages
        }
        uint16 message_length = *(uint16 *)(va + 2);
        char *message = (char *)(va + 4);
        if (message_length > ENTITY - 4 || va + message_length > (uint64)parent_va + SHMEM_SIZE)
        {
            exit(1);
        }
        printf("Parent Process: received message from child %d: %s\n", child_pid, message);
    }
    int status;
    for (int i = 0; i < n_processes; i++)
    {
        int pid = wait(&status);
        if (pid < 0)
        {
            printf("Wait failed\n");
            exit(1);
        }
        if (status != 0)
        {
            printf("Child process %d exited with error\n", pid);
        }
    }
    free(parent_va);
    exit(0);
}