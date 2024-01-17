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

#include <poll.h>
#include <unistd.h>

#include <copyfd.h>
#include <client/runner.h>

static inline void copy(int src, int dst) {
	ssize_t len;
	char buff[1024];
	len = read(src, buff, sizeof buff);
	if (write(dst, buff, len) < len) {
		fputs("Warning: short write\n", stderr);
	}
}

int run_client(int sock_fd) {
	int fds[2];
	struct pollfd pollfds[2];
	int recvlen;

	if ((recvlen = recvfds(sock_fd, fds, sizeof fds / sizeof *fds, NULL, 0, NULL))
			< 2) {
		return 1;
	}

	pollfds[0].fd = fds[0];
	pollfds[0].events = POLLIN;
	pollfds[1].fd = 0;
	pollfds[1].events = POLLIN;

	for (;;) {
		poll(pollfds, 2, -1);

		if (pollfds[0].revents & POLLIN) {
			copy(fds[0], 1);
		}
		if (pollfds[1].revents & POLLIN) {
			copy(0, fds[1]);
		}
	}

	return 0;
}
