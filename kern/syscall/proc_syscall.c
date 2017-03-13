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
#include <addrspace.h>
#include <kern/fcntl.h>
#include <vfs.h>
#include <limits.h>

//int counter=0;



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

  newProc = fork_proc_create(name);

  if(newProc == NULL){
    return ENOMEM;
  }

  memcpy(trapframe, tf, sizeof(struct trapframe));
  //*trapframe = *tf;

  //newProc->ppid=curproc->pid;

  //kprintf("\n Just before fork \n");

  //kprintf("\n PID : %d\n",newProc->pid);
  //kprintf("\n PID parent: %d\n",curproc->pid);

  *retval=newProc->pid;

  // Unknown fourth arg - passing newproc id because - long int.
  int check = thread_fork(name, newProc, (void*)enter_forked_process, trapframe, newProc->pid);

  if(check != 0){
    return ENOMEM;
  }

  //kprintf("Thread fork return %d",check);

  //kprintf("\n Just after fork \n");


  return 0;
};



int sys_waitpid(pid_t pid, int *status, int options, int *retval)
{
  //kprintf("\nWaitpid%d %d\n",pid,options);


  //kprintf("Ptr is %p",status);
  if (pid == curproc->ppid || pid == curproc->pid)
    return ECHILD;

  if(pid<=0)
    return ESRCH;



  if (pid > PID_MAX)
    return ESRCH;
  //}




  if(options!=0)
    return EINVAL;






  // Need to check exit status of the child before waiting on it.
  if(proctable[pid] == NULL)
    return ESRCH;

  if(proctable[pid]->ppid!=curproc->pid) //does parent wait on child?
    return ECHILD;

  //if(proctable[pid]->exit_status==1)
    //return ESRCH;
  if (proctable[pid]->ppid == pid) {
      return ECHILD;
    }



  if(status==NULL)
    return 0;

  if(status==(void *)0x40000000||status==(void *)0x80000000)
    return EFAULT;
//   if(proctable[pid]->exit_status!=1) { // If child has not exited already

      P(proctable[pid]->proc_sem);  // wwait for child
//    }


  //P(proctable[pid]->proc_sem);

  if(status != NULL)
    copyout((const void *)&proctable[pid]->exit_code, (userptr_t)status, sizeof(int));


    //status = &proctable[pid]->exit_code;
    // if (check) {
    //   //proc_destroy(proctable[pid]);
    //   //proctable[pid] = NULL;
    //   return check;
    // }
  //}

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
      //curproc->exit_status=1;
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

int sys_execv(const char *progname, char **args){
  //kprintf("\nprograme %s args %s\n",progname,args[0]);
  //kprintf("Args is %p",args[0]);




  if(progname==NULL||args==NULL||progname==(void *)0x40000000||progname==(void *)0x80000000||args ==(void*)0x40000000||args==(void *)0x80000000||args>=(char**)0x80000000||progname>=(char *)0x80000000)
    return EFAULT;


  //int no_of_elements;
  int u=0;
  for(u=0;args[u]!=NULL;u++)
  {

    if((args[u]==(void*)0x400000000)||(args[u]==(char*)0x80000000))
    {
      //kprintf("\nhi\n");
      //kfree(kernel_args);
      return EFAULT;
    }
  }

  //(void) **args;
  size_t length;
  char *program_kern=(char *)kmalloc(sizeof(char)*PATH_MAX); //might need to kmalloc

  if(program_kern==NULL)
    return ENOMEM;
  //casting the malloc to the right type, might not be required
  // program_kern=(char *)kmalloc(sizeof(char)*PATH_MAX);


   //First Copy Program into Kernel Memory, PATH_MAX is the maximum size of an instruction path
   int check1=copyinstr((userptr_t)progname, program_kern, PATH_MAX , &length); //might need to check error for all these
   if(check1)
   {
      kfree(program_kern);
      return check1;
   }

   if (length < 2 || length>PATH_MAX) {
    kfree(program_kern);
    return EINVAL;
  }

   //if (strcmp(progname,"") == 0)
    //return EISDIR;
    //kprintf("\nProgram Name is %s\n",program_kern);

   char **kernel_args=(char **)kmalloc(sizeof(char**)*u);
   if(kernel_args==NULL) {
     kfree(kernel_args);
     kfree(program_kern);
     return ENOMEM;
   }

   //Copy address/pointers from User to Kernel Memory
  int check2=copyin((userptr_t) args, kernel_args, sizeof(char **));
  if(check2)
   {
      kfree(kernel_args);
      kfree(program_kern);
      return EFAULT;

   }
   int i=0;
  for(i=0;args[i]!=NULL;i++)
  {

  //Copy each value in user memory to kernel memory
    kernel_args[i] = (char *)kmalloc(sizeof(char)*strlen(args[i])+1); //might need dummy operation for size //length
    if(kernel_args[i] == NULL){
      return ENOMEM;
    }
    //kprintf("args is %s",args[i]);
    int check3 = copyinstr((userptr_t) args[i],kernel_args[i], strlen(args[i])+1, &length); //strlen(kernel_args[i])+1 //ARG_MAX
    if(check3){
      // Deallocate memory for kernel_args[i]
      // for(int l = 0; args[l]!= NULL; l++){
      //
      // }
      kfree(kernel_args);
      kfree(program_kern);
      return ENOMEM;
    }
  }

  int argc=i;
  //kprintf("\nArgc is %d\n",argc);

  if(argc > ARG_MAX){
    kfree(kernel_args);
    kfree(program_kern);
    return ENOMEM;
  }


  kernel_args[i]=NULL;
  //for(i=0;kernel_args[i]!=NULL;i++)
    //kprintf("\nKergnel args is %s\n",kernel_args[i]);
  //userptr_t *copyoutargs=NULL;

  //struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;

	/* Open the file. */
	result = vfs_open(program_kern, O_RDONLY, 0, &v);

  kfree(program_kern);

	if (result) {
      kfree(kernel_args);
		  return result;
	}

  //New address space needs to be created
  //as_deactivate();
  as_destroy(curproc->p_addrspace);
  curproc->p_addrspace = NULL;

	/* Create a new address space. */
	curproc->p_addrspace = as_create();
	if (curproc->p_addrspace == NULL) {
    kfree(kernel_args);
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	proc_setas(curproc->p_addrspace);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
    kfree(kernel_args);
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curproc->p_addrspace, &stackptr);
	if (result) {
    kfree(kernel_args);
		return result;
	}

  i=0;
  int counter=0;
     int arg_length=0,old_length=0;
     while(kernel_args[i]!=NULL)
     {
      arg_length=strlen(kernel_args[i])+1; //for '\0'
      //kprintf("\nCurrent arg length is %d\n",arg_length);
      old_length=arg_length;
      if(arg_length%4!=0)
        arg_length+=4-(arg_length%4); //padding areas
      //kprintf("\nNew arg length is %d\n",arg_length);
      counter+=arg_length;

      char *stack_copy=(char *)kmalloc(sizeof(char)*arg_length);
      //kprintf("\nStack Copy is %d\n",strlen(stack_copy));
      //stack_copy=kstrdup(kernel_args[i]);
      int j=0;
      for(j=0;j<old_length;j++)
      {
          stack_copy[j]=kernel_args[i][j]; //copy direct string
          //kprintf("\nStack Copy is %d\n",strlen(stack_copy));
      }
      for(j=old_length;j<arg_length;j++)
      {
          //sprintf(stack_copy, "%s%c", stack_copy,'\0');
          //strcat(stack_copy,'\0');
          stack_copy[j]='\0'; //copy nulls
          //kprintf("\nStack Copy is %c %d\n",stack_copy[j],j);
      }
        //kprintf("\nStack Copy is %c\n",stack_copy[j]);

      //kprintf("\nStack Copy is %d\n",strlen(stack_copy));
      stackptr-=arg_length; //moving stack ptr down
      int check4=copyout((const void *)stack_copy,(userptr_t)stackptr,(size_t)arg_length); //copy it back to user sapce
      if(check4){
        kfree(stack_copy);
        kfree(kernel_args);
        return check4;
      }
      //kprintf("\nStack Ptr is %s\n",(char *)stackptr);
      //kernel_args[i]=(char *)stackptr;
        //copyoutargs=(userptr_t*)stackptr;
        kernel_args[i]=(char *)stackptr;
        i++;
        kfree(stack_copy);
     }
     //kprintf("i is %d",i);
      stackptr -= 4; //one null spot between args and pointers
      int k=0;
      int count=8;
      for(k=i-1;k>=0;k--)
      {
        //kprintf("Sup");
        stackptr -= 4;
        int check5=copyout((const void *)(kernel_args+k),(userptr_t)stackptr,(sizeof(char *))); //copy pointers to the stack memories having the arguments
        //kprintf("\nKernel Arg is %s\n",*(kernel_args+k));
        count+=8;

      if(check5){
        kfree(kernel_args);
        return check5;
      }
      }
      kfree(kernel_args);

      //kfree(stack_copy);
      //may need to do kfree's






	/* Warp to user mode. */
	enter_new_process(argc, (userptr_t)stackptr /*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);

	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;


}
