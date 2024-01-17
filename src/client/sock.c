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

#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>

#include <client/sock.h>

int unix_connect(char *path) {
	int ret;
	struct sockaddr_un addr;

	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, path, sizeof addr.sun_path - 1);
	addr.sun_path[sizeof addr.sun_path - 1] = '\0';

	if ((ret = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("socket() failed");
		return -1;
	}

	if (connect(ret, (struct sockaddr *) &addr, (socklen_t) sizeof addr)
			< 0) {
		perror("connect() failed");
		close(ret);
		return -1;
	}

	return ret;
}
