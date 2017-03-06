#include <types.h>
#include <lib.h>
#include <proc.h>
#include <copyinout.h>
#include <proc_syscall.h>
#include <syscall.h>
#include <current.h>
#include <thread.h>



//retval is a parameter passed by reference
int sys_fork(struct trapframe *tf, int *retval){

  struct proc *newProc;
  //struct trapframe *trapframe = kmalloc(sizeof(trapframe));
  void *trapframe=NULL;

  // thread for the newly created process
  //struct thread *newthread;

  const char *name = "Newly created process!";

  memcpy(trapframe, tf, sizeof(tf));

  newProc = fork_proc_create(name);

  newProc->ppid=curproc->pid;
  *retval=newProc->pid;

  // Unknown fourth arg - passing newproc id because - long int.
  thread_fork(name, newProc, enter_forked_process, (void *)trapframe, newProc->pid);



  return 0;
};
