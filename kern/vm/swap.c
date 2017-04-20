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
