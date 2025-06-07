#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_map_shared_pages(void)
{
  int dst_pid;
  uint64 src_va, size;

  argint(0, &dst_pid);
  argaddr(1, &src_va);
  argaddr(2, &size);
  
  struct proc *src = myproc();
  struct proc *dst = find_proc_by_pid(dst_pid); 
  if (dst == 0)
        return -1;
  
  return map_shared_pages(src, dst, src_va, size);
}

uint64
sys_unmap_shared_pages(void)
{
  uint64 addr, size;
  argaddr(0, &addr);
  argaddr(1, &size);
  struct proc *p = myproc();
  return unmap_shared_pages(p, addr, size);
}