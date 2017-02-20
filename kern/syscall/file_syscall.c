int sys_write(int fd, const void *buf,size_t size, ssize_t *retval){
	struct uio uio;
	if(fd<0||fd>OPEN_MAX)
		return EBDAF; //googled
	//check if FD invalid, return EBDAF

	//check when address space pointed by buf is invalid, return EFAULT	
}
