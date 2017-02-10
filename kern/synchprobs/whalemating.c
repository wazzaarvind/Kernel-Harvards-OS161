/*
 * Copyright (c) 2001, 2002, 2009
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

/*
 * Driver code is in kern/tests/synchprobs.c We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * Called by the driver during initialization.
 */

struct semaphore *wm_male_semaphore;
struct semaphore *wm_female_semaphore;

void whalemating_init() {
	KASSERT(wm_male_semaphore==NULL);
	KASSERT(wm_female_semaphore==NULL);
	wm_male_semaphore=sem_create("WM MALE SEMAPHORE",0);
	wm_female_semaphore=sem_create("WM FEMALE SEMAPHORE",0);	
	return;
}

/*
 * Called by the driver during teardown.
 */

void
whalemating_cleanup() {
	KASSERT(wm_male_semaphore!=NULL);
	KASSERT(wm_female_semaphore!=NULL);
	sem_destroy(wm_male_semaphore);
	sem_destroy(wm_female_semaphore);
	return;
}

void
male(uint32_t index)
{
	KASSERT(wm_male_semaphore!=NULL);
	(void)index;
	male_start(index);
	P(wm_male_semaphore);
	male_end(index);
	/*
	 * Implement this function by calling male_start and male_end when
	 * appropriate.
	 */
	return;
}

void
female(uint32_t index)
{
	KASSERT(wm_female_semaphore!=NULL);
	(void)index;
	female_start(index);
	P(wm_female_semaphore);
        //Mechanism to hold
        female_end(index);
	/*
	 * Implement this function by calling female_start and female_end when
	 * appropriate.
	 */
	return;
}

void
matchmaker(uint32_t index)
{
	KASSERT(wm_male_semaphore!=NULL);
	KASSERT(wm_female_semaphore!=NULL);
	(void)index;
	matchmaker_start(index);
	V(wm_male_semaphore);
	V(wm_female_semaphore);
	//Start one male and one female
	matchmaker_end(index);
	/*
	 * Implement this function by calling matchmaker_start and matchmaker_end
	 * when appropriate.
	 */
	return;
}
