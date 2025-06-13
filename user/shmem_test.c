#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MESSAGE "Hello daddy"
#define MSG_SIZE (sizeof(MESSAGE) + 400) // includes null terminator
#define SHARED_REGION_SIZE (sizeof(int) + MSG_SIZE) // turn + message

#define GET_SIZE sbrk(0)

struct shared_data {
    volatile int turn;
    volatile char message[MSG_SIZE];
};

int main(void)
{
    struct shared_data *shared_buf = malloc(SHARED_REGION_SIZE);
    if (shared_buf == 0) {
        printf("Memory allocation failed\n");
        return 1;
    }

    shared_buf->turn = 0; // 1 means parent's turn

    int child_pid = fork();
    if (child_pid < 0) {
        printf("Fork failed\n");
        free(shared_buf);
        return 1;
    }

    if (child_pid == 0) {
        // Child process
        printf("Child process started with size: %p\n", GET_SIZE);
        sleep(3); // Ensure parent has time to set up shared memory
        
        void* va = GET_SIZE - SHARED_REGION_SIZE; // get the address of the shared memory
        printf("va of shared memory in child: %p\n", va);
        struct shared_data *shared_child = (struct shared_data *) (va);
        printf("Child process size after shared mapping: %p\n", GET_SIZE);

        // Write message into shared memory
        strcpy((char *)shared_child->message, MESSAGE);
        shared_child->turn = 1; // notify parent

        // Wait for parent to finish reading
        while (shared_child->turn == 1) {
            sleep(1);
        }

        // Unmap shared memory
        if (unmap_shared_pages(shared_child, SHARED_REGION_SIZE) < 0) {
            printf("Unmapping failed\n");
            free(shared_buf);
            return -1;
        }
        printf("Child process size after unmapping: %p\n", GET_SIZE);

        void *extra = malloc(5000);
        if (extra == 0) {
            printf("Child malloc failed\n");
            return 1;
        }
        printf("Child after malloc 20 more: %p\n", GET_SIZE);
        free(extra);
        return 0;

    } else {
        // Parent process
        // Map shared_buf into child's address space
        sleep(1); // Ensure child has time to print his size
        uint64 va;
        if ((va = map_shared_pages(child_pid, shared_buf, SHARED_REGION_SIZE)) < 0) {
            printf("Mapping failed\n");
            free(shared_buf);
            return -1;
        }
        printf("Parent: map returned va: %p\n", va);
        shared_buf->turn = 0; // Let child continue
        // Wait for child to write message
        while (shared_buf->turn == 0) {
            sleep(1);
        }

        printf("Parent read: %s\n", shared_buf->message);

        // Notify child to exit
        shared_buf->turn = 0;

        // Wait for child to finish
        wait(0);

        free(shared_buf);
        return 0;
    }
}
