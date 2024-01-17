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

#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>

#include <copyfd.h>
#include <daemon/runner.h>

static void match(int fd1, int fd2);

int run_daemon(int sockfd) {
	for (;;) {
		int p1fd, p2fd;
		struct sockaddr_un addr;
		socklen_t addrlen;

		p1fd = accept(sockfd, (struct sockaddr *) &addr, &addrlen);
		p2fd = accept(sockfd, (struct sockaddr *) &addr, &addrlen);

		match(p1fd, p2fd);
	}
}

static void match(int fd1, int fd2) {
	int pipe1[2], pipe2[2];
	int p1set[2], p2set[2];

	if (pipe(pipe1) == -1) {
		perror("pipe() failed");
		goto error1;
	}
	if (pipe(pipe2) == -1) {
		perror("pipe() failed");
		goto error2;
	}

	p1set[0] = pipe1[0];
	p1set[1] = pipe2[1];
	p2set[0] = pipe2[0];
	p2set[1] = pipe1[1];

	if (sendfds(fd1, p1set, 2, NULL, 0) < 0 ||
	    sendfds(fd2, p2set, 2, NULL, 0) < 0) {
		goto error3;
	}

	return;
error3:
	close(pipe2[0]);
	close(pipe2[1]);
error2:
	close(pipe1[0]);
	close(pipe1[1]);
error1:
	close(fd1);
	close(fd2);
}
