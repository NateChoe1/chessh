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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <readline/readline.h>

#include <copyfd.h>
#include <client/chess.h>
#include <client/runner.h>

#define ERR_FATAL -2
#define ERR_INVALID -1

static int parse_cmd(struct game *game, char *move);
static int parse_user_move(struct game *game, int peerfd);
static int parse_op_move(struct game *game, int fd);
static void print_letters(int player);
static void print_board(struct game *game, int player);

int run_client(int sock_fd) {
	int fds[2];
	int pid;
	int recvlen;
	ssize_t pidlen;
	struct game *game;

	if ((recvlen = recvfds(sock_fd, fds, sizeof fds / sizeof *fds,
					&pid, sizeof pid, &pidlen)) < 2 ||
	    pidlen < (ssize_t) sizeof pid) {
		return 1;
	}

	game = new_game();

	print_board(game, pid);
	for (int curr_player = 0;; curr_player = !curr_player) {
		int move_code;
		if (curr_player == pid) {
			while ((move_code = parse_user_move(game, fds[1]))) {
				switch (move_code) {
				case ERR_FATAL:
					exit(EXIT_FAILURE);
				case ERR_INVALID:
					fputs("Invalid move\n", stderr);
					break;
				}
			}
		}
		else {
			puts("Waiting on opponent's move");
			while (parse_op_move(game, fds[0])) ;
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

static int parse_user_move(struct game *game, int peerfd) {
	char *move;
	move = readline("Your move: ");
	if (move == NULL) {
		return ERR_FATAL;
	}
	for (int i = 0; i < 4; ++i) {
		if (move[i] == '\0') {
			return ERR_INVALID;
		}
	}
	if (move[4] != '\0') {
		return ERR_INVALID;
	}
	if (parse_cmd(game, move)) {
		return ERR_INVALID;
	}
	if (write(peerfd, move, 5) < 5) {
		return 0;
	}
	return 0;
}

static int parse_op_move(struct game *game, int fd) {
	char buff[5];
	size_t seen;
	for (seen = 0; seen < sizeof buff;) {
		ssize_t thisread;
		thisread = read(fd, buff, sizeof buff - seen);
		if (thisread < 0) {
			return ERR_FATAL;
		}
		seen += thisread;
	}
	if (buff[sizeof buff - 1] != '\0') {
		return ERR_FATAL;
	}
	if (parse_cmd(game, buff)) {
		return ERR_INVALID;
	}
	return 0;
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
