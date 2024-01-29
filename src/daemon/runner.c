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

#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/socket.h>

#include <copyfd.h>
#include <daemon/runner.h>

static void match(int fd1, int fd2);

int run_daemon(int sockfd) {
	/* TODO: Make this more sophisticated */
	signal(SIGPIPE, SIG_IGN);
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

	/* Buffers to send player ids */
	int id1, id2;

	/* Randomize white and black  */
	if (random() % 2 == 0) {
		id1 = 0;
		id2 = 1;
	}
	else {
		id1 = 1;
		id2 = 0;
	}

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

	sendfds(fd1, p1set, 2, &id1, sizeof id1);
	sendfds(fd2, p2set, 2, &id2, sizeof id2);

	close(pipe2[0]);
	close(pipe2[1]);
error2:
	close(pipe1[0]);
	close(pipe1[1]);
error1:
	close(fd1);
	close(fd2);
}
