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

  const char *name = "Newly created process!";

  newProc = fork_proc_create(name);

  if(newProc == NULL){
    return ENOMEM;
  }

  memcpy(trapframe, tf, sizeof(struct trapframe));


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

  if (proctable[pid]->ppid == pid) {
      return ECHILD;
    }



  if(status==NULL)
    return 0;

  if(status==(void *)0x40000000||status==(void *)0x80000000)
    return EFAULT;

    // Going back to the old setting :
   if(proctable[pid]->exit_status!=1)
    {
      P(proctable[pid]->proc_sem);  // wwait for child
   }




  //if(status != NULL)
    copyout((const void *)&proctable[pid]->exit_code, (userptr_t)status, sizeof(int));
//  }


  proc_destroy(proctable[pid]);




  proctable[pid]=NULL;
  *retval=pid;

  return 0;
}

int sys__exit(int exitcode){

      //kprintf("\nEXIT %d\n",exitcode);
      //KASSERT(curproc->exit_status!=1); Why?

      curproc->exit_code=_MKWAIT_EXIT(exitcode);
      //curproc->exit_status=1;
      curproc->exit_status=1;
      V(curproc->proc_sem);
      //proc_destroy(curproc);

      thread_exit();
      //proc_destroy(curproc);
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




  if(progname==NULL||args==NULL||progname==(void *)0x40000000||progname==(void *)0x80000000||args==(void*)0x40000000||args==(char **)0x40000000||args>=(char **)0x80000000||(void *)progname>=(void *)0x80000000)
    return EFAULT;


  //int no_of_elements;

  //kprintf("\nWorks\n");

  //(void) **args;
  //int l=0;
  size_t length;
  char *program_kern=(char *)kmalloc(sizeof(char)*PATH_MAX); //might need to kmalloc
  int countAlloc = 0;

  if(program_kern==NULL)
    return ENOMEM;
  //casting the malloc to the right type, might not be required
  // program_kern=(char *)kmalloc(sizeof(char)*PATH_MAX);


   //First Copy Program into Kernel Memory, PATH_MAX is the maximum size of an instruction path
   int check1=copyinstr((userptr_t)progname, program_kern, PATH_MAX , &length); //might need to check error for all these
   if(check1)
   {
      kfree(program_kern);
      return EFAULT;
   }

   if (length < 2 || length>PATH_MAX) {
    kfree(program_kern);
    return EINVAL;
  }



    // Moved sanity check on args down.
    int u=0;
    for(u=0;args[u]!=NULL;u++)
    {
      //kprintf(" POINTER %p\n", args[u]);
      if((args[u]==(void*)0x40000000)||((void *)args[u]>=(void*)0x80000000))
      {
        kfree(program_kern);
        return EFAULT;
      }

    }

   char **kernel_args=(char **)kmalloc(sizeof(char**)*u);

   //kprintf("\nWorks2\n");

   if(kernel_args==NULL) {
     //kfree(kernel_args);
     kfree(program_kern);
     return ENOMEM;
   }


  int i=0;
  for(i=0;args[i]!=NULL;i++)
  {

  //Copy each value in user memory to kernel memory
    //kprintf("ARGS i : %p", args[i]);

    kernel_args[i] = (char *)kmalloc(sizeof(char)*strlen(args[i])+1);
    countAlloc++; //might need dummy operation for size //length
    if(kernel_args[i] == NULL){
      kfree(program_kern);
      //kfree(kernel_args[i]);
      //kfree(kernel_args);
      return EFAULT;
    }
    //kprintf("args is %s",args[i]);
    int check3 = copyinstr((userptr_t) args[i],kernel_args[i], strlen(args[i])+1, &length); //strlen(kernel_args[i])+1 //ARG_MAX
    if(check3){
      // Deallocate memory for kernel_args[i]
      // for(int l = 0; args[l]!= NULL; l++){
      //
      // }
      //for(l=0;kernel_args[l]!=NULL;l++)
      //  kfree(kernel_args[l]);
      //kfree(kernel_args);
      kfree(program_kern);
      return EFAULT;
    }
  }
  //kprintf("\nWorks2\n");


  int argc=i;
  //kprintf("\nArgc is %d\n",argc);

  if(argc > ARG_MAX){
    //for(l=0;kernel_args[l]!=NULL;l++)
    //    kfree(kernel_args[l]);
    //kfree(kernel_args);
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
      //for(l=0;kernel_args[l]!=NULL;l++)
        //kfree(kernel_args[l]);
      //kfree(kernel_args);
		  return result;
	}

  //New address space needs to be created
  //as_deactivate();
  as_destroy(curproc->p_addrspace);
  curproc->p_addrspace = NULL;

	/* Create a new address space. */
	curproc->p_addrspace = as_create();
	if (curproc->p_addrspace == NULL) {
    //for(l=0;kernel_args[l]!=NULL;l++)
        //kfree(kernel_args[l]);
    //kfree(kernel_args);
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	proc_setas(curproc->p_addrspace);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
    //for(l=0;kernel_args[l]!=NULL;l++)
        //kfree(kernel_args[l]);
  //  kfree(kernel_args);
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(curproc->p_addrspace, &stackptr);
	if (result) {
    //for(l=0;kernel_args[l]!=NULL;l++)
        //kfree(kernel_args[l]);
    //kfree(kernel_args);
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
        //for(l=0;kernel_args[i]!=NULL;l++)
          //kfree(kernel_args[l]);
      //  kfree(kernel_args);
        return check4;
      }
        kfree(kernel_args[i]);
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
        //for(l=0;kernel_args[l]!=NULL;l++)
        //  kfree(kernel_args[l]);
        //kfree(kernel_args);
        return check5;
      }
      }
      //kprintf("Counter : %d\n", countAlloc);
      //for(i=0;kernel_args[i]!=NULL;i++){
          //kprintf("i : %d\n", i);
          //kfree(kernel_args[i]);
          //kernel_args[i] = NULL;
      //}
      //kprintf("kfree done, Counter : %d\n", i);
      //   ;
      // }
      //kfree(kernel_args[0]);
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


int sys_sbrk(intptr_t amount, int *retval)
{
  kprintf("\nAmount : %d\n",(int)amount);
  struct addrspace *as = curproc->p_addrspace;
  if(as==NULL)
    return EINVAL;
  vaddr_t vaddr_to_free;

  //Top is start and bottom is end

  kprintf("Initial value : %d",curproc->p_addrspace->heap_bottom);

  int returnval = curproc->p_addrspace->heap_bottom;

  if(as->heap_bottom + amount < as->heap_top)
    return EINVAL;
  if(amount + as->heap_bottom >= as->stack_top)
    return ENOMEM;
  if(amount+ as->heap_bottom < as->heap_bottom)
  {
    vaddr_to_free = (as->heap_bottom + amount)&PAGE_FRAME;
    int npages = (amount/4096)*-1;
    for(int i=0;i<npages;i++)
      free_upage(KVADDR_TO_PADDR(vaddr_to_free));
    curproc->p_addrspace->heap_bottom = curproc->p_addrspace->heap_bottom + amount;
    return 0;
  }

  curproc->p_addrspace->heap_bottom = curproc->p_addrspace->heap_bottom + amount;
  kprintf("Final value : %d",curproc->p_addrspace->heap_bottom);
  *retval = returnval;
  return 0;


}
