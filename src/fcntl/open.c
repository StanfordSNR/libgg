#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include "syscall.h"
#include "libc.h"

#include "../gg/gg.h"

int open(const char *filename, int flags, ...)
{
	mode_t mode = 0;
	bool __gg_check_to_allow = false;

	/* NOTE there are some cases that the outfile could be also an infile. One
	   example is `ranlib`. To distinguish these (as the infile must be read
	   from .gg directory), the outfile must be opened with O_WRONLY flag,
		 otherwise it will be treated as an infile */
	if (__gg.enabled) {
		if ( strcmp(filename, __gg.outfile) == 0 && ( flags & O_WRONLY ) ) {
			/* let's allow it */
		}
		else {
			char * new_file = __gg_get_filename(filename);

			if (NULL != new_file) {
				filename = new_file;
			}
			else if ((flags & O_CREAT) && (flags & O_EXCL)) {
				/* When running in gg mode, we will allow creating files with
				   O_EXCL | O_CREAT. This allows applications to create temporary files. */
				__gg_check_to_allow = true;
			}
			else {
				/* let's see if we allowed this file before */
				if (!__gg_is_allowed(filename, false)) {
					errno = ENOENT;
					return -1;
				}
			}
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

	int retval = __syscall_ret(fd);

	if ( __gg.enabled && __gg_check_to_allow && retval != -1 ) {
		AllowedFile allowed_file;
		strcpy( allowed_file.path, filename );
		vector_AllowedFile_push_back( &__gg.allowed_files, &allowed_file );
	}

	return retval;
}

LFS64(open);
