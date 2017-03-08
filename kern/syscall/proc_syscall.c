#include <types.h>
#include <lib.h>
#include <proc.h>
#include <copyinout.h>
#include <proc_syscall.h>
#include <syscall.h>
#include <current.h>
#include <thread.h>
#include <kern/errno.h>
#include <kern/wait.h>



//retval is a parameter passed by reference
int sys_fork(struct trapframe *tf, int *retval){

  kprintf("\n Fork sys call\n");
  struct proc *newProc;
  struct trapframe *trapframe = kmalloc(sizeof(trapframe));
  //void *trapframe;

  //(void) tf;
  // thread for the newly created process
  //struct thread *newthread;

  const char *name = "Newly created process!";

  memcpy(trapframe, tf, sizeof(trapframe));

  newProc = fork_proc_create(name);

  newProc->ppid=curproc->pid;

  kprintf("\n Just before fork \n");

  *retval=newProc->pid;

  // Unknown fourth arg - passing newproc id because - long int.
  thread_fork(name, newProc, (void*)enter_forked_process, trapframe, newProc->pid);

  kprintf("\n Just after fork \n");


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

  // Need to check exit status of the child before waiting on it.
  if(proctable[pid]->exit_status==1)
    return ESRCH;

  P(proctable[pid]->proc_sem);

  int check = copyout(&proctable[pid]->exit_code, (userptr_t)status, sizeof(int));

  if(check){}


  //&status=proctable[pid]->exit_code; //_MKWAIT_EXIT


  //status++;
  proc_destroy(proctable[pid]);

  *retval=pid;

  return 0;
}

int sys__exit(int exitcode){

      // Ensure not already exited

      // Who will check on the parent ?

      //Might need a KASSERT to make sure the process is not already exited
      curproc->exit_code=_MKWAIT_EXIT(exitcode);
      curproc->exit_status=1;
      V(curproc->proc_sem);

      thread_exit();
      //how to actually exit?

      return 0;
}

int sys_getpid(pid_t *retval){
  *retval=curproc->pid;
  return 0;
}

// int sys_execv(const char *program, char **args)
// {
//   size_t length;
//   char *program_kern; //might need to kmalloc

//   //First Copy Program into Kernel Memory, PATH_MAX is the maximum size of an instruction path
//   int check1=copyinstr((userptr_t)program, program_kern, PATH_MAX, &length); //might need to check error for all these


//   char **kernel_args=(char **)kmalloc(sizeof(char**));

//   //Copy address/pointers from User to Kernel Memory
//   int check2=copyin((userptr_t) args, kernel_rgs, sizeof(char **));

//   int i=0;
//   for(i=0;args[i]!=NULL;i++)
//   {
//     //Copy each value in user memory to kernel memory
//     kernel_args[i] = (char *)kmalloc(sizeof(char)*PATH_MAX);
//     int check3=copyinstr((userptr_t) args[i],kernel_args[i], PATH_MAX, &length);
//   }

//   kernel_args[i]=NULL;

//   //Do RunProgram activities


//   struct addrspace *as;
//   struct vnode *v;
//   vaddr_t entrypoint, stackptr;

//   int result;

//   /* Open the file. */
//   result = vfs_open(progname, O_RDONLY, 0, &v);
//   if (result) {

//     //Program has ended,might need to deallocate memory
//     return result;
//   }





//    We should be a new process.
//   KASSERT(proc_getas() == NULL);

//   /* Create a new address space. */
//   as = as_create();
//   if (as == NULL) {
//     vfs_close(v);
//     return ENOMEM;
//   }

//   /* Switch to it and activate it. */
//   proc_setas(as);
//   as_activate();

//   /* Load the executable. */
//   result = load_elf(v, &entrypoint);
//   if (result) {
//     //might need to deallocate memory
//     /* p_addrspace will go away when curproc is destroyed */
//     vfs_close(v);
//     return result;
//   }

//   /* Done with the file now. */
//   vfs_close(v);

//   result = as_define_stack(as, &stackptr);
//   if (result) {

//     //might need to deallocate space
//     /* p_addrspace will go away when curproc is destroyed */
//     return result;
//   }

//   //Have to copy arguments from Kernel Space to User Space and use those as parameters in enter_new_process





//   /* Warp to user mode. */
//   enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,
//         NULL /*userspace addr of environment*/,
//         stackptr, entrypoint);

//   /* enter_new_process does not return. */
//   panic("enter_new_process returned\n");
//   return EINVAL;

// }
