#include <elf.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include "syscall.h"
#include "atomic.h"
#include "libc.h"

#include "../gg/gg.h"

void __init_tls(size_t *);

static void dummy(void) {}
weak_alias(dummy, _init);

__attribute__((__weak__, __visibility__("hidden")))
extern void (*const __init_array_start)(void), (*const __init_array_end)(void);

static void dummy1(void *p) {}
weak_alias(dummy1, __init_ssp);

#define AUX_CNT 38

void __init_libc(char **envp, char *pn)
{
	size_t i, *auxv, aux[AUX_CNT] = { 0 };
	__environ = envp;
	for (i=0; envp[i]; i++);
	libc.auxv = auxv = (void *)(envp+i+1);
	for (i=0; auxv[i]; i+=2) if (auxv[i]<AUX_CNT) aux[auxv[i]] = auxv[i+1];
	__hwcap = aux[AT_HWCAP];
	__sysinfo = aux[AT_SYSINFO];
	libc.page_size = aux[AT_PAGESZ];

	if (pn) {
		__progname = __progname_full = pn;
		for (i=0; pn[i]; i++) if (pn[i]=='/') __progname = pn+i+1;
	}

	__init_tls(aux);
	__init_ssp((void *)aux[AT_RANDOM]);

	if (aux[AT_UID]==aux[AT_EUID] && aux[AT_GID]==aux[AT_EGID]
		&& !aux[AT_SECURE]) return;

	struct pollfd pfd[3] = { {.fd=0}, {.fd=1}, {.fd=2} };
#ifdef SYS_poll
	__syscall(SYS_poll, pfd, 3, 0);
#else
	__syscall(SYS_ppoll, pfd, 3, &(struct timespec){0}, 0, _NSIG/8);
#endif
	for (i=0; i<3; i++) if (pfd[i].revents&POLLNVAL)
		if (__sys_open("/dev/null", O_RDWR)<0)
			a_crash();
	libc.secure = 1;
}

static void libc_start_init(void)
{
	_init();
	uintptr_t a = (uintptr_t)&__init_array_start;
	for (; a<(uintptr_t)&__init_array_end; a+=sizeof(void(*)()))
		(*(void (**)())a)();
}

weak_alias(libc_start_init, __libc_start_init);

bool __gg_enabled;
bool __gg_verbose;
char * __gg_dir;
char * __gg_thunk;
vector_InFile infiles;
vector_InDir indirs;

void __gg_init()
{
	if (getenv(GG_ENABLED_ENVAR)) {
		__gg_enabled = true;
		fprintf(stderr, "[gg] running in gg mode.\n");
	}
	else {
		return;
	}

	if (getenv(GG_VERBOSE_ENVAR)) {
		__gg_verbose = true;
		fprintf(stderr, "[gg] verbose is on.\n");
	}

	__gg_dir = getenv(GG_DIR_ENVAR);

	if ( __gg_dir == NULL ) {
		GG_ERROR("gg directory is not set, using default (.gg).\n");
		__gg_dir = ".gg";
	}
	else {
		GG_DEBUG("gg directory: %s\n", __gg_dir);
	}

	__gg_thunk = getenv(GG_THUNK_PATH_ENVAR);

  GG_DEBUG( "thunk filename: %s\n", __gg_thunk );

  if (__gg_thunk == NULL) {
    GG_ERROR( "cannot find thunk filename.\n" );
    return;
  }

	__gg_read_thunk();
}

int __libc_start_main(int (*main)(int,char **,char **), int argc, char **argv)
{
	char **envp = argv+argc+1;

	__init_libc(envp, argv[0]);
	__libc_start_init();

	__gg_init();

	/* Pass control to the application */
	exit(main(argc, argv, envp));
	return 0;
}
