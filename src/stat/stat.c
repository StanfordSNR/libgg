#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "syscall.h"
#include "libc.h"
#include "gg.h"

int stat(const char *restrict path, struct stat *restrict buf)
{
    if( getenv( GG_ENV_VAR ) ) {
        char *new_file = get_gg_file(path);
        if (NULL != new_file) {
            path = new_file;
        } else {
			fprintf(stderr, "DANITER STAT DENIED : %s\n", path);
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
