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
	char * __gg_normalized = NULL;

	/* NOTE there are some cases that the outfile could be also an infile. One
	example is `ranlib`. To distinguish these (as the infile must be read
	from .gg directory), the outfile must be opened with O_WRONLY or O_RDWR,
	otherwise it will be treated as an infile. */
	if ( __gg.enabled ) {
		__gg_normalized = __gg_normalize_path( filename, NULL );
		filename = __gg_normalized;

		GG_DEBUG( "open(filename=\"%s\", npath=\"%s\", flags=0x%x)\n", filename, __gg_normalized, flags );

		char * infile_path = __gg_get_filename( filename );
		Output * output = __gg_get_output( filename );

		const int accmode = flags & O_ACCMODE;
		const bool is_infile = ( infile_path != NULL );
		const char * allowed_file = __gg_get_allowed( filename, false );
		const bool is_outfile = ( output != NULL );

		if ( is_outfile ) {
			/* no need to check is_allowed_file */
			if ( is_infile ) {
				/* this is both an infile and an outfile */
				if ( accmode == O_RDONLY ) {
					/* the user is going to read the file, so we redirect to the infile */
					filename = infile_path;
				}
				else if ( ( ( accmode == O_RDWR ) || ( accmode == O_WRONLY ) ) && ( flags & O_TRUNC ) ) {
					/* from now on, this file can only be accessed as an outfile. */
					__gg_disable_infile( filename );
					output->created = true;
					filename = output->tag;
				}
				else {
					/* we can't let the user access this file */
					free( __gg_normalized );
					errno = EACCES;
					return -1;
				}
			}
			else { /* not an infile, just an outfile */
				/* let the user do anything with this outfile */
				output->created = true;
				filename = output->tag;
			}
		}
		else { /* not an outfile */
			if ( is_infile ) {
				if ( accmode == O_RDONLY ) {
					/* the user is going to read the file, so we redirect to the infile */
					filename = infile_path;
				}
				else {
					free( __gg_normalized );
					errno = ENOENT;
					return -1;
				}
			}
			else { /* not an infile, nor an outfile */
				if ( allowed_file ) {
					/* let the user do anything with this allowed file */
					filename = allowed_file;
				}
				else {
					/* it's not an infile, an allowed file or an outfile. the user is allowed
					to open it with (O_WRONLY | O_RDWR) & O_TRUNC */
					if ( ( accmode == O_RDWR || accmode == O_WRONLY ) &&
							 ( flags & O_EXCL ) && ( flags & O_CREAT ) ) {
						filename = __gg_create_allowed( filename );
					}
					else {
						free( __gg_normalized );
						errno = ENOENT;
						return -1;
					}
				}
			}
		}

		GG_DEBUG( "open redirected to: %s\n", filename );
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

	if ( __gg_normalized ) {
		free( __gg_normalized );
	}

	return retval;
}

LFS64(open);
