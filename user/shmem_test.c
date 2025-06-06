#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(void)
{
    char *buf = malloc(4096);
    int target_pid = 5;

    if (map_shared_pages(target_pid, buf, 4096) < 0) {
        printf("Mapping failed\n");
    }
}