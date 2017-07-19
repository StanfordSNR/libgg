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

		bool is_infile = ( infile_path != NULL );
		bool is_allowed_file = __gg_is_allowed( filename, false );
		bool is_outfile = ( strcmp( filename, __gg.outfile ) == 0 );

		if ( ( is_infile || is_allowed_file ) && !is_outfile ) {
			if ( is_infile ) {
				/* infiles can only be opened in read-only mode */
				if ( flags & O_RDONLY ) {
					filename = infile_path;
				}
				else {
					errno = EACCES;
					return -1;
				}
			} /* is_infile */
			else {
				/* it's an allowed file: do anything you want! */
			}
		}
		else if ( !is_outfile ) {
			/* it's not an infile, an allowed file or an outfile. the user is allowed
			to open it with (O_WRONLY | O_RDWR) & O_TRUNC */
			if ( ( flags & O_WRONLY || flags & O_RDWR) && ( flags & O_TRUNC ) ) {
				__gg_check_to_allow = true;
			}
			else {
				errno = EACCES;
				return -1;
			}
		}
		else if ( !is_allowed_file ) {
			/* this is both an infile and an outfile */
			if ( flags & O_RDONLY ) {
				/* the user is going to read the file, so we redirect to the infile */
				filename = infile_path;
			}
			else if ( ( flags & O_WRONLY || flags & O_RDWR) && ( flags & O_TRUNC ) ) {
				/* from now on, this file can only be accessed as an outfile. */
				__gg_disable_infile( filename );
			}
			else {
				/* we can't let the user access this file */
				errno = EACCES;
				return -1;
			}
		}
		else {
			GG_ERROR( "something is wrong with infiles and allowed files." );
			errno = EACCES;
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

	int retval = __syscall_ret(fd);

	if ( __gg.enabled && __gg_check_to_allow && retval != -1 ) {
		AllowedFile allowed_file;
		strcpy( allowed_file.path, filename );
		vector_AllowedFile_push_back( &__gg.allowed_files, &allowed_file );
	}

	return retval;
}

LFS64(open);
