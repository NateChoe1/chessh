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
static void print_letters(int player);
static void print_board(struct game *game, int player);

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

	print_board(game, pid);

	for (int i = 0;; i = (i+1) % 2) {
		char cmd[1024];
		ssize_t cmdlen;
again:
		cmdlen = read(rotation[i], cmd, sizeof cmd);
		errno = 0;
		cmd[cmdlen-1] = '\0';
		if (parse_cmd(game, cmd) < 0) {
			fprintf(stderr, "Invalid move received: %s\n", cmd);
			if (rotation[i] == 0) {
				print_board(game, pid);
				goto again;
			}
		}
		if (rotation[i] == 0 &&
		    write(fds[1], cmd, cmdlen) < cmdlen) {
			perror("short write");
		}
		print_board(game, pid);
	}

	return 0;
}

static int parse_cmd(struct game *game, char *move) {
	struct move move_s;
	move_s.c_i = move[0] - 'a';
	move_s.r_i = 8 - (move[1] - '0');
	move_s.c_f = move[2] - 'a';
	move_s.r_f = 8 - (move[3] - '0');
	return make_move(game, &move_s);
}

static void print_letters(int player) {
	if (player == 0) {
		puts("  a b c d e f g h");
	}
	else {
		puts("  h g f e d c b a");
	}
}

static void print_board(struct game *game, int player) {
	print_letters(player);
	for (int i = 0; i < 8; ++i) {
		int row = player == 0 ? i : 7-i;
		printf("%d ", 8 - row);
		for (int j = 0; j < 8; ++j) {
			int col = player == 0 ? j : 7-j;
			struct piece *piece = &game->board.board[row][col];
			char *sym = "  ";

			fputs((row+col)%2==0 ? "\33[45":"\33[44", stdout);
			fputs(piece->player==0 ? ";37m":";30m", stdout);
			switch (piece->type) {
			case ROOK:   sym = "♜ "; break;
			case KNIGHT: sym = "♞ "; break;
			case BISHOP: sym = "♝ "; break;
			case QUEEN:  sym = "♛ "; break;
			case KING:   sym = "♚ "; break;
			case PAWN:   sym = "♟︎ "; break;
			case EMPTY:  sym = "  "; break;
			}
			fputs(sym, stdout);
		}
		printf("\33[0m %d\n", 8 - row);
	}
	print_letters(player);
}
