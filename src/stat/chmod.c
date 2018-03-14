#include <sys/stat.h>
#include <fcntl.h>
#include "syscall.h"

#include "../gg/gg.h"

int chmod(const char *path, mode_t mode)
{
	if ( __gg.enabled ) {
		Output * output = __gg_get_output( path );
		if ( output ) {
			path = output->tag;
		}
	}

#ifdef SYS_chmod
	return syscall(SYS_chmod, path, mode);
#else
	return syscall(SYS_fchmodat, AT_FDCWD, path, mode);
#endif
}
