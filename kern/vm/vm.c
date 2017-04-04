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

int last;
int start;
int size;

int numBytes;

struct coremap *coremap_page;

void vm_initialise() {

 	last = 0;
	start = 0;
	size = 0;

	last = ram_getsize(); // Get the last address of ram.

	size = last/4096;

	start = ram_getfirstfree();  // Used by kernel

  coremap_page = (struct coremap *)PADDR_TO_KVADDR(start);
	//struct coremap coremap_page[size];

	// Find size used by core map
	int memOfCoremap = sizeof(struct coremap) * size;

	int pages =  (start + memOfCoremap)/4096;

	int start_index = 0;

	// Look for partial pages.
	// if(pages > pages + 0.0001){
	// 	 	start_index = (int) pages + 1;
	// } else {
	// 		start_index = (int) pages;
	// }

  start_index=pages;

	for(int i=0;i<start_index;i++){
		coremap_page[i].available=0;
		coremap_page[i].chunk_size=0;
		coremap_page[i].owner=-2;
		coremap_page[i].state=0;
		//change this later
	}

	for(int i = start_index; i < size; i++){
		coremap_page[i].available=1;
		coremap_page[i].chunk_size=0;
		coremap_page[i].owner=-1;
		coremap_page[i].state=0;
	}

	numBytes = start;
}

vaddr_t alloc_kpages(unsigned npages)
{
  int i = 0;
  int j = 0;

	int flag=0;

	for(i=0; i<size; i++){

		for(j=i; (unsigned)j<i+npages; j++){
			if(coremap_page[j].available!=1)
			{
				flag = 1;
				break;
			}
		}

		if(flag == 1){
			flag = 0;
			continue;
		}

		break;
	}

	if(i==size-1 && coremap_page[size-1].available!=1 && npages>1) {
		return 0;
	}

	numBytes += npages * 4096;
	coremap_page[i].chunk_size=npages;
	coremap_page[i].start = start + (i * 4096);
	int start_alloc=i;
	while(npages>0)
	{
		coremap_page[i++].available=0;
		//do we need to modify other variables?
		npages--;
	}

	// What is happening here :
	return PADDR_TO_KVADDR(coremap_page[start_alloc].start); //start_alloc*4096?

}

void free_kpages(vaddr_t addr)
{
  int i = 0;

	// Sanity check on address
	// Sanity check on owner

	for(int i=0;i<size;i++){
		if(coremap_page[i].start == addr){
			break;
		}
	}

  if(i == 0){
    // Handle page requested to be removed is not found.
  }

	int npages = coremap_page[i].chunk_size;

	numBytes -= npages * 4096;

	while(npages > 0){
		coremap_page[i].available=1;
		coremap_page[i].chunk_size=0;
		coremap_page[i].owner=-1;
		coremap_page[i].state=0;
		i++;
		npages--;
	}


}

unsigned int coremap_used_bytes(void)
{
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
