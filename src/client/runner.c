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
#include <errno.h>

#include <unistd.h>

#include <copyfd.h>
#include <client/chess.h>
#include <client/runner.h>

static int parse_cmd(struct game *game, char *move);
static void print_board(struct game *game);

int run_client(int sock_fd) {
	int fds[2];
	int pid;
	int recvlen;
	ssize_t pidlen;
	struct game *game;
	int rotation[2];

	if ((recvlen = recvfds(sock_fd, fds, sizeof fds / sizeof *fds,
					&pid, sizeof pid, &pidlen)) < 2 ||
	    pidlen < (ssize_t) sizeof pid) {
		return 1;
	}

	if (pid == 0) {
		rotation[0] = 0;
		rotation[1] = fds[0];
	}
	else {
		rotation[0] = fds[0];
		rotation[1] = 0;
	}

	game = new_game();

	print_board(game);

	for (int i = 0;; i = (i+1) % 2) {
		char cmd[1024];
		ssize_t cmdlen;
		cmdlen = read(rotation[i], cmd, sizeof cmd);
		errno = 0;
		if (rotation[i] == 0 &&
		    write(fds[1], cmd, cmdlen) < cmdlen) {
			perror("short write");
		}
		cmd[cmdlen-1] = '\0';
		if (parse_cmd(game, cmd) < 0) {
			fprintf(stderr, "Invalid move received: %s\n", cmd);
		}
		print_board(game);
	}

	return 0;
}

static int parse_cmd(struct game *game, char *move) {
	struct move move_s;
	move_s.r_i = move[0] - '0';
	move_s.c_i = move[1] - '0';
	move_s.r_f = move[2] - '0';
	move_s.c_f = move[3] - '0';
	return make_move(game, &move_s);
}

static void print_board(struct game *game) {
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			struct piece *piece = &game->board.board[i][j];
			char filler = (i+j)%2==0 ? ' ':'#';
			char pc = '.';
			switch (piece->type) {
			case ROOK: pc = 'r'; break;
			case KNIGHT: pc = 'n'; break;
			case BISHOP: pc = 'b'; break;
			case QUEEN: pc = 'q'; break;
			case KING: pc = 'k'; break;
			case PAWN: pc = 'p'; break;
			case EMPTY: pc = filler; break;
			}
			putchar(filler);
			putchar(pc);
		}
		putchar('\n');
	}
}
