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


paddr_t size;

int tpages = 0;

int numBytes;

void vm_initialise() {

  tpages = 0; // Total possible pages in the ram.
  size = 0;
  numBytes = 0;

  spinlock_init(&vmlock);

  // Get the size of the RAM.
  size = ram_getsize();
  tpages = size/PAGE_SIZE;

  // Check how much is used by Kernel + Exception handler
  paddr_t f_paddr = ram_getfirstfree();

  // Init of coremap - vm.h
  // Allocate the memory used by coremap - start in first free
  coremap = (struct coremap_struct *)PADDR_TO_KVADDR(f_paddr); // Still in physical **

  // Coremap usage:
  int coremap_size = sizeof(struct coremap_struct) * tpages;

// Knvalidate pages :  K + EC + CM
  int mem_Kused = coremap_size + (f_paddr);
  int pages_Kused = mem_Kused/PAGE_SIZE;

  // Check for fractions :
  if(mem_Kused % PAGE_SIZE > 0){
    pages_Kused++;
  }

  for(int i = 0; i < pages_Kused; i++){
    coremap[i].available = 0;
    coremap[i].chunk_size = 0;
  }

  // Make the rest available :
  for(int j = pages_Kused; j < tpages; j++){
    coremap[j].available = 1;
    coremap[j].chunk_size = 0;
  }

  numBytes += pages_Kused * PAGE_SIZE;
  //kprintf("Num bytes : %d \n", numBytes);
}

vaddr_t alloc_kpages(unsigned npages)
{

  int alloc = 0;
  int req = (int) npages;
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

  if(i == (int)(tpages-1)){
    if(req > 1){
      spinlock_release(&vmlock);
      return 0;
    }
    if(coremap[i].available == 0){
      spinlock_release(&vmlock);
      return 0;
    }
  }

  startAlloc = i;
  numBytes += alloc * PAGE_SIZE;


  while(req > 0){
    req--;
    coremap[i].available = 0;
    coremap[i].chunk_size = (int) npages;
    i++;
  }

  spinlock_release(&vmlock);

  return PADDR_TO_KVADDR(startAlloc * PAGE_SIZE);

}

void free_kpages(vaddr_t addr)
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

unsigned int coremap_used_bytes(void)
{
	//kprintf("Hi%d\n",numBytes);
	return numBytes;
}

void vm_bootstrap(void)
{
	// Do nothing.
}

int vm_fault(int faulttype, vaddr_t faultaddress)
{
    (void) faulttype;
    (void) faultaddress;

    return 0;
}

void vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("VM tried to do tlb shootdown?!\n");
}
