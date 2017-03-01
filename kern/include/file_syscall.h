#include<types.h>

int sys_write(int fd, const void *buf, size_t buflen, ssize_t *retval);
int sys_read(int fd, void *buf, size_t buflen, ssize_t *retval);
int sys_open(const char *path_file, int flags, mode_t mode, int *retval);
int sys_close(int fd);
int sys_dup2(int fd_old, int fd_new, int *retval);
int sys_lseek(int fd, off_t pos, int whence, off_t *new_pos);
int sys_chdir(const char *path_name);
//int sys__getcwd(char *buf, size_t buflen,int *retval);
