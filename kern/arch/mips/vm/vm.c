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

int size = 0; 

int numBytes = 0; 

void vm_initialise{

	size = mainbus_ramsize();
	
	size = size/4096;
	
	coremap_page coremap_page[size];
	
	for(int i = 0;i < size;i++){
		coremap_page[i].available=1;
		coremap_page[i].chunk_size=0;
		coremap_page[i].owner=-1;
		coremap_page[i].state=0;
	}
}


/*static
paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;

	spinlock_acquire(&stealmem_lock);

	addr = ram_stealmem(npages);

	spinlock_release(&stealmem_lock);
	return addr;
}*/


vaddr_t alloc_kpages(unsigned npages)
{
	paddr_t pa;
	for(int i=0;i<size;i++){
		if(coremap_page[i].available!=1)
			continue;
		if(coremap_page[i+npages-1].available!=1)
			continue;
		break;
	}

	if(i==size-1&&coremap_page[size-1].available!=1)
		return 0;

	//pa = getppages(npages);
	//if (pa==0) {
	//	return 0;
	//}

	numBytes += npages * 4096; 
	coremap_page[i].chunk_size=npages;
	int start_alloc=i;
	while(npages>0)
	{
		coremap_page[i++].available=0;
		//do we need to modify other variables?
		npages--;
	}


	return PADDR_TO_KVADDR(pa); //start_alloc*4096?

}

void free_kpages(vaddr_t addr)
{	
	for(int i=0;i<size;i++){
		if(coremap_page[i].addr == addr){
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

}

unsigned int coremap_used_bytes(void)
{
	return numBytes;
}

void vm_bootstrap(void)
{

}

void vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("VM tried to do tlb shootdown?!\n");
}
