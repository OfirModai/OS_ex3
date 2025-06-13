#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

//assuming that dst_proc->lock is already acquired
uint64
map_shared_pages(struct proc* src_proc, struct proc* dst_proc, uint64 src_va, uint64 size) {
  uint64 src_start = PGROUNDDOWN(src_va);
  uint64 src_end = PGROUNDUP(src_va + size);
  uint64 dst_start = PGROUNDUP(dst_proc->sz);
  uint64 dst_va = dst_start;

  // Lock both processes to access their fields safely
  // Always acquire locks in consistent order to avoid deadlock
  struct proc *first, *second;
  if(src_proc < dst_proc) {
    first = src_proc;
    second = dst_proc;
  } else {
    first = dst_proc;
    second = src_proc;
  }

  acquire(&first->lock);
  if(first != second) {
    acquire(&second->lock);
  }


  for (uint64 addr = src_start; addr < src_end; addr += PGSIZE) {
    pte_t* src_pte = walk(src_proc->pagetable, addr, 0);
    if (src_pte == 0 || (*src_pte & PTE_V) == 0 || (*src_pte & PTE_U) == 0) {
      // Release locks before returning
      if(first != second) {
        release(&second->lock);
      }
      release(&first->lock);
      return -1;
    }

    uint64 src_pa = PTE2PA(*src_pte);
    int src_flags = PTE_FLAGS(*src_pte) | PTE_S; // seting as not own the page

    if (mappages(dst_proc->pagetable, dst_va, PGSIZE, src_pa, src_flags) != 0) {
      // Release locks before returning
      if(first != second) {
        release(&second->lock);
      }
      release(&first->lock);
      return -1;
    }

    dst_va += PGSIZE;
  }

  dst_proc->sz = dst_va;

  uint64 offset = src_va - src_start;
  offset = 0;
  // Release locks before returning
  if(first != second) {
    release(&second->lock);
  }
  release(&first->lock);
  return dst_start + offset;
}

uint64
unmap_shared_pages(struct proc* p, 
                   uint64 addr,
                   uint64 size)
{
  acquire(&p->lock);
  
  uint64 start = PGROUNDDOWN(addr);
  uint64 end = PGROUNDUP(addr + size);
  for (uint64 a = start; a < end; a += PGSIZE) {
    pte_t *pte = walk(p->pagetable, a, 0);
    if (pte == 0 || (*pte & PTE_V) == 0 || (*pte & PTE_S) == 0) {
      release(&p->lock);
      return -1;
    }
  }

  uint64 oldsz = PGROUNDUP(p->sz);
  uint64 npages = (end - start) / PGSIZE;
  uvmunmap(p->pagetable, start, npages, 1);
  p->sz = oldsz - (npages * PGSIZE);
  release(&p->lock);
  return 0;
}
