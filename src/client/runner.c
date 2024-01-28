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
#include <ctype.h>
#include <stdlib.h>

#include <unistd.h>

#include <readline/readline.h>

#include <copyfd.h>
#include <client/chess.h>
#include <client/runner.h>

static int parse_cmd(struct game *game, char *move);
static int parse_user_move(struct game *game, int peerfd);
static int parse_op_move(struct game *game, int fd);
static void print_letters(int player);
static void print_board(struct game *game, int player);

static char **piecesyms;
static char *emojisyms[] = {
	[ROOK]   = "♜ ",
	[KNIGHT] = "♞ ",
	[BISHOP] = "♝ ",
	[QUEEN]  = "♛ ",
	[KING]   = "♚ ",
	[PAWN]   = "♟︎ ",
	[EMPTY]  = "  ",
};
static char *portsyms[] = {
	[ROOK]   = "R ",
	[KNIGHT] = "N ",
	[BISHOP] = "B ",
	[QUEEN]  = "Q ",
	[KING]   = "K ",
	[PAWN]   = "P ",
	[EMPTY]  = "  ",
};

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

	for (;;) {
		char *charset;
		charset = readline("Which piece symbols would you like? [unicode/ascii]: ");
		if (charset == NULL) {
			puts("Failed to read response, assuming ascii");
			piecesyms = portsyms;
			break;
		}
		for (int i = 0; charset[i] != '\0'; ++i) {
			charset[i] = tolower(charset[i]);
		}
		if (strcmp(charset, "unicode") == 0) {
			piecesyms = emojisyms;
			break;
		}
		if (strcmp(charset, "ascii") == 0) {
			piecesyms = portsyms;
			break;
		}
		puts("Invalid response");
	}

	game = new_game();

	/* TODO: Remove me */
	//if (init_game(game, "rnb1kb1r/pp3ppp/2p5/4q3/4n3/3Q4/PPPB1PPP/2KR1BNR w kq - 0 1") == -1) {
	if (init_game(game, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") == -1) {
		puts("I suck at coding");
		return 1;
	}

	print_board(game, pid);
	for (int curr_player = 0;; curr_player = !curr_player) {
		int move_code;
		if (curr_player == pid) {
			for (;;) {
				move_code = parse_user_move(game, fds[1]);
				switch (move_code) {
				case ILLEGAL_MOVE:
					puts("Illegal move!");
					continue;
				case MISSING_PROMOTION:
					puts("Missing promotion!");
					continue;
				}
				break;
			}
		}
		else {
			puts("Waiting on opponent's move");
			move_code = parse_op_move(game, fds[0]);
		}

		switch (move_code) {
		case WHITE_WIN:
			puts("White wins!");
			goto end;
		case BLACK_WIN:
			puts("Black wins!");
			goto end;
		case FORCED_DRAW:
			puts("It's a draw!");
			goto end;
		case IO_ERROR:
			puts("I/O error");
			goto end;
		}
		if (move_code < 0) {
			puts("An unknown error has occured, ending game");
			goto end;
		}

		print_board(game, pid);
	}
end:
	puts("Game over!");
	return 0;
}

static int parse_cmd(struct game *game, char *move) {
	struct move move_s;
	if (strlen(move) < 4) {
		return ILLEGAL_MOVE;
	}
	move_s.c_i = tolower(move[0]) - 'a';
	move_s.r_i = 8 - (move[1] - '0');
	move_s.c_f = tolower(move[2]) - 'a';
	move_s.r_f = 8 - (move[3] - '0');
	move_s.promotion = EMPTY;
	switch (tolower(move[4])) {
	case 'n': move_s.promotion = KNIGHT; break;
	case 'q': move_s.promotion = QUEEN;  break;
	case 'r': move_s.promotion = ROOK;   break;
	case 'b': move_s.promotion = BISHOP; break;
	}
	return make_move(game, &move_s);
}

static int parse_user_move(struct game *game, int peerfd) {
	char *move;
	int code = 0;
	move = readline("Your move: ");
	if (move == NULL) {
		return IO_ERROR;
	}
	if ((code = parse_cmd(game, move))) {
		switch (code) {
		case NONFATAL_ERROR:
			return code;
		default:
			break;
		}
	}
	if (write(peerfd, move, 5) < 5) {
		return IO_ERROR;
	}
	return code;
}

static int parse_op_move(struct game *game, int fd) {
	char buff[1024];
	int code;
	ssize_t read_len;
	if ((read_len = read(fd, buff, sizeof buff)) < 5) {
		return IO_ERROR;
	}
	if (buff[read_len-1] != '\0') {
		return IO_ERROR;
	}
	if ((code = parse_cmd(game, buff))) {
		return code;
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
			char *sym = piecesyms[piece->type];

			fputs((row+col)%2==0 ? "\33[45":"\33[44", stdout);
			fputs(piece->player==0 ? ";37m":";30m", stdout);
			fputs(sym, stdout);
		}
		printf("\33[0m %d\n", 8 - row);
	}
	print_letters(player);
}
