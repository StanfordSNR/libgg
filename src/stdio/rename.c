#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "syscall.h"

#include "../gg/gg.h"

int rename(const char *old, const char *new)
{
	if (__gg.enabled) {
		GG_DEBUG( "rename(old=\"%s\", new=\"%s\")\n", old, new );
		/* the only allowed destination is one of th output tags */

		Output * output = __gg_get_output(new);

		if (output == NULL) {
			errno = EACCES;
			return -1;
		}

		new = output->tag;

		/* only the files that are created by the process can be renamed. */
		const char * allowed_file = __gg_get_allowed(old, false);
		if(!allowed_file) {
			errno = EACCES;
			return -1;
		}
		old = allowed_file;
	}

#ifdef SYS_rename
	return syscall(SYS_rename, old, new);
#else
	return syscall(SYS_renameat, AT_FDCWD, old, AT_FDCWD, new);
#endif
}
