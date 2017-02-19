#ifndef _FILE_SYSCALL_H_
#define _FILE_SYSCALL_H_

int sys_write(int fd, const void *buf, size_t buflen, ssize_t *retval);
