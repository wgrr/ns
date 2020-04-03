#define _GNU_SOURCE

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdnoreturn.h>

#include <errno.h>
#include <sched.h>
#include <string.h>
#include <signal.h>

#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>

static void procinit(void);
static noreturn int procstart(void*);

static noreturn void sysfatal(char*);
static noreturn void usage(void);

static char procstack[8192 * 4];

static void
procinit()
{
	if (sethostname("proc", strlen("proc")) < 0)
		sysfatal("proc sethostname");
	if (chroot("./alpine_3.11") < 0)
		sysfatal("proc chroot");
	if (chdir("/") < 0)
		sysfatal("proc chdir");
	if (mount("proc", "proc", "proc", 0, NULL) < 0)
		sysfatal("proc mount");
}

static noreturn int
procstart(void *a)
{
	char **argv = a;

	procinit();
	execve(argv[0], argv+1, NULL);
	do exit(EXIT_FAILURE); while(1);
}

/* missing: network namespace setup,
 *          user/group namespace setup
 *          cgroup namespace setup
 *          devtmpfs /dev
 *          sysfs /sys
 */
int
main(int argc, char **argv)
{
	int pid, pstatus;
	char *topofstack, **newargv;

	if (argc < 2)
		usage();

	/* skip argv[0] */
	newargv = argv + 1;

	topofstack = procstack + sizeof procstack;

	/* systemd crap shares NEWNS by default to all processes in,
	 * order to isolate mounts uncomment the following lines.
	 * NOTE: it requires root previleges.
	if (unshare(CLONE_NEWNS) < 0)
		sysfatal("unshare");
	if (mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL) < 0)
		sysfatal("mount");
	 */

	pid = clone(&procstart, topofstack,
			CLONE_NEWCGROUP| /* hides parent cgroup from child */
			CLONE_NEWUSER|   /* hides parent user from child */
			CLONE_NEWIPC|    /* hides parent ipc from child */
			CLONE_NEWNET|    /* hides parent network stack from child */
			CLONE_NEWPID|    /* hides parent processes from child */
			CLONE_NEWUTS|    /* hides parent hostname from child */
			CLONE_NEWNS|     /* hides parent mounts from child */
			CLONE_VFORK|
			CLONE_VM|
			SIGCHLD,
			newargv);

	if (pid < 0)
		sysfatal("clone");
	if (waitpid(pid, &pstatus, 0) < 0)
		sysfatal("waitpid");

	printf("proc %d exited with %d\n", pid, ((unsigned)pstatus >> 8)&0xff);
	fflush(stdout);
	return 0;
}

static noreturn void 
sysfatal(char *ctx)
{
	fprintf(stdout, "ns: %s: %s\n", ctx, strerror(errno));
	fflush(stdout);
	do exit(EXIT_FAILURE); while(1);
}

static noreturn void 
usage()
{
	fprintf(stderr, "usage: ns cmd [ args ... ]\n");
	do exit(EXIT_FAILURE); while(1);
}
