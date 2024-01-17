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

#ifndef HAVE_COPYFD
#define HAVE_COPYFD

#include <stddef.h>
#include <sys/types.h>

/* returns 0 on success, -1 on failure */
extern int sendfds(int dest, int fds[], int fdcount, void *data, size_t len);

/* returns the number of received fds on success, -1 on failure */
extern int recvfds(int src,  int fds[], int fdcount,
		void *data, size_t len, ssize_t *received_data);

#endif
