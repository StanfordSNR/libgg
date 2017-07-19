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
	from .gg directory), the outfile must be opened with O_WRONLY or O_RDWR,
	otherwise it will be treated as an infile. */
	if ( __gg.enabled ) {
		char * infile_path = __gg_get_filename( filename );

		const bool is_infile = ( infile_path != NULL );
		const bool is_allowed_file = __gg_is_allowed( filename, false );
		const bool is_outfile = ( strcmp( filename, __gg.outfile ) == 0 );

		if ( is_outfile ) {
			/* no need to check is_allowed_file */
			if ( is_infile ) {
				/* this is both an infile and an outfile */
				if ( ( flags & O_ACCMODE ) == O_RDONLY ) {
					/* the user is going to read the file, so we redirect to the infile */
					filename = infile_path;
				}
				else if ( ( ( flags & O_ACCMODE == O_RDWR ) || ( flags & O_ACCMODE == O_WRONLY ) ) && ( flags & O_TRUNC ) ) {
					/* from now on, this file can only be accessed as an outfile. */
					__gg_disable_infile( filename );
				}
				else {
					/* we can't let the user access this file */
					errno = EACCES;
					return -1;
				}
			}
			else { /* not an infile, just an outfile */
				/* let the user do anything with this outfile */
			}
		}
		else { /* not an outfile */
			if ( is_infile ) {
				if ( ( flags & O_ACCMODE ) == O_RDONLY ) {
					/* the user is going to read the file, so we redirect to the infile */
					filename = infile_path;
				}
				else {
					errno = EACCES;
					return -1;
				}
			}
			else { /* not an infile, nor an outfile */
				if ( is_allowed_file ) {
					/* let the user do anything with this allowed file */
				}
				else {
					/* it's not an infile, an allowed file or an outfile. the user is allowed
					to open it with (O_WRONLY | O_RDWR) & O_TRUNC */
					if ( ( ( flags & O_ACCMODE == O_RDWR ) || ( flags & O_ACCMODE == O_WRONLY ) ) && ( flags & O_EXCL ) && ( flags & O_CREAT ) ) {
						__gg_check_to_allow = true;
					}
					else {
						errno = EACCES;
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

	if ( __gg.enabled && __gg_check_to_allow && retval != -1 ) {
		AllowedFile allowed_file;
		strcpy( allowed_file.path, filename );
		vector_AllowedFile_push_back( &__gg.allowed_files, &allowed_file );
	}

	return retval;
}

LFS64(open);
