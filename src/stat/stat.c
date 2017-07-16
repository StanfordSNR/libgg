#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "syscall.h"
#include "libc.h"

#include "../gg/gg.h"

int stat(const char *restrict path, struct stat *restrict buf)
{
	if( __gg.enabled ) {
		return __gg_stat( path, buf );
	}

	int retval;
#ifdef SYS_stat
	retval = syscall(SYS_stat, path, buf);
#else
	retval = syscall(SYS_fstatat, AT_FDCWD, path, buf, 0);
#endif

	return retval;
}

LFS64(stat);
