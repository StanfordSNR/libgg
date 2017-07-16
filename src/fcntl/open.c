#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include "syscall.h"
#include "libc.h"

#include "../gg/gg.h"

int open(const char *filename, int flags, ...)
{
	mode_t mode = 0;

	if(__gg_enabled) {
		char * new_file = get_gg_file(filename);

		if (NULL != new_file) {
			filename = new_file;
		}
		else if (strcmp(filename, __gg_outfile) != 0 ) {
			errno = ENOENT;
			return -1;
		}
	}

	if ((flags & O_CREAT) || (flags & O_TMPFILE) == O_TMPFILE) {
		va_list ap;
		va_start(ap, flags);
		mode = va_arg(ap, mode_t);
		va_end(ap);
	}

	int fd = __sys_open_cp(filename, flags, mode);
	if (fd>=0 && (flags & O_CLOEXEC))
		__syscall(SYS_fcntl, fd, F_SETFD, FD_CLOEXEC);

	return __syscall_ret(fd);
}

LFS64(open);
