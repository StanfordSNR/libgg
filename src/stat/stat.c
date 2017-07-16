#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "syscall.h"
#include "libc.h"

#include "../gg/gg.h"

int stat(const char *restrict path, struct stat *restrict buf)
{
	int gg_fake_dir = -1;

	if( __gg.enabled ) {
		char * new_file = get_gg_file( path );

		if ( NULL != new_file ) {
				path = new_file;
		}
		else if ( ( gg_fake_dir = is_dir_allowed( path ) ) >= 0 ) {
			path = __gg.dir;
		}
		else {
			if( __gg.verbose ) {
				fprintf( stderr, "[gg] stat() denied: %s\n", path );
			}

			errno = ENOENT;
			return -1;
		}
	}

	int retval;
#ifdef SYS_stat
	retval = syscall(SYS_stat, path, buf);
#else
	retval = syscall(SYS_fstatat, AT_FDCWD, path, buf, 0);
#endif

	/* [gg] we assign a different inode num to each folder to make them appear as
	   different directories */
	if ( gg_fake_dir >= 0 ) {
		buf->st_ino += ( gg_fake_dir + 1 );
	}

	return retval;
}

LFS64(stat);
