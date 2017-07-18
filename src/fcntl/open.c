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

	if (__gg.enabled && strcmp(filename, __gg.outfile) != 0) {
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
			bool allowed = false;
			for (size_t i = 0; i < __gg.allowed_files.count; i++) {
				if (strcmp(filename, __gg.allowed_files.data[ i ].path) == 0) {
					allowed = true;
					break;
				}
			}

			if (!allowed) {
				errno = ENOENT;
				return -1;
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

	if ( __gg.enable && __gg_check_to_allow && retval != -1 ) {
		vector_AllowedFiles_push_back( &__gg.allowed_files, strdup( filename ) );
	}

	return retval;
}

LFS64(open);
