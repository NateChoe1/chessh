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
#include <stdlib.h>

#include <getopt.h>

#include <legal.h>
#include <client/sock.h>
#include <client/perft.h>
#include <client/runner.h>

struct client_args {
	char *dir;
	char *user;
	char *pass;

	int perft;
	char *start_pos;
};

static void parse_args(int argc, char *argv[], struct client_args *ret);
static void print_help(char *progname);

int main(int argc, char *argv[]) {
	struct client_args args;
	char sock_path[4096];
	int sock_fd;

	parse_args(argc, argv, &args);

	if (args.perft != -1) {
		return run_perft(args.perft, args.start_pos);
	}

	snprintf(sock_path, sizeof sock_path, "%s/matchmaker", args.dir);
	sock_path[sizeof sock_path - 1] = '\0';
	if ((sock_fd = unix_connect(sock_path)) < 0) {
		return 1;
	}

	return run_client(sock_fd);
}

static void parse_args(int argc, char *argv[], struct client_args *ret) {
	ret->dir = ret->user = ret->pass = NULL;
	ret->perft = -1;
	ret->start_pos = NULL;

	for (;;) {
		int opt = getopt(argc, argv, "hld:u:p:t:i:");
		switch (opt) {
		case -1:
			goto got_args;
		case 'h':
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		case 'l':
			print_legal();
			exit(EXIT_SUCCESS);
		case 'd':
			ret->dir = optarg;
			break;
		case 'u':
			ret->user = optarg;
			break;
		case 'p':
			ret->pass = optarg;
			break;
		case 't':
			ret->perft = atoi(optarg);
			break;
		case 'i':
			ret->start_pos = optarg;
			break;
		default:
			print_help(argv[0]);
			exit(EXIT_FAILURE);
		}
	}
got_args:

	if (ret->perft != -1) {
		return;
	}

	if (ret->dir == NULL || ret->user == NULL || ret->pass == NULL) {
		fprintf(stderr, "%s: missing required argument\n", argv[0]);
		print_help(argv[0]);
		exit(EXIT_FAILURE);
	}
}

static void print_help(char *progname) {
	printf("Usage: %s -d [dir] -u [username] -p [password]\n"
	       "OTHER FLAGS:\n"
	       "  -h: Show this help and quit\n"
	       "  -l: Show a legal notice and quit\n"
	       "  -t [level]: Run a perft test with [level] levels\n"
	       "  -i [start]: Use [start] as the starting position for the perft test\n",
	       progname);
}
