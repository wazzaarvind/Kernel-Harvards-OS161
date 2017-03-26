#include <file_syscall.h>
#include <types.h>
#include <uio.h>
#include <kern/errno.h>
#include <kern/seek.h>
#include <kern/syscall.h>
#include <limits.h>
#include <lib.h>
#include <current.h>
#include <copyinout.h>
#include <vfs.h>
#include <proc.h>
#include <kern/iovec.h>
#include <kern/fcntl.h>
#include <kern/stat.h>

int sys_write(int fd, const void *buf,size_t size, ssize_t *retval){

	//kprintf("BUFF IS %d",curproc->filetable[fd]->flags);
  //check if FD invalid, return EBDAF
  if(fd<0||fd>=OPEN_MAX)
  	return EBADF;

  if(curproc->filetable[fd]==NULL)
     	return EBADF;
  if(buf==NULL||buf>=(void *)0x80000000||buf==(void *)0x40000000)
  		return EFAULT;
  	if((curproc->filetable[fd]->flags & O_ACCMODE)==O_RDONLY)
  	{
     	*retval=-1;
     	return EBADF;
    }

  /*Achuth edits : Fetching the stdout file handle and writing to the file.*/

  struct uio uioWrite;
  struct iovec iov;

  lock_acquire(curproc->filetable[fd]->lock);
  iov.iov_ubase = (userptr_t)buf;
  iov.iov_len = size;

  uioWrite.uio_iov = &iov;
  uioWrite.uio_iovcnt = 1;
  uioWrite.uio_offset = curproc->filetable[fd]->offset;
  uioWrite.uio_resid = size;
  uioWrite.uio_segflg = UIO_USERSPACE;
  uioWrite.uio_rw = UIO_WRITE;
  uioWrite.uio_space = curproc->p_addrspace;

  int err = VOP_WRITE(curproc->filetable[fd]->file, &uioWrite);

  if(err){
      //kprintf("\nError!!\n");
	    lock_release(curproc->filetable[fd]->lock);
	    return err;
  }

  curproc->filetable[fd]->offset = uioWrite.uio_offset;
  *retval = size - uioWrite.uio_resid;
  lock_release(curproc->filetable[fd]->lock);
	return 0;
	//check when address space pointed by buf is invalid, return EFAULT
}

int sys_read(int fd, void *buf, size_t buflen, ssize_t *retval){


	if(fd<0||fd>=OPEN_MAX)
        	return EBADF;
     if(curproc->filetable[fd]==NULL)
     	return EBADF;
     if(buf==NULL||buf>=(void *)0x80000000||buf==(void *)0x40000000)
  		return EFAULT;
     if((curproc->filetable[fd]->flags & O_ACCMODE)==O_WRONLY)
     {
     	*retval=-1;
     	return EBADF;
     }
	  struct uio uioRead;
  	struct iovec iov;


  lock_acquire(curproc->filetable[fd]->lock);

	  iov.iov_ubase = (userptr_t) buf;
	  iov.iov_len = buflen;
	  uioRead.uio_iov = &iov;
  	uioRead.uio_iovcnt = 1;
  	uioRead.uio_offset = curproc->filetable[fd]->offset; //not sure
  	uioRead.uio_resid = buflen;
  	uioRead.uio_segflg = UIO_USERSPACE;
  	uioRead.uio_rw = UIO_READ;
  	uioRead.uio_space = curproc->p_addrspace;

	int err = VOP_READ(curproc->filetable[fd]->file, &uioRead);

  if(err){
		lock_release(curproc->filetable[fd]->lock);
		return err;
	}
	curproc->filetable[fd]->offset = uioRead.uio_offset;
	*retval = buflen - uioRead.uio_resid;
	lock_release(curproc->filetable[fd]->lock);

  return 0;
}


int sys_open(char *path_file, int flags, mode_t mode, int *retval){


	int file_index=3;

  char buf[128];

	while(curproc->filetable[file_index]!=NULL){
		  file_index++;
  }

	if(file_index>64){
		return EINVAL;
  }

  //struct vnode *open_vn;//;= kmalloc(sizeof(open_vn));

  size_t kernCopy;

  int errorCpy = copyinstr((const_userptr_t)path_file, buf, 128, &kernCopy);

  if (errorCpy != 0) {
    return EFAULT;
  }

  	curproc->filetable[file_index]=kmalloc(sizeof(struct filehandle));
	int check=vfs_open(buf,flags, mode, &curproc->filetable[file_index]->file); // or curproc->filetable[index]->file instead of open_vn

	if(check!=0)
	{
		return check;
	}



	curproc->filetable[file_index]->offset=0;
	curproc->filetable[file_index]->flags=flags;

	curproc->filetable[file_index]->counter=1;
	curproc->filetable[file_index]->lock=lock_create(path_file);
	//curproc->filetable[file_index]->file=open_vn;


	*retval=file_index;
	return 0;

}


int sys_close(int fd){

	//kprintf("FD is %d",fd);

	if(fd<0||fd>=OPEN_MAX)
        	return EBADF;
  if(curproc->filetable[fd]==NULL)
 	        return EBADF;

  	//kprintf("Decrementing counter");
  	if(curproc->filetable[fd]->counter!=0)
	curproc->filetable[fd]->counter--;

	if(curproc->filetable[fd]->counter == 0 && fd>2)
	{
	 lock_destroy(curproc->filetable[fd]->lock);
	 vfs_close(curproc->filetable[fd]->file);
   kfree(curproc->filetable[fd]);
   curproc->filetable[fd]=NULL;
	}
	return 0;
}

int sys_dup2(int fd_old, int fd_new, int *retval){

	if(fd_new<0||fd_new>=OPEN_MAX)
		return EBADF;
	if(fd_old<0||fd_old>=OPEN_MAX)
		return EBADF;

	if(curproc->filetable[fd_old]==NULL)
		return EBADF;

	if(curproc->filetable[fd_new]!=NULL)
		sys_close(fd_new);
	curproc->filetable[fd_new]=curproc->filetable[fd_old];
	curproc->filetable[fd_new]->counter++;
	*retval=fd_new;
  return 0;
}


int sys_lseek(int fd, off_t pos, int whence, off_t *new_pos){

	struct stat *stats_file=kmalloc(sizeof(struct stat));
	if(fd<0||fd>=OPEN_MAX||curproc->filetable[fd]==NULL)
		return EBADF;

	//struct vnode *lseek_vn= kmalloc(sizeof(lseek_vn));

	int check=VOP_ISSEEKABLE(curproc->filetable[fd]->file);
	if(check==0)
		return ESPIPE;

	if(whence==SEEK_SET)
	{	curproc->filetable[fd]->offset=pos; //might need to change type of offset in proc.h to off_t
		if(curproc->filetable[fd]->offset<0)
			return EINVAL;
	}


	else if(whence==SEEK_CUR)
	{
		curproc->filetable[fd]->offset+=pos;
		if(curproc->filetable[fd]->offset<0)
			return EINVAL;
	}
	else if(whence==SEEK_END)
	{
		int check1=VOP_STAT(curproc->filetable[fd]->file,stats_file);
		if(check1==0)
			curproc->filetable[fd]->offset=pos+stats_file->st_size;
		else
			return check1;
		if(curproc->filetable[fd]->offset<0)
			return EINVAL;
	}
	else
		return EINVAL;

	*new_pos=curproc->filetable[fd]->offset;

	return 0;

}
