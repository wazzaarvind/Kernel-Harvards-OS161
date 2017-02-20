#include<file_syscall.h>
#include<types.h>
#include<uio.h>
#include <kern/errno.h>
#include <thread.h>
#include <current.h>
#include <kern/syscall.h>
#include <limits.h>
#include <lib.h>



int sys_write(int fd, const void *buf,size_t size, ssize_t *retval){
	//struct uio uio;
	 if(fd<0||fd>OPEN_MAX)
	 	return EBADF; //googled
	// //check if FD invalid, return EBDAF


	return 0;
	//check when address space pointed by buf is invalid, return EFAULT
}
