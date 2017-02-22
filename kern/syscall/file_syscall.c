#include <file_syscall.h>
#include <types.h>
#include <uio.h>
#include <kern/errno.h>
#include <kern/syscall.h>
#include <limits.h>
#include <lib.h>
#include <current.h>
#include <copyinout.h>
#include <vfs.h>
#include <proc.h>
#include <kern/iovec.h>


int sys_write(int fd, const void *buf,size_t size, ssize_t *retval){

  //check if FD invalid, return EBDAF
  if(fd<0||fd>OPEN_MAX)
  	return EBADF;

  int result = 0;

  /*Achuth edits : Fetching the stdout file handle and writing to the file.*/

  struct uio *uioWrite = kmalloc(sizeof(uioWrite));
  struct iovec *iov = kmalloc(sizeof(iov));
  struct vnode *outFile= kmalloc(sizeof(outFile));

  outFile = curproc->filetable[1]->file;

  iov->iov_ubase = (void *)buf;
  iov->iov_len = size;

  uioWrite->uio_iov = iov;
  uioWrite->uio_iovcnt = 1;
  uioWrite->uio_offset = 0;
  uioWrite->uio_resid = size;
  uioWrite->uio_segflg = UIO_USERSPACE;
  uioWrite->uio_rw = UIO_WRITE;
  uioWrite->uio_space = curproc->p_addrspace;

  int err = VOP_WRITE(outFile, uioWrite);

  if(err){

  }

  retval++;

	return result;
	//check when address space pointed by buf is invalid, return EFAULT
}
