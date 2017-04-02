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

void vm_initialise{
int size=mainbus_ramsize();
size=size/4096;
coremap_page[size];

	for(int i=0;i<size;i++){
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

	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}

	while(npages>0)
	{
		coremap_page[i++].available=0;
		npages--;
	}

	return PADDR_TO_KVADDR(pa);

}

void free_kpages(vaddr_t addr)
{
	coremap_page[i].available=1;
		coremap_page[i].chunk_size=0;
		coremap_page[i].owner=-1;
		coremap_page[i].state=0;

}

unsigned int coremap_used_bytes(void)
{

}

void vm_bootstrap(void)
{

}

void vm_tlbshootdown(const struct tlbshootdown *tlbs)
{

}
