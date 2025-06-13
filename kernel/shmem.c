#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

//assuming that dst_proc->lock is already acquired
uint64
map_shared_pages(struct proc* src_proc,
                 struct proc* dst_proc,
                 uint64 src_va, uint64 size) 
{
  // lock for axcessing pagetable
  acquire(&src_proc->lock);
  
  // finding the pte of src_va in src_proc
  pte_t *pte = walk(src_proc->pagetable, src_va, 0);
  if(pte == 0 || (*pte & PTE_V) == 0 || (*pte & PTE_U) == 0) {
    release(&src_proc->lock);
    return -1; // no valid mapping found
  }
  release(&src_proc->lock);
  
  uint64 perm_bits = PTE_FLAGS(*pte) | PTE_S; // add the not owned bit PTE_S
  uint64 pa = PTE2PA(*pte); // physical address of the page
  uint64 oldsz = PGROUNDUP(dst_proc->sz);
  
  // iterate for PGSIZE to map the size in PA, to pagetable of dst, like in uvmalloc
  for(uint64 a = oldsz; a < oldsz + size; a += PGSIZE) {
    if(mappages(dst_proc->pagetable, a, PGSIZE, pa, perm_bits) != 0) {
      unmap_shared_pages(dst_proc, oldsz, a - oldsz); // unmap the pages we already mapped
      release(&dst_proc->lock);
      return -1;
    }
    pa += PGSIZE;
  }
  dst_proc->sz = oldsz + size;
  release(&dst_proc->lock);
  return oldsz; //I think we need to return the va where the shared memory is starting, but I am not sure
}

uint64
unmap_shared_pages(struct proc* p, 
                   uint64 addr,
                   uint64 size)
{
  acquire(&p->lock);
  // firstly check that addr exists in p and it's shared
  pte_t *pte = walk(p->pagetable, addr, 0);
  if(pte == 0 || (*pte & PTE_V) == 0 || (*pte & PTE_S) == 0) {
    release(&p->lock);
    return -1; // no shared mapping found
  }
  uint64 oldsz = PGROUNDUP(p->sz);
  uint64 npages = size / PGSIZE;
  uvmunmap(p->pagetable, PGROUNDUP(addr), npages, 1);
  p->sz = oldsz - npages * PGSIZE;
  release(&p->lock);
  return 0;
}                   
