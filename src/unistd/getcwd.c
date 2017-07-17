#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <libgen.h>
#include "syscall.h"

#include "../gg/gg.h"

char *getcwd(char *buf, size_t size)
{
	char tmp[PATH_MAX];
	if (!buf) {
		buf = tmp;
		size = PATH_MAX;
	} else if (!size) {
		errno = EINVAL;
		return 0;
	}

	if (__gg.enabled) {
		char * cwd = dirname(__gg.thunk_file);
		if (strlen(cwd) + 1 > size) {
			errno = ERANGE;
			return NULL;
		}
		strcpy(buf, cwd);
	}
	else {
		if (syscall(SYS_getcwd, buf, size) < 0) return 0;
	}

	return buf == tmp ? strdup(buf) : buf;
}
