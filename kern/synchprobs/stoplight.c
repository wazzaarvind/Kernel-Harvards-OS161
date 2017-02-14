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
 * Driver code is in kern/tests/synchprobs.c We will replace that file. This
 * file is yours to modify as you see fit.
 *
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is, of
 * course, stable under rotation)
 *
 *   |0 |
 * -     --
 *    01  1
 * 3  32
 * --    --
 *   | 2|
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X first.
 * The lockantics of the problem are that once a car enters any quadrant it has
 * to be somewhere in the intersection until it call leaveIntersection(),
 * which it should call while in the final quadrant.
 *
 * As an example, let's say a car approaches the intersection and needs to
 * pass through quadrants 0, 3 and 2. Once you call inQuadrant(0), the car is
 * considered in quadrant 0 until you call inQuadrant(3). After you call
 * inQuadrant(2), the car is considered in quadrant 2 until you call
 * leaveIntersection().
 *
 * You will probably want to write some helper functions to assist with the
 * mappings. Modular arithmetic can help, e.g. a car passing straight through
 * the intersection entering from direction X will leave to direction (X + 2)
 * % 4 and pass through quadrants X and (X + 3) % 4.  Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in synchprobs.c to record their progress.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * Called by the driver during initialization.
 */
struct lock *sl_lock[4];
struct lock *master_lock;

struct semaphore *sl_sem[4];
struct semaphore *master_sem;
void
stoplight_init() {
	master_sem=sem_create("Master Semaphore",1);
	sl_sem[0]=sem_create("Q0 Semaphore",1);
	sl_sem[1]=sem_create("Q1 Semaphore",1);
	sl_sem[2]=sem_create("Q2 Semaphore",1);
	sl_sem[3]=sem_create("Q3 Semaphore",1);
	master_lock=lock_create("Master lock");
	sl_lock[0]=lock_create("Q0 lock");
	sl_lock[1]=lock_create("Q1 lock");
	sl_lock[2]=lock_create("Q2 lock");
	sl_lock[3]=lock_create("Q3 lock");
	return;
}

/*
 * Called by the driver during teardown.
 */

void stoplight_cleanup() {
	for(int i=0;i<4;i++)
	{	lock_destroy(sl_lock[i]);
		sem_destroy(sl_sem[i]);
	}
	lock_destroy(master_lock);
	sem_destroy(master_sem);
	return;
}

void
turnright(uint32_t direction, uint32_t index)
{
	P(sl_lock[direction]);

	//lock_acquire(master_lock);
	//lock_acquire(sl_lock[direction]);
	inQuadrant(direction,index);
	leaveIntersection(index);
	V(sl_lock[direction]);
	//lock_release(sl_lock[direction]);
	//lock_release(master_lock);
	/*
	 * Implement this function.
	 */
	return;
}
void
gostraight(uint32_t direction, uint32_t index)
{
	(void)direction;
	(void)index;
	P(master_sem);
	P(sl_lock[direction]);
	P(sl_lock[(direction+3)%4]);


	//lock_acquire(master_lock);
	//lock_acquire(sl_lock[direction]);
	//lock_acquire(sl_lock[(direction+3)%4]);
	inQuadrant(direction,index);
	inQuadrant((direction+3)%4,index);
	leaveIntersection(index);

        V(sl_lock[direction]);
        V(sl_lock[(direction+3)%4]);
	V(master_sem);
	//lock_release(sl_lock[direction]);
	//lock_release(sl_lock[(direction+3)%4]);
	//lock_release(master_lock);
	/*
	 * Implement this function.
	 */
	return;
}
void
turnleft(uint32_t direction, uint32_t index)
{
	P(master_sem);
	//lock_acquire(master_lock);
	(void)direction;
	(void)index;

	P(sl_lock[direction]);
        P(sl_lock[(direction+3)%4]);
	P(sl_lock[(direction+2)%4]);

	//lock_acquire(sl_lock[direction]);
	//lock_acquire(sl_lock[(direction+3)%4]);
	//lock_acquire(sl_lock[(direction+2)%4]);


	inQuadrant(direction,index);
        inQuadrant((direction+3)%4,index);
        inQuadrant((direction+2)%4,index);
	leaveIntersection(index);

	V(sl_lock[direction]);
        V(sl_lock[(direction+3)%4]);
        V(sl_lock[(direction+2)%4]);




	//lock_release(sl_lock[direction]);
	//lock_release(sl_lock[(direction+3)%4]);
        //lock_release(sl_lock[(direction+2)%4]);

	V(master_sem);
	//lock_release(master_lock);
	/*
	 * Implement this function.
	 */
	return;
}
