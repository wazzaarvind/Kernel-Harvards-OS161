#include <types.h>
#include <lib.h>
#include <proc.h>
#include <copyinout.h>
#include <proc_syscall.h>
#include <syscall.h>
#include <current.h>
#include <thread.h>
#include <kern/errno.h>
#include <vm.h>
#include <kern/wait.h>



//retval is a parameter passed by reference
int sys_fork(struct trapframe *tf, int *retval){

  //kprintf("\n Fork sys call\n");
  struct proc *newProc;
  struct trapframe *trapframe = kmalloc(sizeof(struct trapframe));
  //void *trapframe;

  //(void) tf;
  // thread for the newly created process
  //struct thread *newthread;

  const char *name = "Newly created process!";

  memcpy(trapframe, tf, sizeof(struct trapframe));
  //*trapframe = *tf;

  newProc = fork_proc_create(name);

  //newProc->ppid=curproc->pid;

  //kprintf("\n Just before fork \n");

  //kprintf("\n PID : %d\n",newProc->pid);
  //kprintf("\n PID parent: %d\n",curproc->pid);

  *retval=newProc->pid;

  // Unknown fourth arg - passing newproc id because - long int.
  thread_fork(name, newProc, (void*)enter_forked_process, trapframe, newProc->pid);

  //kprintf("Thread fork return %d",check);

  //kprintf("\n Just after fork \n");


  return 0;
};



int sys_waitpid(pid_t pid, int *status, int options, int *retval)
{
  //kprintf("\nWaitpid%d %d\n",pid,options);

  if (pid == curproc->ppid || pid == curproc->pid) 
    return ECHILD;

  if(pid<=0)
    return ESRCH;


  //if (pid > PID_MAX || pid < PID_MIN) {
    //return ESRCH;
  //}



  if(status==NULL)
    return EFAULT;

  if(options!=0)
    return EINVAL;


    if (proctable[pid]->ppid == curproc->ppid) {
      return ECHILD;
    }

 

  // Need to check exit status of the child before waiting on it.
  if(proctable[pid] == NULL)
    return ESRCH;

  if(proctable[pid]->ppid!=curproc->pid) //does parent wait on child?
    return ECHILD;

  //if(proctable[pid]->exit_status==1)
    //return ESRCH;





  if(proctable[pid]->exit_status!=1) { // If child has not exited already
  
      P(proctable[pid]->proc_sem);  // wwait for child
    }
  

  //P(proctable[pid]->proc_sem);

  if (status != NULL) {
    int check = copyout((const void *)&proctable[pid]->exit_code, (userptr_t)status, sizeof(int));
    if (check) {
      //proc_destroy(proctable[pid]);
      //proctable[pid] = NULL;
      return check;
    }
  }

  //if(check){}


  //&status=proctable[pid]->exit_code; //_MKWAIT_EXIT


  //status++;
  //proc_destroy(proctable[pid]);

  proctable[pid]=NULL;
  *retval=pid;

  return 0;
}

int sys__exit(int exitcode){

      //kprintf("\nEXIT %d\n",exitcode);
      KASSERT(curproc->exit_status!=1);
      // Ensure not already exited

      // Who will check on the parent ?


      //if(proctable[curproc->ppid]->exit_status==0) 
      //{
      //Might need a KASSERT to make sure the process is not already exited
      //if(exitcode!=0)
        //curproc->exit_code=_MKWAIT_SIG(exitcode);
      //else
      curproc->exit_code=_MKWAIT_EXIT(exitcode);
      curproc->exit_status=1;
      V(curproc->proc_sem);
      //}
      //else
        //proc_destroy(curproc);
      //V(proctable[curproc->pid]->proc_sem);

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


     /*int i=0;
     int arg_length=0,old_length=0;
     while(args[i]!=NULL)
     {
      arg_length=strlen(args[i]+1);
      old_length=strlen(args[i]+1);
      if(arg_length%4!=0)
        arg_length+=4-(arg_length%4);

      char *stack_copy=kmalloc(sizeof(arg_length));
      int j=0;
      for(j=0;j<arg_length;j++)
      {
        if(j<old_length)
          stack_copy[j]=args[i][j];
        else
          stack_copy[j]='\0';

      }
      stackptr-=arg_length;
      int check4=copyout((const void *)stack_copy,(userptr_t)stackptr,(size_t)arg_length);
      if(check4){

        }
        i++;
     }
      //if(argv[i]==NULL){
        stackptr-=4;
      //}

      int k=0;
      for(k=i-1;k>=0;k--)
      {
        stackptr = stackptr - sizeof(char*);
        int check5=copyout((const void *)(args + i),(userptr_t)stackptr,(sizeof(char *)));
      if(check5){

      }
      }
      //may need to do kfree's



      //Have to copy all pointers


     }*/


//   /* Warp to user mode. */
//   enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,
//         NULL /*userspace addr of environment*/,
//         stackptr, entrypoint);

//   /* enter_new_process does not return. */
//   panic("enter_new_process returned\n");
//   return EINVAL;

// }
