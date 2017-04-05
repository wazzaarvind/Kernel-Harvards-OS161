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

void vm_initialise() {

 	last = 0;
	start = 0;
	size = 0;

	last = ram_getsize(); // Get the last address of ram.

	kprintf("Last %d\n",last);

	size = last/4096;
	kprintf("Size %d\n",size);

	start = ram_getfirstfree();  // Used by kernel
	kprintf("Start %d\n",start);


  coremap_page = (struct coremap *)PADDR_TO_KVADDR(start); // Suggested by Ben.
	//struct coremap coremap_page[size];

	// Find size used by core map
	int memOfCoremap = sizeof(struct coremap) * size;
	kprintf("MCM %d\n",memOfCoremap);


	int pages =  (start + memOfCoremap)/4096;

  if( (start + memOfCoremap) % 4096 > 0){
    pages += 1;
  }

  start = pages * 4096;

	kprintf("Pages %d\n",pages);

	for(int i=0;i<pages;i++){
		coremap_page[i].available=0;
		coremap_page[i].chunk_size=0;
		//coremap_page[i].owner=-2;
		//coremap_page[i].state=0;
    //kprintf("%d : %d", coremap_page[i].owner, i);
		//change this later
	}

  kprintf("Updated start %d\n",start);

	for(int i = pages; i < size; i++){
		coremap_page[i].available=1;
		coremap_page[i].chunk_size=0;
		//coremap_page[i].owner=-1;
		//coremap_page[i].state=0;
	}

  // for(int i = start_index; i < size; i++){
  //
  //   kprintf("%d : %d", coremap_page[i].owner, i);
  //
  //   //change this later
  // }
  //

  numBytes = pages * 4096;
  kprintf("Num bytes : %d \n", numBytes);
}

vaddr_t alloc_kpages(unsigned npages)
{
  int i = 0;
  int j = 0;

	int flag=0;

	for(i = 0; i < size; i++){
    // Add another check here.
		for(j = i; (unsigned) j < i + npages; j++){

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

	if(i == size-1  && npages>1) {
		return 0;
	}

	numBytes += npages * 4096;
	coremap_page[i].chunk_size = npages;
	//coremap_page[i].start =  (i * 4096);
	int start_alloc=i;
	while(npages>0)
	{
		coremap_page[i].available=0;
		//do we need to modify other variables?
		npages--;
    i++;
	}

	// What is happening here :
	return PADDR_TO_KVADDR(start_alloc * 4096); //start_alloc*4096?

}

void free_kpages(vaddr_t addr)
{
  int i = 0;

	// Sanity check on address
	// Sanity check on owner

	for(int i=0;i<size;i++){
    vaddr_t Page_addr = PADDR_TO_KVADDR(i * 4096);
		if(Page_addr == addr){
			break; //need to convert
		}
	}

  // if(i == 0){
  //   // Handle page requested to be removed is not found.
  // }

	int npages = coremap_page[i].chunk_size;

	numBytes -= npages * 4096;

	while(npages > 0){
		coremap_page[i].available=1;
		coremap_page[i].chunk_size=0;
		// coremap_page[i].owner=-1;
		// coremap_page[i].state=0;
		i++;
		npages--;
	}


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
