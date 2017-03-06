#include <types.h>
#include <lib.h>
#include <proc.h>
#include <copyinout.h>
#include <proc_syscall.h>
#include <syscall.h>
#include <current.h>
#include <thread.h>
#include <kern/errno.h>



//retval is a parameter passed by reference
int sys_fork(struct trapframe *tf, int *retval){

  kprintf("Fork starts");

  struct proc *newProc;
  struct trapframe *trapframe = kmalloc(sizeof(trapframe));
  //void *trapframe;

  // thread for the newly created process
  //struct thread *newthread;

  const char *name = "Newly created process!";

  memcpy(trapframe, tf, sizeof(trapframe));

  newProc = fork_proc_create(name);

  newProc->ppid=curproc->pid;
  *retval=newProc->pid;

  // Unknown fourth arg - passing newproc id because - long int.
  thread_fork(name, newProc, enter_forked_process, trapframe, newProc->pid);


  return 0;
};



int sys_waitpid(pid_t pid, int *status, int options, int *retval)
{
  if(pid<=0)
    return ESRCH;

  if(status==NULL)
    return EFAULT;

  if(options!=0)
    return EINVAL;

  if(proctable[pid]->ppid!=curproc->pid) //does parent wait on child?
    return ECHILD;

  if(proctable[pid]->exit_status==1)
    return ESRCH;

  P(proctable[pid]->proc_sem);

  status=proctable[pid]->exit_code; //_MKWAIT_EXIT

  proc_destroy(proctable[pid]);

  *retval=pid;

  return 0;
}

int sys__exit(int exitcode){

      //Might need a KASSERT to make sure the process is not already exited
      curproc->exit_code=_MKWAIT_EXIT(exitcode);
      curproc->exit_status=1;
      V(curproc->proc_sem);

      //how to actually exit?

      return 0;
}

int sys_getpid(pid_t *retval){
  *retval=curproc->pid;
  return 0;
}
