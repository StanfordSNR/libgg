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

__gg_struct __gg;

void __gg_init()
{
	if (getenv(GG_ENABLED_ENVAR)) {
		__gg.enabled = true;
		vector_InData_init( &__gg.indata );
		vector_DummyDir_init( &__gg.indirs );
		vector_AllowedFile_init( &__gg.allowed_files );
		vector_Output_init( &__gg.outputs );
		GG_DEBUG( "running in gg mode.\n" );
	}
	else {
		__gg.enabled = false;
		__gg.verbose = false;
		__gg.dir = NULL;
		__gg.manifest_file = NULL;
		__gg.thunk_file = NULL;
		return;
	}

	if (getenv(GG_VERBOSE_ENVAR)) {
		__gg.verbose = true;
		GG_DEBUG( "verbose is on.\n" );
	}

	__gg.dir = getenv(GG_DIR_ENVAR);

	if ( __gg.dir == NULL ) {
		GG_WARNING("gg directory is not set, using default (.gg).\n");
		__gg.dir = ".gg";
	}
	else {
		GG_DEBUG("gg directory: %s\n", __gg.dir);
	}

	__gg.thunk_file = getenv(GG_THUNK_PATH_ENVAR);
	if (__gg.thunk_file) {
		GG_DEBUG( "reading thunk file: %s\n", __gg.thunk_file );
		__gg_read_thunk();
	}

	__gg.manifest_file = getenv(GG_MANIFEST_ENVAR);
	if (__gg.manifest_file) {
		GG_DEBUG( "reading manfiest file: %s\n", __gg.manifest_file );
		__gg_read_manifest();
	}
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
