#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "proc.h"
#include "vm.c"

uint64 sys_unmap_shared_pages(void)
{
  struct proc *p = myproc();
  uint64 addr, size;

  // Fetch the address and size of the shared memory to unmap.
  if (argaddr(0, &addr) < 0 || argaddr(1, &size) < 0)
    return -1;

  // Validate the address and size.
  if (addr >= p->sz || addr + size > p->sz || size == 0)
    return -1;

  // Unmap the shared pages.
  p->sz = unmap_shared_pages(p, addr, size);
  return p->sz;
}