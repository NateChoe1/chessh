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

int run_daemon(int sockfd) {
	signal(SIGPIPE, SIG_IGN);
	for (;;) {
		int p1fd, p2fd;
		int p1set[2], p2set[2];
		int pipe1[2], pipe2[2];
		int id1, id2;
		struct sockaddr_un addr;
		socklen_t addrlen;

		memset(&addr, 0, sizeof addr);
		addrlen = 0;

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

		/* I haven't used a do...while loop irl in MONTHS!! */
		do {
			p1fd = accept(sockfd, (struct sockaddr *) &addr, &addrlen);
		} while (p1fd < 0);

		/* If player 1 disconnects before player 2 joins, make player 2
		 * wait */
		for (;;) {
			p2fd = accept(sockfd, (struct sockaddr *) &addr, &addrlen);
			if (sendfds(p1fd, p1set, 2, &id1, sizeof id1) < 0) {
				close(p1fd);
				p1fd = p2fd;
				continue;
			}
			sendfds(p2fd, p2set, 2, &id2, sizeof id2);
			break;
		}
		close(p1fd);
		close(p2fd);
		close(pipe2[0]);
		close(pipe2[1]);
error2:
		close(pipe1[0]);
		close(pipe1[1]);
error1:
		continue;
	}
}
