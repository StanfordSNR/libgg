#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include "syscall.h"
#include "../gg/gg.h"

int access(const char *filename, int amode)
{
	if( __gg.enabled ) {
		char *new_file = __gg_get_filename(filename);
		if (NULL != new_file) {
			filename = new_file;
		} else {
            if( __gg.verbose ){
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
