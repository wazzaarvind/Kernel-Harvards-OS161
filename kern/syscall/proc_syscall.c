#include <proc_syscall.h>
#include <proc.h>
#include <lib.h>
#include <syscall.h>
#include <copyinout.h>
#include <thread.h>



pid_t sys_fork(struct trapframe *tf){

  struct proc *newProc;
  struct trapframe *trapframe = kmalloc(sizeof(trapframe));

  // thread for the newly created process
  //struct thread *newthread;

  const char *name = "Newly created process!";

  memcpy(tf, trapframe, sizeof(trapframe));

  newProc = fork_proc_create(name);

  // Unknown fourth arg - passing newproc id because - long int.
  thread_fork(name, newProc, enter_forked_process, trapframe, newProc->pid);

  return newProc->pid;
};
