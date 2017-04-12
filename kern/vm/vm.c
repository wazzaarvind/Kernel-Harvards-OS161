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
#include <vm.h>
#include <addrspace.h>


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
    coremap[i].chunk_size = (int) npages;
    i++;
  }
  //kprintf("\nMagic in VM: %d\n",numBytes);

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

int vm_fault(int faulttype, vaddr_t faultaddress) // we cannot return int, no index in linked list
{

    int spl = 0;
    uint32_t ehi,elo;

    switch (faulttype) {
	    case VM_FAULT_READONLY:
		/* We always create pages read-write, so we can't get this */
		panic("dumbvm: got VM_FAULT_READONLY\n");
	    case VM_FAULT_READ:
	    case VM_FAULT_WRITE:
		break;
	    default:
		return EINVAL;
	 }

    if (curproc == NULL) {
      /*
      * No process. This is probably a kernel fault early
      * in boot. Return EFAULT so as to panic instead of
      * getting into an infinite faulting loop.
      */
      return EFAULT;
    }

    //as = proc_getas();

    if (curproc->p_addrspace == NULL) {
      /*
      * No address space set up. This is probably also a
      * kernel fault early in boot.
      */
      return EFAULT;
    }

    KASSERT(curproc->p_addrspace->sgmt!=NULL);
    KASSERT(curproc->p_addrspace->heap_top!=0);
    KASSERT(curproc->p_addrspace->heap_bottom!=0);
      //KASSERT for stack?



    int segFlag = 0;

    // Can I use curproc?
    // TODO : curproc->p_addrspace->sgmt KASSERT
    //
    // Page align to retrieve the PTE.
    //faultaddress &= PAGE_FRAME;

    struct segment *curseg = curproc->p_addrspace->sgmt;

    // struct segment *tempIter = curproc->p_addrspace->sgmt;
    // int segCount = 0;
    // while(tempIter != NULL){
    //   segCount++;
    //   tempIter = tempIter->next;
    // }

    while(curseg != NULL){
         if(faultaddress >= curseg->start && faultaddress < curseg->end){
            segFlag = 1;
            break;
         }
         curseg = curseg->next;
    }

    if(segFlag != 1){
      if(faultaddress >= curproc->p_addrspace->stack_top && faultaddress < curproc->p_addrspace->stack_bottom){
          segFlag = 1;
      }
    }

    // If its not in one of the segments.
    if(segFlag != 1){
      return EFAULT;
    }

    faultaddress &= PAGE_FRAME;


    // If it is, then find the corresponding PTE.
    struct page_table *first = curproc->p_addrspace->first_page;

    int pageFlag = 0;

    // page align faultaddress and find coresponding page in the PTE.
    while(first != NULL){
      //
      if(first->vaddr == faultaddress)  //does this necessarily need to be the case? Will it never be in between?
      {
        pageFlag=1;
        break;
      }
      first=first->next;
    }

    //check the same for heap and stack areas as well!
    // if pageFlag == 1 : The requested page already exists in the page table.
    // else : Allocate a new page and physical memory.
    if(pageFlag==1){
      /* Disable interrupts on this CPU while frobbing the TLB. */
      // TODO : KASSERT

        paddr_t paddr = first->paddr;

        spl = splhigh();

        // for (int i=0; i<NUM_TLB; i++) {
        //     tlb_read(&ehi, &elo, i);
        //     if (elo & TLBLO_VALID) {
        //         continue;
        //       }
        //     ehi = faultaddress;
        //     elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
        //     DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
        //     tlb_write(ehi, elo, i);
        //     splx(spl);
        //     return 0;
        // }

        // TLB is currently full so evicting a TLB entry
        ehi = faultaddress;
        elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
        tlb_random(ehi, elo);
        splx(spl);
        return 0;
    }

    // There is no PTE for the current vaddr_t.
    if(pageFlag==0)
    {
      // So create one
      struct page_table *cur_page = kmalloc(sizeof(struct page_table));

      cur_page->vaddr = faultaddress;
      cur_page->paddr = alloc_upages(); //page aligned address?
      cur_page->next = NULL;

      if(curproc->p_addrspace->last_page == NULL){
        curproc->p_addrspace->last_page = cur_page;
        curproc->p_addrspace->first_page = cur_page;
      } else {
        curproc->p_addrspace->last_page->next = cur_page;
        curproc->p_addrspace->last_page = curproc->p_addrspace->last_page->next;
      }

      paddr_t paddr = cur_page->paddr;

      // Fill up the TLB.
      // Load TLB and return.
      spl = splhigh();
      ehi = faultaddress;
      elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
      tlb_random(ehi, elo);
      splx(spl);
      return 0;

      // for (int i=0; i<NUM_TLB; i++) {
      //     tlb_read(&ehi, &elo, i);
      //     if (elo & TLBLO_VALID) {
      //         continue;
      //       }
      //     ehi = faultaddress;
      //     elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
      //     DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
      //     tlb_write(ehi, elo, i);
      //     splx(spl);
      //     return 0;
      // }

      // TLB is currently full so evicting a TLB entry
    }

    return EFAULT;
}

void vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("VM tried to do tlb shootdown?!\n");
}


vaddr_t alloc_upages(void){

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

  return startAlloc * PAGE_SIZE;

  //return paddr;
}

// Change this
void free_upage(paddr_t addr)
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
