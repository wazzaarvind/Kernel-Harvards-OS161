#include <mainbus.h>
#include <types.h>
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

void vm_initialise{

 	last = 0;
	start = 0;
	size = 0;

	coremap_lock = lock_create("coremap_lock");
	last = ram_getsize(); // Get the last address of ram.
	start = ram_getfirstfree(); // Get the first address of ram.

	size = last - start;

	size = size/4096;

	coremap_page coremap_page[size];

	for(int i = 0;i < size;i++){
		coremap_page[i].available=1;
		coremap_page[i].chunk_size=0;
		coremap_page[i].owner=-1;
		coremap_page[i].state=0;
	}

	numBytes = 0;
}

vaddr_t alloc_kpages(unsigned npages)
{
	paddr_t pa;

	lock_acquire(coremap_lock);

	for(int i=0;i<size;i++){
		if(coremap_page[i].available!=1)
			continue;
		if(coremap_page[i+npages-1].available!=1)
			continue;
		break;
	}

	if(i==size-1&&coremap_page[size-1].available!=1&&npages>1) {
		lock_release(coremap_lock);
		return 0;
	}

	numBytes += npages * 4096;
	coremap_page[i].chunk_size=npages;
	coremap_page[i].start = firstpaddr + (i * 4096);
	int start_alloc=i;
	while(npages>0)
	{
		coremap_page[i++].available=0;
		//do we need to modify other variables?
		npages--;
	}

	lock_release(coremap_lock);

	// What is happening here :
	return PADDR_TO_KVADDR(coremap_page[start_alloc].start); //start_alloc*4096?

}

void free_kpages(vaddr_t addr)
{
	lock_acquire(coremap_lock);

	// Sanity check on address

	for(int i=0;i<size;i++){
		if(coremap_page[i].start == addr){
			break;
		}
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

	lock_release(coremap_lock);

}

unsigned int coremap_used_bytes(void)
{
	return numBytes;
}

void vm_bootstrap(void)
{
	// Do nothing.
}

void vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("VM tried to do tlb shootdown?!\n");
}
