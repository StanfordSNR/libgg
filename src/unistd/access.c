#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "syscall.h"
#include "../gg/gg.h"

int access(const char *filename, int amode)
{
	if( getenv( GG_ENABLED_ENVAR ) ) {
		char *new_file = get_gg_file(filename);
		if (NULL != new_file) {
			filename = new_file;
		} else {
            if( getenv( GG_VERBOSE_ENVAR ) ){
                fprintf(stderr, "DANITER ACCESS DENIED : %s\n", filename);
            }
            return ENOENT;
        }
	}
#ifdef SYS_access
	return syscall(SYS_access, filename, amode);
#else
	return syscall(SYS_faccessat, AT_FDCWD, filename, amode, 0);
#endif
}
