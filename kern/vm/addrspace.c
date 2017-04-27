/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <vm.h>
#include <addrspace.h>
#include <proc.h>
#include <spl.h>
#include <mips/tlb.h>
#include <uio.h>
#include <bitmap.h>


/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}

	//for(int i = 0; i < 5; i++){
			//as->sgmt[i] = NULL;
	//}
	as->sgmt = NULL;

	as->first_page = NULL;
	as->last_page = NULL;


	// Make address 0.

	/*
	 * Initialize as needed.
	 */

	as->heap_top = 0;
	as->heap_bottom = 0;

	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	//kprintf("\nascopy");
	//kprintf("\nHi4");

	struct addrspace *newas;

	newas = as_create();

	if (newas==NULL) {
		return ENOMEM;
	}

	// check if old is null.
	if(old == NULL){
		return ENOMEM;
	}


	newas->heap_top = old->heap_top;
	newas->heap_bottom = old->heap_bottom;
	newas->stack_top = old->stack_top;
	newas->stack_bottom = old->stack_bottom;

	// From as_create
	newas->sgmt = NULL;
	newas->first_page = NULL;
	newas->last_page = NULL;

	//Have to copy all segments and pages
	struct segment *oldPtr = old->sgmt;
	struct segment *newPtr = NULL;

	// Copying segments one by one.
	while(oldPtr != NULL){
		newPtr = kmalloc(sizeof(struct segment));

		if(newPtr == NULL){
			return ENOMEM;
		}

		newPtr->start = oldPtr->start;
		newPtr->end = oldPtr->end;
		newPtr->npages = oldPtr->npages;
		newPtr->next = NULL;

		if(newas->sgmt == NULL){
			newas->sgmt = newPtr;

		} else {

			struct segment *temp = newas->sgmt;

			while(temp->next != NULL){
					temp = temp->next;
			}

			temp->next = newPtr;
		}

		oldPtr = oldPtr->next;
	}

	// Next copying the PTE and the pages!
	struct page_table *oldPte = old->first_page;
	struct page_table *newPte = NULL;

	while(oldPte != NULL){


		newPte = kmalloc(sizeof(struct page_table));

		if(newPte == NULL){
			return ENOMEM;
		}

		newPte->pt_lock = lock_create("Pte lock");

		if(swap_or_not == SWAP_ENABLED)
		{
			lock_acquire(newPte->pt_lock);

			lock_acquire(oldPte->pt_lock);
		}

		newPte->paddr = alloc_upages();

		if(newPte->paddr == 0){
			return ENOMEM;
		}

		int index = 0;

		index = newPte->paddr/PAGE_SIZE;

		if(coremap[index].state == DESTORY){
			lock_destroy(coremap[index].page->pt_lock);
			kfree(coremap[index].page);
		}

		coremap[index].state = RECENTLY_USED;
		coremap[index].page = newPte;


		newPte->vaddr = oldPte->vaddr;
		newPte->mem_or_disk = oldPte->mem_or_disk;
		newPte->next = NULL;

		if(oldPte->mem_or_disk == IN_DISK)
		{
			newPte->bitmapIndex = oldPte->bitmapIndex;
		  swap_in_disk(newPte);

		}
		else
		{ // TODO : Check!
			memmove((void *)(MIPS_KSEG0+newPte->paddr),(const void *)(MIPS_KSEG0+oldPte->paddr),PAGE_SIZE);
		}


		if(newas->last_page == NULL){
			newas->last_page = newPte;
			newas->first_page = newPte;
		} else {
			newas->last_page->next = newPte;
			newas->last_page = newas->last_page->next;
		}

		if(swap_or_not == SWAP_ENABLED)
		{
			lock_release(newPte->pt_lock);
			lock_release(oldPte->pt_lock);
		}

		// Now copy over the contents of the memory

		oldPte = oldPte->next;
	}

	/*
	 * Write this.
	 */

	*ret = newas;

	return 0;
}

void
as_destroy(struct addrspace *as)
{
	// (void) as;
	// return;
	// kprintf("\nasdes");
	as->heap_top = 0;
	as->heap_bottom = 0;

	as->stack_bottom = 0;
	as->stack_top = 0;
	/*
	 * Clean up as needed.
	 */
	 while(as->sgmt!=NULL)
	 {
	 	struct segment *segdes = as->sgmt;
		as->sgmt=as->sgmt->next;
	 	kfree(segdes);
	 }

	 // First take away all the physical pages allocated :
	 struct page_table *pagedes = as->first_page;

	 while(pagedes != NULL)
	 {
	 	if(swap_or_not == SWAP_DISABLED)
	 	{
	 		free_upage(pagedes->paddr);
	 		pagedes = pagedes->next;
	 	}
	 	// do we need locks?
	 	else{
	 	     lock_acquire(pagedes->pt_lock);
				 if(pagedes->mem_or_disk == IN_MEMORY){
					 	int index = pagedes->paddr/PAGE_SIZE;
						if(coremap[index].state == VICTIM){
							coremap[index].state = DESTORY;
						} else {
			 				free_upage(pagedes->paddr);
						}
						lock_release(pagedes->pt_lock);
				  } else {
						lock_release(pagedes->pt_lock);
						lock_acquire(bitmap_lock);
						bitmap_unmark(swapTable,(unsigned) pagedes->bitmapIndex);
						lock_release(bitmap_lock);
					}


				pagedes = pagedes->next;
			}
	 }

	 // Next destroy the PTE.
	 while(as->first_page!=NULL)
	 {
	 	pagedes = as->first_page;
		if(pagedes->mem_or_disk == IN_MEMORY){
			int index = pagedes->paddr/PAGE_SIZE;
			if(coremap[index].state != DESTORY){
				lock_destroy(pagedes->pt_lock);
				as->first_page = as->first_page->next;
				kfree(pagedes);
			 }
		} else {
			lock_destroy(pagedes->pt_lock);
			as->first_page = as->first_page->next;
			kfree(pagedes);
		}
	}

	kfree(as);
}

void
as_activate(void)
{
	int i, spl;
	struct addrspace *as;

	as = proc_getas();
	if (as == NULL) {
		return;
	}

	/* Disable interrupts on this CPU while frobbing the TLB. */
	spl = splhigh();

	for (i=0; i<NUM_TLB; i++) {
		tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
	}

	splx(spl);
}

void
as_deactivate(void)
{


	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t memsize,
		 int readable, int writeable, int executable)
{
	/*
	 * Write this.
	 */

	 size_t npages;

	 /* Align the region. First, the base... */
	memsize += vaddr & ~(vaddr_t)PAGE_FRAME;
	vaddr &= PAGE_FRAME;

	/* ...and now the length. */
	memsize = (memsize + PAGE_SIZE - 1) & PAGE_FRAME;

	npages = memsize / PAGE_SIZE;

	 // Not implementing permissions for now.
	(void)readable;
	(void)writeable;
	(void)executable;


	vaddr_t last_vaddr = vaddr + (npages * PAGE_SIZE);

	struct segment *curseg = kmalloc(sizeof(struct segment));

	curseg->start = vaddr;
	curseg->end = last_vaddr;
	curseg->npages = npages;
	curseg->next = NULL;

	if(as->sgmt == NULL){
		// This is the first segment.
		as->sgmt = curseg;
		as->sgmt->next = NULL;

	} else {

		struct segment *temp = as->sgmt;//= *((struct segment *)as->sgmt);

		while(temp->next != NULL){
				//temp = *((struct segment *)temp.next);
				temp = temp->next;
		}

		temp->next = curseg;
	}



	as->heap_top = last_vaddr;
	as->heap_bottom = last_vaddr;

	//ben says stack not required
	//put heap top and bottom and stack top and bottom in here
	 return 0;
}

int
as_prepare_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_complete_load(struct addrspace *as)
{
	/*
	 * Write this.
	 */

	(void)as;
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/*
	 * Write this.
	 */

	as->stack_bottom = USERSTACK;
	as->stack_top = USERSTACK - (1024 * PAGE_SIZE);

	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;

	return 0;
}
