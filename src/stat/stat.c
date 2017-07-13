#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "syscall.h"
#include "libc.h"
#include "gg.h"

int stat(const char *restrict path, struct stat *restrict buf)
{
	if( getenv( GG_ENABLED_ENVAR ) ) {
		char * new_file = get_gg_file( path );

		if ( NULL != new_file ) {
				path = new_file;
		}
		else if ( is_dir_allowed( path ) ) {
			path = getenv( GG_DIR_ENVAR );
		}
		else {
			if( getenv( GG_VERBOSE_ENVAR ) ) {
				fprintf( stderr, "[gg] stat() denied: %s\n", path );
			}

			errno = ENOENT;
			return ENOENT;
		}
	}

#ifdef SYS_stat
	return syscall(SYS_stat, path, buf);
#else
	return syscall(SYS_fstatat, AT_FDCWD, path, buf, 0);
#endif
}

LFS64(stat);
