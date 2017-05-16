#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


#include "syscall.h"
#include "gg.h"

int execve(const char *path, char *const argv[], char *const envp[])
{
	if( getenv( GG_ENV_VAR ) && getenv( GG_VERBOSE ) ){
		fprintf(stderr, "DANITER EXEC %s\n", path);
	}
	/* do we need to use environ if envp is null? */
	return syscall(SYS_execve, path, argv, envp);
}
