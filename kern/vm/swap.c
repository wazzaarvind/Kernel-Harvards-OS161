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
#include <bitmap.h>
#include <kern/iovec.h>
#include <uio.h>
#include <vfs.h>
#include <kern/fcntl.h>
#include <kern/stat.h>
#include <mips/tlb.h>
#include <spl.h>

int evict_page(void){
  //kprintf("\nevict");
  //Lame

  int found = -1;
  int i = swapStart;
  //int end = swapStart - 1;

  // if(swapStart == 0)
  //   end = tpages-1;

  while(1){
    if(i == tpages-1){
      i = 0;
    }
    if(coremap[i].state == NOT_RECENTLY_USED){ // Change for LRU.
      found = i;
      swapStart = found + 1;
      break;
    }

    if(coremap[i].state == RECENTLY_USED){
      coremap[i].state = NOT_RECENTLY_USED;
    }
    i++;
  }
  return found;
}

void swap_out(struct page_table *store){
  //kprintf("\nSwapOut")

    struct page_table *temp = store;

    int temp_index = temp->bitmapIndex;

    unsigned int ind = 0;

    lock_acquire(bitmap_lock);

    int check = bitmap_alloc(swapTable, &ind);

    if(check != 0){
      kprintf("\nInside bitmap %d", ind);
    }

    temp->bitmapIndex = ind;

    //if(bitmap_isset(swapTable,(unsigned)temp_index) == true)
    if(temp_index>0)
       bitmap_unmark(swapTable,(unsigned)temp_index);



    lock_release(bitmap_lock);

    //lock_acquire(temp->pt_lock);

    int spl = 0;

    spl = splhigh();
    int index = tlb_probe(temp->vaddr, 0);
    if(index > 0)
    {
        tlb_write(TLBHI_INVALID(index), TLBLO_INVALID(), index);
    }
    splx(spl);

    // Synchronization required!!
    temp->mem_or_disk = IN_DISK; // Change mem to disk



    // Move the contents to disk.
    struct uio uioWrite;
    struct iovec iov;

    iov.iov_kbase = (void *)PADDR_TO_KVADDR(temp->paddr);
    iov.iov_len = PAGE_SIZE;

    uioWrite.uio_iov = &iov;
    uioWrite.uio_iovcnt = 1;
    uioWrite.uio_offset = temp->bitmapIndex * PAGE_SIZE;
    uioWrite.uio_resid = PAGE_SIZE;
    uioWrite.uio_segflg = UIO_SYSSPACE;
    uioWrite.uio_rw = UIO_WRITE;
    uioWrite.uio_space = NULL;
    //kprintf("\nSwapOutmid");

    int check2 = VOP_WRITE(swap_vnode, &uioWrite);

    if(check2){

      kprintf("\nVOP_WRITE fail\n");
    }

    temp->paddr = -1;


    //lock_release(temp->pt_lock);

}

void swap_in(struct page_table *first){
  //kprintf("\nSwapIN");

  struct uio uioRead;
  struct iovec iovRead;

  iovRead.iov_kbase = (void *)PADDR_TO_KVADDR(first->paddr);
  //iovRead.iov_kbase = (void *)first->vaddr;
  iovRead.iov_len = PAGE_SIZE;

  uioRead.uio_iov = &iovRead;
  uioRead.uio_iovcnt = 1;
  uioRead.uio_offset = first->bitmapIndex * PAGE_SIZE; //not sure
  uioRead.uio_resid = PAGE_SIZE;
  uioRead.uio_segflg = UIO_SYSSPACE;
  uioRead.uio_rw = UIO_READ;
  uioRead.uio_space = NULL;

  VOP_READ(swap_vnode, &uioRead);
  //kprintf("\nFails?\n");
  first->mem_or_disk = IN_MEMORY;

  int index = first->bitmapIndex;
  first->bitmapIndex = -1;

  lock_acquire(bitmap_lock);

  //if(bitmap_isset(swapTable,(unsigned)index) == true)
  //{
    //kprintf("\nInside bitmap swap_in %d",bitmap_isset(swapTable,index));
    bitmap_unmark(swapTable,(unsigned)index);
    //kprintf("\nInside bitmap swap_in second%d",bitmap_isset(swapTable,index));
  //}
  lock_release(bitmap_lock);

}


void swap_in_disk(struct page_table *first){
  //kprintf("\nSwapIN");

  struct uio uioRead;
  struct iovec iovRead;

  iovRead.iov_kbase = (void *)PADDR_TO_KVADDR(first->paddr);
  //iovRead.iov_kbase = (void *)first->vaddr;
  iovRead.iov_len = PAGE_SIZE;

  uioRead.uio_iov = &iovRead;
  uioRead.uio_iovcnt = 1;
  uioRead.uio_offset = first->bitmapIndex * PAGE_SIZE; //not sure
  uioRead.uio_resid = PAGE_SIZE;
  uioRead.uio_segflg = UIO_SYSSPACE;
  uioRead.uio_rw = UIO_READ;
  uioRead.uio_space = NULL;

  VOP_READ(swap_vnode, &uioRead);
  //kprintf("\nFails?\n");
  first->mem_or_disk = IN_MEMORY;

  first->bitmapIndex = -1;

}

void updateTLB(vaddr_t faultaddress, paddr_t paddr){

  int spl = 0;
  uint32_t ehi,elo;

  spl = splhigh();

  // if(tlbStart < NUM_TLB){
  //   for (int i=0; i<NUM_TLB; i++) {
  //       tlb_read(&ehi, &elo, i);
  //       if (elo & TLBLO_VALID) {
  //           continue;
  //         }
  //       ehi = faultaddress;
  //       elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
  //       DEBUG(DB_VM, "dumbvm: 0x%x -> 0x%x\n", faultaddress, paddr);
  //       tlb_write(ehi, elo, i);
  //       splx(spl);
  //       return;
  //   }
  // }
  ehi = faultaddress;
  elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
  tlb_write(ehi, elo, tlbStart%NUM_TLB);
  splx(spl);
  tlbStart++;
}
