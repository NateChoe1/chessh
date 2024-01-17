/* chessh - chess over ssh
 * Copyright (C) 2024  Nate Choe <nate@natechoe.dev>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * */

#include <stdio.h>
#include <getopt.h>

#include <legal.h>
#include <daemon/sock.h>
#include <daemon/runner.h>

struct daemon_args {
	char *dir;
};

static int parse_args(int argc, char *argv[], struct daemon_args *ret);
static void print_help(char *progname);

int main(int argc, char *argv[]) {
	struct daemon_args args;
	int arg_ret;
	char sock_path[4096];
	int sock_fd;

	if ((arg_ret = parse_args(argc, argv, &args))) {
		return arg_ret;
	}

	snprintf(sock_path, sizeof sock_path, "%s/matchmaker", args.dir);
	sock_path[sizeof sock_path - 1] = '\0';
	if ((sock_fd = setup_unix_sock(sock_path)) < 0) {
		return 1;
	}

	return run_daemon(sock_fd);
}

static int parse_args(int argc, char *argv[], struct daemon_args *ret) {
	ret->dir = NULL;

	for (;;) {
		int opt = getopt(argc, argv, "hld:");
		switch (opt) {
		case -1:
			goto got_args;
		case 'h':
			print_help(argv[0]);
			return 0;
		case 'l':
			print_legal();
			return 0;
		case 'd':
			ret->dir = optarg;
			break;
		default:
			print_help(argv[0]);
			return 1;
		}
	}
got_args:

	if (ret->dir == NULL) {
		print_help(argv[0]);
		return 1;
	}

	return 0;
}

static void print_help(char *progname) {
	printf("Usage: %s -d [dir]\n"
	       "OTHER FLAGS:\n"
	       "  -h: Show this help and quit\n"
	       "  -l: Show a legal notice and quit\n",
	       progname);
}
