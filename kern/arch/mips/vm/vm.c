#include <vm.h>
#include <mainbus.h>

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


vaddr_t alloc_kpages(unsigned npages)
{
	for(int i=0;i<size;i++){
		if(coremap_page[i].available!=1)
			continue;

	}
	
}

void free_kpages(vaddr_t addr)
{

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
