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

/*
 * Synchronization primitives.
 * The specifications of the functions are in synch.h.
 */

#include <types.h>
#include <lib.h>
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <current.h>
#include <synch.h>

////////////////////////////////////////////////////////////
//
// Semaphore.

struct semaphore *
sem_create(const char *name, unsigned initial_count)
{
	struct semaphore *sem;

	sem = kmalloc(sizeof(*sem));
	if (sem == NULL) {
		return NULL;
	}

	sem->sem_name = kstrdup(name);
	if (sem->sem_name == NULL) {
		kfree(sem);
		return NULL;
	}

	sem->sem_wchan = wchan_create(sem->sem_name);
	if (sem->sem_wchan == NULL) {
		kfree(sem->sem_name);
		kfree(sem);
		return NULL;
	}

	spinlock_init(&sem->sem_lock);
	sem->sem_count = initial_count;
	return sem;
}

void
sem_destroy(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/* wchan_cleanup will assert if anyone's waiting on it */
	spinlock_cleanup(&sem->sem_lock);
	wchan_destroy(sem->sem_wchan);
	kfree(sem->sem_name);
	kfree(sem);
}

void
P(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	/*
	 * May not block in an interrupt handler.
	 *
	 * For robustness, always check, even if we can actually
	 * complete the P without blocking.
	 */
	KASSERT(curthread->t_in_interrupt == false);

	/* Use the semaphore spinlock to protect the wchan as well. */
	spinlock_acquire(&sem->sem_lock);
	while (sem->sem_count == 0) {
		/*
		 *
		 * Note that we don't maintain strict FIFO ordering of
		 * threads going through the semaphore; that is, we
		 * might "get" it on the first try even if other
		 * threads are waiting. Apparently according to some
		 * textbooks semaphores must for some reason have
		 * strict ordering. Too bad. :-)
		 *
		 * Exercise: how would you implement strict FIFO
		 * ordering?
		 */
		wchan_sleep(sem->sem_wchan, &sem->sem_lock);
	}
	KASSERT(sem->sem_count > 0);
	sem->sem_count--;
	spinlock_release(&sem->sem_lock);
}

void
V(struct semaphore *sem)
{
	KASSERT(sem != NULL);

	spinlock_acquire(&sem->sem_lock);

	sem->sem_count++;
	KASSERT(sem->sem_count > 0);
	wchan_wakeone(sem->sem_wchan, &sem->sem_lock);

	spinlock_release(&sem->sem_lock);
}

////////////////////////////////////////////////////////////
//
// Lock.

struct lock *
lock_create(const char *name)
{
	struct lock *lock;

	lock = kmalloc(sizeof(*lock));
	if (lock == NULL) {
		return NULL;
	}

	lock->lk_name = kstrdup(name);

	if (lock->lk_name == NULL) {
		kfree(lock);
		return NULL;
	}

	HANGMAN_LOCKABLEINIT(&lock->lk_hangman, lock->lk_name);

	// add stuff here as needed
	//Arvind edit
	KASSERT(lock != NULL);

	lock->lk_wchan = wchan_create(lock->lk_name);
	if(lock->lk_wchan == NULL){
		kfree(lock->lk_name);
		kfree(lock);
		return NULL;
	} //why? - Arvind

	spinlock_init(&lock->lk_spinlock);
	lock->lk_thread = NULL;
	lock->state = 0;
	return lock;
}

void
lock_destroy(struct lock *lock)
{
	KASSERT(lock != NULL);

	// add stuff here as needed
	KASSERT(lock->state==0); //Arvind edit
	KASSERT(lock->lk_thread==NULL);
	//Arvind edit
	spinlock_cleanup(&lock->lk_spinlock);
	wchan_destroy(lock->lk_wchan);
	kfree(lock->lk_name);
	kfree(lock);
}

void
lock_acquire(struct lock *lock)
{
	/* Call this (atomically) before waiting for a lock */
	//HANGMAN_WAIT(&curthread->t_hangman, &lock->lk_hangman);

	// Write this
	//check waitchannel
	//check current thread status
	//then acquire lock
	//check P()

	// Achuth edit

	// Check if the lock that is being passed is not null.
	//kprintf("%s", lock->lk_thread->t_name);
	KASSERT(lock != NULL);
	//KASSERT(lock->lk_thread != NULL);

  // Check if the thread is not in an interrupt
	KASSERT(curthread->t_in_interrupt == false);

  // Use spin lock to protect the wchan. - HOW?
	spinlock_acquire(&lock->lk_spinlock);

  // if(lock_do_i_hold(lock)){
	// 	 return ;
	// }

	while(lock->state == 1){

		wchan_sleep(lock->lk_wchan, &lock->lk_spinlock);
	}
	KASSERT(lock->state == 0);
	lock->lk_thread = curthread;
	lock->state = 1;
	spinlock_release(&lock->lk_spinlock);
	/* Call this (atomically) once the lock is acquired */
	//HANGMAN_ACQUIRE(&curthread->t_hangman, &lock->lk_hangman);
}

void
lock_release(struct lock *lock)
{
	/* Call this (atomically) when the lock is released */
	//HANGMAN_RELEASE(&curthread->t_hangman, &lock->lk_hangman);

	// Write this
	//check if you have the lock
	//if yes, release the lock
	//check V()

	KASSERT(lock != NULL);
	//if(lock_do_i_hold(lock)==true)
	//Arvind edit
	//KASSERT(lock_do_i_hold(lock)==true);
	spinlock_acquire(&lock->lk_spinlock);
	KASSERT(lock->state == 1);
	lock->lk_thread=NULL; //Arvind edit
	lock->state = 0;
	KASSERT(lock->state == 0);
	wchan_wakeone(lock->lk_wchan, &lock->lk_spinlock);
	spinlock_release(&lock->lk_spinlock);
}

bool
lock_do_i_hold(struct lock *lock)
{
	return (lock->lk_thread == curthread);
}

////////////////////////////////////////////////////////////
//
// CV


struct cv *
cv_create(const char *name)
{
	struct cv *cv;

	cv = kmalloc(sizeof(*cv));
	if (cv == NULL) {
		return NULL;
	}

	cv->cv_name = kstrdup(name);
	if (cv->cv_name==NULL) {
		kfree(cv);
		return NULL;
	}

	// add stuff here as needed
	KASSERT(cv!=NULL);
	cv->cv_wchan=wchan_create(cv->cv_name);
	if(cv->cv_wchan == NULL){
                kfree(cv->cv_name);
                kfree(cv);
                return NULL;
	}
	spinlock_init(&cv->cv_spinlock);

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	KASSERT(cv!= NULL);
	//KASSERT(lock_do_i_hold(lock));
	// add stuff here as needed
	spinlock_cleanup(&cv->cv_spinlock);
	wchan_destroy(cv->cv_wchan);
	kfree(cv->cv_name);
	kfree(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	KASSERT(cv!=NULL);
	KASSERT(lock!=NULL);
	//Arvind edit
	//1.Release Lock 2. Sleep 3. Wake up and reacquire lock
	// Write this
	KASSERT(lock_do_i_hold(lock));
	spinlock_acquire(&cv->cv_spinlock);
	lock_release(lock); //spinlock?
	wchan_sleep(cv->cv_wchan, &cv->cv_spinlock);
	spinlock_release(&cv->cv_spinlock);
	lock_acquire(lock); //spinlock?
	//(void)cv;    // suppress warning until code gets written
	//(void)lock;  // suppress warning until code gets written
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	KASSERT(cv!=NULL);
	KASSERT(lock!=NULL);
	KASSERT(lock_do_i_hold(lock));
	//spinlock?
	spinlock_acquire(&cv->cv_spinlock);
	wchan_wakeone(cv->cv_wchan, &cv->cv_spinlock);
	spinlock_release(&cv->cv_spinlock);
	// Wake up one thread sleeping on this CV
	// Write this
	//(void)cv;    // suppress warning until code gets written
	//(void)lock;  // suppress warning until code gets written
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	KASSERT(cv!=NULL);
	KASSERT(lock!=NULL);
	KASSERT(lock_do_i_hold(lock));
	//spinlock?
	spinlock_acquire(&cv->cv_spinlock);
	wchan_wakeall(cv->cv_wchan, &cv->cv_spinlock);
	spinlock_release(&cv->cv_spinlock);

}

//Arvind edit
//Read-Write Locks section

// CITATION https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock

struct rwlock *
rwlock_create(const char *rw_name)
{
	//Add stuff as needed
	struct rwlock *rwlock;

	rwlock = kmalloc(sizeof(*rwlock));

	if(rwlock == NULL){
		return NULL;
	}

	rwlock->rwlock_name = kstrdup(rw_name);

	if(rwlock->rwlock_name== NULL){
		kfree(rwlock);
		return NULL;
	}

	KASSERT(rwlock != NULL);
	rwlock->rwlock_sem=sem_create(rwlock->rwlock_name,40);
	rwlock->glock_sem=sem_create(rwlock->rwlock_name,0);
	rwlock->rwlock_cv=cv_create("RW CV");
	//rwlock->rlock_sem=sem_create("Read Lock",0);
	//rwlock->wlock_sem=sem_create("Write Lock",0);
	//spinlock_init(&rwlock->rw_spinlock);
	rwlock->rwlock=lock_create("RWLock");
	rwlock->readLock = lock_create("rLock");
	rwlock->writeLock = lock_create("wLock");

	return rwlock;
}

void
rwlock_destroy(struct rwlock *rw_lock)
{
	// //Add stuff as needed
	KASSERT(rw_lock!=NULL);
	sem_destroy(rw_lock->rwlock_sem);
	sem_destroy(rw_lock->glock_sem);
	cv_destroy(rw_lock->rwlock_cv);
	lock_destroy(rw_lock->rwlock);
	lock_destroy(rw_lock->readLock);
	lock_destroy(rw_lock->writeLock);
	kfree(rw_lock->rwlock_name);
	kfree(rw_lock);
}

void
rwlock_acquire_read(struct rwlock *rw_lock)
{
	KASSERT(rw_lock!=NULL);
	lock_acquire(rw_lock->rwlock);
	P(rw_lock->rwlock_sem);

	lock_release(rw_lock->rwlock);
	//rw_lock->readCount++;
	
}

void
rwlock_release_read(struct rwlock *rw_lock)
{
	KASSERT(rw_lock!=NULL);
	KASSERT(rw_lock->rwlock_sem->sem_count<40);
	V(rw_lock->rwlock_sem);

}

void
rwlock_acquire_write(struct rwlock *rw_lock)
{
	KASSERT(rw_lock!=NULL);
	int i=0;
	lock_acquire(rw_lock->rwlock);
	
	while(i<40)
		{
			P(rw_lock->rwlock_sem);
			i++;
		}
	lock_release(rw_lock->rwlock);


}

void
rwlock_release_write(struct rwlock *rw_lock)
{
	KASSERT(rw_lock!=NULL);
	int i=0;
	KASSERT(rw_lock->rwlock_sem->sem_count==0);
	while(i<40)
	{
		V(rw_lock->rwlock_sem);
		i++;
	}



}
