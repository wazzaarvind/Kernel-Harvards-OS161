#include <types.h>
#include <lib.h>
#include <proc.h>
#include <copyinout.h>
#include <proc_syscall.h>
#include <syscall.h>
#include <thread.h>




pid_t sys_fork(struct trapframe *tf, int *retval){

  struct proc *newProc;
  //struct trapframe *trapframe = kmalloc(sizeof(trapframe));
  void *trapframe;

  // thread for the newly created process
  //struct thread *newthread;

  const char *name = "Newly created process!";

  memcpy(trapframe, tf, sizeof(tf));

  newProc = fork_proc_create(name);

  // Unknown fourth arg - passing newproc id because - long int.
  thread_fork(name, newProc, enter_forked_process, (void *)trapframe, newProc->pid);

  *retval=newProc->pid;

  return 0;
};
