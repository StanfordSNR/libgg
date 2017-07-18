#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "syscall.h"

#include "../gg/gg.h"

int rename(const char *old, const char *new)
{
	if (__gg.enabled) {
		/* the only allowed destination is the outfile */
		if (strcmp(new, __gg.outfile) != 0) {
			errno = EACCES;
			return -1;
		}

		/* only the files that are created by the process can be renamed. */
		if(!__gg_is_allowed(old, false)) {
			errno = EACCES;
			return -1;
		}
	}

#ifdef SYS_rename
	return syscall(SYS_rename, old, new);
#else
	return syscall(SYS_renameat, AT_FDCWD, old, AT_FDCWD, new);
#endif
}
