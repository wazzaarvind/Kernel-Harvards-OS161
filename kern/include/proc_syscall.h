#include<types.h>
#include <mips/trapframe.h>
#include<proc.h>


pid_t sys_fork(struct trapframe *tf, int *reval);

int waitpid(pid_t pid, int *status, int options, int *retval);
