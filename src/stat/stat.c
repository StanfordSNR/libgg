#include <sys/stat.h>
#include <fcntl.h>
#include "syscall.h"
#include "libc.h"

#include "../gg/gg.h"

int stat(const char *restrict path, struct stat *restrict buf)
{
	if( __gg.enabled ) {
		return __gg_stat( path, buf );
	}

#ifdef SYS_stat
	return syscall(SYS_stat, path, buf);
#else
	return syscall(SYS_fstatat, AT_FDCWD, path, buf, 0);
#endif
}

LFS64(stat);
