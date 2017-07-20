#include <sys/stat.h>
#include <fcntl.h>
#include "syscall.h"
#include "libc.h"

#include "../gg/gg.h"

int lstat(const char *restrict path, struct stat *restrict buf)
{
	if( __gg.enabled ) {
		GG_DEBUG( "lstat(path=\"%s\")\n", path );
		return __gg_stat( path, buf );
	}

#ifdef SYS_lstat
	return syscall(SYS_lstat, path, buf);
#else
	return syscall(SYS_fstatat, AT_FDCWD, path, buf, AT_SYMLINK_NOFOLLOW);
#endif
}

LFS64(lstat);
