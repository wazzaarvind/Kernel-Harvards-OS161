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


  int found = -1;
  int i = swapStart;
  int end = swapStart - 1;

  if(swapStart == 0)
    end = tpages-1;

  while(i != end){
    if(i == tpages-1){
      i = 0;
    }
    if(coremap[i].state == RECENTLY_USED){ // Change for LRU.
      found = i;
      swapStart = found + 1;
      break;
    }
    i++;
  }
  return found;
}

void swap_out(int i, struct page_table *store){
  //kprintf("\nSwapOut");

  struct page_table *temp = store; // First page of process owning this cormap page.
        // page align faultaddress and find coresponding page in the PTE.
    while(temp != NULL){


      if(temp->paddr == (unsigned int)(i * PAGE_SIZE))  //does this necessarily need to be the case? Will it never be in between?
      {
        unsigned int ind = 0;

        lock_acquire(bitmap_lock);
          int check = bitmap_alloc(swapTable, &ind);
          //kprintf("\nInside bitmap %d", ind);
        lock_release(bitmap_lock);


        int spl = 0;

        spl = splhigh();
        int index = tlb_probe(temp->vaddr, 0);
        if(index > 0)
        {
            tlb_write(TLBHI_INVALID(index), TLBLO_INVALID(), index);
        }
        splx(spl);

        lock_acquire(temp->pt_lock);

        // Synchronization required!!
        temp->mem_or_disk = IN_DISK; // Change mem to disk
        temp->bitmapIndex = ind;

        if(check != 0){
          // TODO : Handle edge case.
          kprintf("\nBitmap fail\n");
        }

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

        lock_release(temp->pt_lock);
        temp->paddr = -1;
        break;
      }


      temp = temp->next;

    }

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

  if(bitmap_isset(swapTable,(unsigned)index) == true)
  {
    //kprintf("\nInside bitmap swap_in %d",index);
    bitmap_unmark(swapTable,(unsigned)index);
  }
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
