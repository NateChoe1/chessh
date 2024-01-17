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

/* These functions were taken pretty much directly from my web server swebs
 *
 * https://github.com/NateChoe1/swebs, src/util.c, around line 176
 *
 * Of course, that code was also taken from somewhere else that I can't remember
 * ;)
 * */

#include <string.h>
#include <alloca.h>
#include <sys/socket.h>

#include <copyfd.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int sendfds(int dest, int fds[], int fdcount, void *data, size_t len) {
	struct msghdr msg;
	struct cmsghdr *cmsg;
	char iobuf[1];
	struct iovec io;
	size_t an_buff_len = CMSG_ALIGN(CMSG_SPACE(fdcount * sizeof *fds));
	char *an_buff = alloca(an_buff_len);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	if (data == NULL) {
		io.iov_base = iobuf;
		io.iov_len = sizeof iobuf;
	}
	else {
		io.iov_base = data;
		io.iov_len = len;
	}
	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = an_buff;
	msg.msg_controllen = an_buff_len;
	cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(fdcount * sizeof *fds);
	memcpy(CMSG_DATA(cmsg), fds, fdcount * sizeof *fds);
	return sendmsg(dest, &msg, 0) == -1 ? -1 : 0;
}

int recvfd(int src, int fds[], int fdcount,
		void *data, size_t len, ssize_t *received_data) {
	struct msghdr msg;
	struct cmsghdr *cmsg;
	struct iovec iov;
	ssize_t nr;
	char buf[1];
	size_t buff_len = CMSG_ALIGN(CMSG_SPACE(fdcount * sizeof *fds));
	char *buff = alloca(buff_len);

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	if (data == NULL) {
		data = buf;
		len = sizeof buf;
	}
	iov.iov_base = data;
	iov.iov_len = len;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	msg.msg_control = buff;
	msg.msg_controllen = sizeof buff;
	nr = recvmsg(src, &msg, 0);
	if (nr < 0) {
		return -1;
	}
	*received_data = nr;

	cmsg = CMSG_FIRSTHDR(&msg);
	if ((cmsg == NULL || cmsg->cmsg_len != CMSG_LEN(len)) ||
	    (cmsg->cmsg_level != SOL_SOCKET) ||
	    (cmsg->cmsg_type != SCM_RIGHTS)) {
		return -1;
	}

	memcpy(fds, CMSG_DATA(cmsg), fdcount * sizeof *fds);
	return cmsg->cmsg_len / sizeof *fds;
}
