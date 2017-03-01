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

  lock_acquire(curproc->filetable[fd]->lock);
  iov->iov_ubase = (void *)buf;
  iov->iov_len = size;

  uioWrite->uio_iov = iov;
  uioWrite->uio_iovcnt = 1;
  uioWrite->uio_offset = curproc->filetable[fd]->offset;
  uioWrite->uio_resid = size;
  uioWrite->uio_segflg = UIO_USERSPACE;
  uioWrite->uio_rw = UIO_WRITE;
  uioWrite->uio_space = curproc->p_addrspace;

  int err = VOP_WRITE(outFile, uioWrite);

  if(err){
	lock_release(curproc->filetable[fd]->lock);
	return err;
  }
  curproc->filetable[fd]->offset=uioWrite->uio_offset;
  *retval = size - uioWrite->uio_resid;
  lock_release(curproc->filetable[fd]->lock);

  //retval++;

	return result;
	//check when address space pointed by buf is invalid, return EFAULT
}

int sys_read(int fd, void *buf, size_t buflen, ssize_t *retval){

	if(fd<0||fd>OPEN_MAX)
        	return EBADF;

	struct uio *uioRead = kmalloc(sizeof(uioRead));
  	struct iovec *iov = kmalloc(sizeof(iov));
  	struct vnode *inFile= kmalloc(sizeof(inFile));
	lock_acquire(curproc->filetable[fd]->lock);

	iov->iov_ubase = (void *)buf;
  	iov->iov_len = buflen;
	inFile = curproc->filetable[1]->file;
	uioRead->uio_iov = iov;
  	uioRead->uio_iovcnt = 1;
  	uioRead->uio_offset = uioRead->uio_offset; //not sure
  	uioRead->uio_resid = buflen;
  	uioRead->uio_segflg = UIO_USERSPACE;
  	uioRead->uio_rw = UIO_READ;
  	uioRead->uio_space = curproc->p_addrspace;

	int err = VOP_READ(inFile, uioRead);
	if(err){
		lock_release(curproc->filetable[fd]->lock);
		return err;
	}
	curproc->filetable[fd]->offset=uioRead->uio_offset;
	*retval = buflen - uioRead->uio_resid;
	lock_release(curproc->filetable[fd]->lock);

  return 0; 
}


int sys_open(const char *path_file, int flags, mode_t mode, int *retval){

	if(path_file==NULL)
		return EFAULT;

	int file_index=3;
	struct stat *stats_file;

	while(curproc->filetable[file_index]!=NULL)
		file_index++;

	if(file_index>65)
		return EINVAL;

	//curproc->filetable[fd]=kmalloc(sizeof(struct)) Do we need to do this?

	struct vnode *open_vn= kmalloc(sizeof(outFile));

	int check=vfs_open(path_file,flags,mode,open_vn); // or curproc->filetable[index]->file;



	curproc->filetable[file_index]->offset=0;

	if(flags==O_APPEND)
	{
		int check1=VOP_STAT(,stats_file)

		//update offset dependng on append
		//curproc->filetable[file_index]->offset=

	}

	curproc->filetable[file_index]->counter++;
	curproc->filetable[file_index]->lock=lock_create(path_file);
	curproc->filetable[file_index]->file=open_vn;
	//offset
	//flags

	*retval=file_index;

}


int sys_close(int fd){

	if(fd<0||fd>OPEN_MAX)
        	return EBADF;

	curproc->filetable[fd]->counter--;

	if(!curproc->filetable[fd]->counter)
	{
		lock_destroy(curproc->filetable[fd]->lock);
		curproc->filetable[fd]=NULL;
		vfs_close(curproc->filetable[fd]->file);
	}
	return 0;
}

int sys_dup2(int fd_old, int fd_new, int *retval){

	if(fd_new>OPEN_MAX)
		return EMFILE;
	if(fd_old<0||fd_old>OPEN_MAX||fd_new<0||fd_new>OPEN_MAX)
		return EBADF;

	if(curproc->filetable[fd_old]==NULL)
		return EBADF;
	//lock_acquire
	if(curproc->filetable[fd_new]!=NULL)
		sys_close(fd_new); //could cause error, might need to be dealt with
	curproc->filetable[fd_new]=curproc->filetable[fd_old];
	curproc->filetable[fd_new]->counter++;
	*retval=fd_new;
	//lock_release
  return 0;
}

/*
int sys_lseek(int fd, off_t pos, int whence, off_t *new_pos){
}

int sys_chdir(const char *path_name){
}

int sys__getcwd(char *buf, size_t buflen,int *retval){
}
*/
