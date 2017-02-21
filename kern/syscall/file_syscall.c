#include<file_syscall.h>
#include<types.h>
#include<uio.h>
#include <kern/errno.h>
#include <kern/syscall.h>
#include <limits.h>
#include <lib.h>
#include <current.h>



int sys_write(int fd, const void *buf,size_t size, ssize_t *retval){

  //check if FD invalid, return EBDAF
  if(fd<0||fd>OPEN_MAX) //googled
  	return EBADF;


	 if(fd == 1){
	 	kprintf(buf);
    kprintf("Value");
    kprintf("%p\n", buf);
    kprintf("%p\n", curproc->p_addrspace);

	 }
	retval=0;
	retval++;
	size++;
	return size;
	//check when address space pointed by buf is invalid, return EFAULT
}
