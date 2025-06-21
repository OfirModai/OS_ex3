#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/memlayout.h"
#include <stdint.h>
#include <stdbool.h>


#define SHMEM_SIZE 4096 // size of a memory page (4KB)    
#define NUM_CHILDREN 4 // number of child processes to create           
#define MAX_MESSAGE_LENGTH 64         // max message length

// this function concatenates src to dst and returns the new length of dst
int my_strcat(char* dst, const char* src)
{
    int dst_len = 0;
    while (dst[dst_len] != '\0') dst_len++;

    int i = 0;
    while (src[i] != '\0')
    {
        dst[dst_len + i] = src[i];
        i++;
    }
    dst[dst_len + i] = '\0';

    return dst_len + i;  // return new length of string (excluding null terminator)
}

//this function converts an integer to a string and appends it to buf at the specified offset
int my_atoi(char* buf, int offset, int num)
{
    if (num == 0)
    {
        buf[offset++] = '0';
        return offset;
    }

    char temp[10];
    int len = 0;

    while (num > 0 && len < sizeof(temp))
    {
        temp[len++] = '0' + (num % 10);
        num /= 10;
    }

    for (int i = len - 1; i >= 0; i--)
    {
        buf[offset++] = temp[i];
    }

    return offset;
}

// this function copies the string from src to dst and returns the length of the copied string
int my_strcpy(char* dst, const char* src)
{
    int i = 0;
    while (src[i] != '\0')  // Loop until the end of src string
    {
        dst[i] = src[i];    // Copy each character from src to dest
        i++;
    }
    dst[i] = '\0';          // Copy the null terminator to dest
    return i;               // Return the number of characters copied (excluding the null terminator)
}

void child_process(int child_idx, void *parent_va)
{
    char msg[MAX_MESSAGE_LENGTH];
    char stars[child_idx+1];
    memset(stars, '*', child_idx);
    stars[child_idx] = '\0'; // Null-terminate the string

    int pos = 0;
    pos = my_strcpy(msg, "Child Process ");
    pos = my_atoi(msg, pos, child_idx);

    pos = my_strcat(msg, ": hello ");
    pos = my_strcpy(msg + pos, stars);

    uint64 buf = map_shared_pages((uint64) parent_va, SHMEM_SIZE);   

    if (buf == (uint64)-1)
    {
        printf("Child Process %d: map_shared_pages failed\n", child_idx);
        exit(1);
    }

    uint64 va = buf + 4; // save 4 bytes for finished child counter
    uint16 message_length = strlen(msg) + 1; // +1 for null terminator
    uint64 slot_size = message_length + 4; // 4 bytes for the header 

    // ensuring current position with added value don't overflow
    while(va + slot_size < buf + SHMEM_SIZE)
    {
        va = (va + 3) & ~3; // Align to 4 bytes
        
        uint32 header = ((uint32)child_idx << 16) | message_length;

        if (__sync_val_compare_and_swap((uint32 *)va, 0, header) == 0)
        {
            memcpy((char *)va + 4, msg, message_length);
            break;
        }

        uint16 curr_msg_len = *(uint32 *)va & 0xFFFF; 
        va += 4 + curr_msg_len; // Move to the next slot
    }

    __sync_fetch_and_add((uint32 *)buf, 1); // Increment the finished child counter
    exit(0);
}

void parent_process(void *buf, int num_children)
{
    char *num_read = malloc(num_children);
    if (!num_read) {
        printf("malloc failed\n");
        exit(1);
    }
    memset(num_read, 0, num_children);
    uint32* finished_children = (uint32*) buf; // Pointer to the finished child counter
    char *addr = (char *)buf + 4; // Start reading messages after the finished child counter
    uint64 end = (uint64)buf + SHMEM_SIZE;

    while (true)
    {
        char* cur = addr;

        while ((uint64)cur + 4 <= end)  // Must have space for at least header
        {
            cur = (char*)(((uint64)cur + 3) & ~3); // Align to 4 bytes
            if ((uint64)cur + 4 > end)
            {
                break;
            }
            uint32 header = *(uint32*)cur;

            if (header == 0)
            {
                cur += 4; // Skip empty slots
                continue;
            }

            uint16 child_id = header >> 16;
            uint16 len = header & 0xFFFF;

            if ((uint64)cur + 4 + len > end)
            {
                printf("Parent Process: out of bounds\n");
                exit(1);
            }

            if (child_id >= 1 && child_id <= num_children && !num_read[child_id - 1])
            {
                char msg[MAX_MESSAGE_LENGTH + 1] = {0};
                memcpy(msg, cur + 4, len < MAX_MESSAGE_LENGTH ? len : MAX_MESSAGE_LENGTH);
                msg[len < MAX_MESSAGE_LENGTH ? len : MAX_MESSAGE_LENGTH] = '\0';

                printf("Parent Process: Child Process %d wrote %s\n", child_id, msg);

                num_read[child_id - 1] = 1;

                cur += 4 + len; // Move to the next message slot

            }
        }

        if (*finished_children >= num_children)
        {
            break;
        }
    }

    printf("Parent Process: All messages read from children\n");
    exit(0);
}


int main(int argc, char *argv[])
{
    void *parent_va = malloc(SHMEM_SIZE);
    int num_children = NUM_CHILDREN; // Default number of child processes

    if (argc > 1)
    {
        num_children = atoi(argv[1]);
    }

    for (int i = 0; i < num_children; i++)
    {
        int pid = fork();

        if (pid < 0)
        {
            printf("Parent Process: Fork failed\n");
            exit(1);
        }

        // child process
        if (pid == 0)
        {
            child_process(i+1, parent_va);
        }
    }

    parent_process(parent_va, num_children);
    return 0;
}