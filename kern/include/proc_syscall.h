#include<types.h>
#include <mips/trapframe.h>
#include<proc.h>


pid_t sys_fork(struct trapframe *tf, int *reval);

int sys_waitpid(pid_t pid, int *status, int options, int *retval);

int	sys__exit(int exitcode);

int sys_getpid(pid_t *retval);

int sys_execv(const char *program, char **args); 

int sys_sbrk(intptr_t amount, int *retval);
