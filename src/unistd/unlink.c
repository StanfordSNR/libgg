#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "syscall.h"

#include "../gg/gg.h"

int unlink(const char *path)
{
	if ( __gg.enabled ) {
		/* we only let programs unlink the temp files */
		const char * allowed_file = __gg_get_allowed( path, false );

		if ( path == NULL ) {
			errno = EACCES;
			return -1;
		}

		path = allowed_file;
	}


#ifdef SYS_unlink
	return syscall(SYS_unlink, path);
#else
	return syscall(SYS_unlinkat, AT_FDCWD, path, 0);
#endif
}
