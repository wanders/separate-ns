/*
    separate-ns: Run process in separate filesystem namespace

    Copyright (C) 2012 Anders Waldenborg <anders@0x63.nu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sched.h>
#include <sys/mount.h>

#define MAX_BINDS 32

#define streq(a,b) (!strcmp((a),(b)))

char stack[1048576];

struct childargs {
	int bindcount;
	struct {
		char *mountpoint;
		char *source;
	} binds[MAX_BINDS];
	char **argv;
	int origuid;
};

static int child(void *_a)
{
	struct childargs *args = _a;
	int i, r;

	for (i = 0; i < args->bindcount; i++) {
		r = mount(args->binds[i].source, args->binds[i].mountpoint, "bind", MS_BIND, "");
		if (r < 0) {
			perror("mount");
			return EXIT_FAILURE;
		}
	}

	/* error checking bitte! */
	r = setuid(args->origuid);
	if (r < 0) {
		perror("setuid");
		return EXIT_FAILURE;
	}
	r = seteuid(args->origuid);
	if (r < 0) {
		perror("seteuid");
		return EXIT_FAILURE;
	}

	setenv("SEPARATE_NS", "1", 1);

	execvp(args->argv[0], args->argv);
	perror("execvp");
	return 0;
}

static void parse_args(struct childargs *args, int argc, char **argv)
{
	int i, r;

	for (i = 1; i < argc - 1; i++) {
		if (streq(argv[i], "--bind")) {
			char *arg=argv[i+1];
			char *dest;
			char lnk[PATH_MAX];
			char mountpoint[PATH_MAX];

			if (args->bindcount >= MAX_BINDS) {
				fprintf (stderr, "Too many --bind arguments\n");
				exit(EXIT_FAILURE);
			}

			dest = strchr(arg, '=');
			*dest = 0;
			dest++;

			if (arg[0] == '.' || strchr(arg, '/')) {
				fprintf (stderr, "Invalid chars in mountpoint\n");
				exit(EXIT_FAILURE);
			}

			r = snprintf(lnk, sizeof(lnk), "/etc/separate-ns/%s", arg);
			if (r == -1 || r >= sizeof(lnk)) {
				fprintf (stderr, "Error building mountpoint path\n");
				exit(EXIT_FAILURE);
			}

			r = readlink(lnk, mountpoint, sizeof (mountpoint));
			if (r == -1) {
				perror("Readlink of mountpoint in /etc/separate-ns/");
				exit(EXIT_FAILURE);
			}

			args->binds[args->bindcount].mountpoint=mountpoint;
			args->binds[args->bindcount++].source=dest;

			i++;

		} else {
			break;
		}
	}

	args->argv = &argv[i];

}

int main(int argc, char **argv)
{
	struct childargs args = {0};
	int r, status;

	parse_args(&args, argc, argv);

	args.origuid = getuid();

	r = setuid(geteuid());
	if (r < 0) {
		perror("setuid");
		return EXIT_FAILURE;
	}

	r = clone(child, &stack[sizeof(stack)-1], CLONE_NEWNS | SIGCHLD, &args);
	if (r < 0) {
		perror("clone");
		return EXIT_FAILURE;
	}

	/* need no root no longer */
	setuid(args.origuid);
	seteuid(args.origuid);

	r = waitpid(r, &status, 0);
	if (r < 0) {
		perror("waitpid");
		return EXIT_FAILURE;
	}

	r = WEXITSTATUS(status);
	return r;
}
