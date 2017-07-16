#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "syscall.h"
#include "../gg/gg.h"

int execve(const char *path, char *const argv[], char *const envp[])
{
	if (__gg_enabled) {
        if (__gg_verbose) {
            GG_INFO( "execve: %s\n", path );
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
