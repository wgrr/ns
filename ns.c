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

struct proc {
	int argc;
	char **argv;
};

static void procinit(void);
static int procstart(void*);

static noreturn void sysfatal(char*);
static noreturn void usage(void);
static void dupargv(struct proc *, char **);

static char procstack[8192];

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

static int
procstart(void *args)
{
	struct proc *p = args;

	procinit();

	execve(p->argv[0], p->argv+1, NULL);
	sysfatal("proc execve");
	return EXIT_FAILURE;
}

int
main(int argc, char **argv)
{
	int pid, pstatus;
	char *topofstack;
	struct proc *newp;

	if (argc < 2)
		usage();

	newp = malloc(sizeof *newp);
	if (newp == NULL)
		sysfatal("newp malloc");

	/* -1 because argv[0] is ignored */
	newp->argc = argc - 1;

	/* +1 for NULL, execve requirement */
	newp->argv = calloc(newp->argc + 1, sizeof *newp->argv);
	if (newp->argv == NULL)
		sysfatal("newp->argv malloc");

	/* skip argv[0] */
	dupargv(newp, argv+1);

	topofstack = procstack + sizeof procstack;
	/* TODO: systemd crap requiments to really isolate mounts
	 * unshare(CLONE_NEWNS);
	if (mount("none", "/", NULL, MS_REC|MS_PRIVATE, NULL) < 0)
		sysfatal("mount");
	*/
	pid = clone(&procstart, topofstack,
			CLONE_NEWUSER|
			CLONE_NEWPID| /* isolate processes */
			CLONE_NEWUTS| /* isolate hostname */
			CLONE_NEWNS|  /* isolate mounts */
			CLONE_VFORK|
			CLONE_VM|
			SIGCHLD,
			newp);
	if (pid < 0)
		sysfatal("clone");
	if (waitpid(pid, &pstatus, 0) < 0)
		sysfatal("waitpid");

	printf("proc %d exited with %d\n", pid, ((unsigned)pstatus >> 8)&0xff);
	fflush(stdout);
	return 0;
}

static void
dupargv(struct proc *p, char **src)
{
	int i;

	for (i = 0; i < p->argc; i++) {
		p->argv[i] = malloc(strlen(src[i])+1);
		if (p->argv[i] ==  NULL)
			sysfatal("dupargv malloc");
		strcpy(p->argv[i], src[i]);
	}
	p->argv[p->argc] = NULL;
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
