#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MESSAGE "Hello daddy"
#define MSG_SIZE (sizeof(MESSAGE)) // includes null terminator
#define SHARED_REGION_SIZE (sizeof(int) + MSG_SIZE) // turn + message

#define GET_SIZE sbrk(0)

struct shared_data {
    int turn;
    char message[MSG_SIZE];
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
        sleep(2); // Ensure parent has time to set up shared memory
        printf("Child process started with size: %p\n", GET_SIZE);
        void* va = GET_SIZE - SHARED_REGION_SIZE; // get the address of the shared memory
        printf("va: %p\n", va);
        // print value in *va
        printf("Shared memory int (turn): %d\n", *((int *)va));
        // Wait until parent maps shared memory
        // while (shared_buf->turn == 0) {
        //     sleep(1);
        // }

        struct shared_data *shared_child = (struct shared_data *) (va);

        printf("Child process size after shared mapping: %p\n", GET_SIZE);

        // Write message into shared memory
        strcpy(shared_child->message, MESSAGE);
        shared_child->turn = 0; // notify parent

        // Wait for parent to finish reading
        while (shared_child->turn == 0) {
            sleep(1);
        }

        // Unmap shared memory
        unmap_shared_pages(shared_child, SHARED_REGION_SIZE);
        printf("Child process size after unmapping: %p\n", GET_SIZE);

        void *extra = malloc(20);
        if (extra == 0) {
            printf("Child malloc failed\n");
            return 1;
        }
        printf("Child malloc 20 more: %p\n", GET_SIZE);
        free(extra);

        return 0;
    } else {
        // Parent process
        // Map shared_buf into child's address space
        sleep(1); // Ensure child has time to set up shared memory
        if (map_shared_pages(child_pid, shared_buf, SHARED_REGION_SIZE) < 0) {
            printf("Mapping failed\n");
            free(shared_buf);
            return -1;
        }
        shared_buf->turn = 1;
        // printf("Parent process started with size: %p\n", GET_SIZE);
        // Let child continue

        // Wait for child to write message
        while (shared_buf->turn == 1) {
            sleep(10);
            printf("Parent waiting for child to write message...\n");
        }

        printf("Parent read: %s\n", shared_buf->message);

        // Notify child to exit
        shared_buf->turn = 1;

        // Wait for child to finish (optional)
        wait(0);

        free(shared_buf);
        return 0;
    }
}
