#include <types.h>
#include <mainbus.h>
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>

vaddr_t alloc_upages(){

  int npages=1;
  int alloc = 0;
  int req = 1;
  //kprintf("\ntpages: %d\n",tpages);
  //kprintf("\nReq: %d\n",req);
  //int startPage = 0;
  int i = 0, startAlloc = 0;

  spinlock_acquire(&vmlock);

  for(i = 0; i < (int) tpages; i++){
    if(coremap[i].available == 1){
      for(int j = i;  j <  i + (int) npages; j++){
        if(coremap[j].available == 1){
          alloc +=1;
        } else {
          break;
        }
        if(alloc == req){
          break;
        }
      }
    }

    if(alloc == req){
      break;
    } else {
      alloc = 0;
    }
  }

  if(i == (int)(tpages)){
    if(req > 1){
      spinlock_release(&vmlock);
      return (vaddr_t)NULL;
    }
    if(coremap[i].available == 0){
      spinlock_release(&vmlock);
      return (vaddr_t)NULL;
    }
  }

  startAlloc = i;
  //kprintf("\nAlloc: %d\n",alloc);
  numBytes += alloc * PAGE_SIZE;


  while(req > 0){
    req--;
    coremap[i].available = 0;
    coremap[i].chunk_size = npages;
    i++;
  }
  //kprintf("\nMagic in VM: %d\n",numBytes);

  spinlock_release(&vmlock);

  return PADDR_TO_KVADDR(startAlloc * PAGE_SIZE);

  return paddr;
}

void free_upage(vaddr_t addr)
{
  int i = 0;
  int pagesToInvalidate = 0;
  vaddr_t iter_addr;

  spinlock_acquire(&vmlock);

  for(i = 0; i < (int) tpages; i++){
    iter_addr = PADDR_TO_KVADDR(i * PAGE_SIZE);
    if(iter_addr == addr){
      break;
    }
  }

  // Sanity checks :
  if(coremap[i].available == 1 || coremap[i].chunk_size == 0){
    spinlock_release(&vmlock);
    return;
  }

  pagesToInvalidate = coremap[i].chunk_size;

  numBytes -= pagesToInvalidate * PAGE_SIZE;

  while (pagesToInvalidate > 0) {
    coremap[i].available = 1;
    coremap[i].chunk_size = 0;
    i++;
    pagesToInvalidate--;
  }

  spinlock_release(&vmlock);

}
