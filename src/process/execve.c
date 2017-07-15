#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "syscall.h"
#include "../gg/gg.h"

int execve(const char *path, char *const argv[], char *const envp[])
{
	if( getenv( GG_ENABLED_ENVAR ) ){
        if( getenv( GG_VERBOSE_ENVAR ) ){
            fprintf( stderr, "DANITER EXEC %s\n", path );
        }

        char *new_file = get_gg_file(path);
        if ( NULL != new_file ) {
            path = new_file;
        } else {
            errno = ENOENT;
            return ENOENT;
        }
	}
	/* do we need to use environ if envp is null? */
	return syscall(SYS_execve, path, argv, envp);
}
