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
#include <client/frontend.h>

static bool ask_user(char *prompt, bool def_answer);
static int parse_op_move(struct game *game, int fd);
static int get_player_move(struct frontend *frontend, struct game *game, int peer);

static char **piecesyms_white;
static char **piecesyms_black;

/* I know these technically aren't emoji, but I don't care. */
static char *emojisyms_white[] = {
	[ROOK]   = "♜ ",
	[KNIGHT] = "♞ ",
	[BISHOP] = "♝ ",
	[QUEEN]  = "♛ ",
	[KING]   = "♚ ",
	[PAWN]   = "♟︎ ",
	[EMPTY]  = "  ",
};
static char *emojisyms_black[] = {
	[ROOK]   = "♖ ",
	[KNIGHT] = "♘ ",
	[BISHOP] = "♗ ",
	[QUEEN]  = "♕ ",
	[KING]   = "♔ ",
	[PAWN]   = "♙ ",
	[EMPTY]  = "  ",
};
static char *portsyms_white[] = {
	[ROOK]   = "R ",
	[KNIGHT] = "N ",
	[BISHOP] = "B ",
	[QUEEN]  = "Q ",
	[KING]   = "K ",
	[PAWN]   = "P ",
	[EMPTY]  = "  ",
};
static char *portsyms_black[] = {
	[ROOK]   = "r ",
	[KNIGHT] = "n ",
	[BISHOP] = "b ",
	[QUEEN]  = "q ",
	[KING]   = "k ",
	[PAWN]   = "p ",
	[EMPTY]  = "  ",
};

int run_client(int sock_fd) {
	int fds[2];
	int pid;
	enum player player;
	int recvlen;
	ssize_t pidlen;
	struct game *game;
	struct frontend *frontend;
	char *end_msg;

	puts("Believe it or not, plaintext isn't actually that portable. I have to ask some questions:");
	if (ask_user("Do you see a pawn symbol? ♟︎ ", true)) {
		piecesyms_white = emojisyms_white;
		piecesyms_black = emojisyms_black;
		/* This varies based on your terminal's background color */
		if (!ask_user("Does that pawn symbol look white?", true)) {
			char **backup = piecesyms_white;
			piecesyms_white = piecesyms_black;
			piecesyms_black = backup;
		}
	}
	else {
		piecesyms_white = portsyms_white;
		piecesyms_black = portsyms_black;
	}

	frontend = NULL;
	if (ask_user("Can you use ncurses? (If you don't know what this means, say 'y')", true)) {
		frontend = new_curses_frontend(piecesyms);
	}
	else {
		frontend = new_text_frontend(piecesyms);
	}
	if (frontend == NULL) {
		puts("Failed to initialize frontend, quitting");
		return 1;
	}

	if ((recvlen = recvfds(sock_fd, fds, sizeof fds / sizeof *fds, &pid, sizeof pid, &pidlen)) < 2 ||
	    pidlen < (ssize_t) sizeof pid) {
		return 1;
	}
	player = pid == 0 ? WHITE : BLACK;

	game = new_game();

	frontend->display_board(frontend->aux, game, player);
	for (;;) {
		int move_code;
		int curr_player = get_player(game) == WHITE ? 0:1;
		if (curr_player == pid) {
			move_code = get_player_move(frontend, game, fds[1]);
		}
		else {
			frontend->report_msg(frontend->aux, "Waiting on opponent's move");
			move_code = parse_op_move(game, fds[0]);
		}

		switch (move_code) {
		case WHITE_WIN:
			end_msg = "White wins!";
			goto end;
		case BLACK_WIN:
			end_msg = "Black wins!";
			goto end;
		case FORCED_DRAW:
			end_msg = "It's a draw!";
			goto end;
		case IO_ERROR:
			end_msg = "I/O error";
			goto end;
		}
		if (move_code < 0) {
			end_msg = "An unknown error has occured, ending game";
			goto end;
		}

		frontend->display_board(frontend->aux, game, player);
	}
end:
	frontend->report_msg(frontend->aux, end_msg);
	sleep(3);
	frontend->free(frontend);
	return 0;
}

static bool ask_user(char *prompt, bool def_answer) {
	char readline_prompt[256];
	snprintf(readline_prompt, sizeof readline_prompt, "%s %s",
			prompt, def_answer ? "[Y/n]: " : "[y/N]: ");
	for (;;) {
		char *response;
		response = readline(readline_prompt);
		if (response == NULL) {
			printf("Failed to read response, assuming '%s'",
					def_answer ? "yes":"no");
			return def_answer;
		}
		for (int i = 0; response[i] != '\0'; ++i) {
			response[i] = tolower(response[i]);
		}
		if (strcmp(response, "y") == 0 || strcmp(response, "yes") == 0 || response[0] == '\0') {
			return true;
		}
		if (strcmp(response, "n") == 0 || strcmp(response, "no") == 0) {
			return false;
		}
		puts("Please answer either 'y' or 'n'");
	}
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
	if ((code = parse_move(game, buff))) {
		return code;
	}
	return 0;
}

static int get_player_move(struct frontend *frontend, struct game *game, int peer) {
	char *move;
	int move_code;
	frontend->report_msg(frontend->aux, "Make your move");
	for (;;) {
		move = frontend->get_move(frontend->aux);
		if (move == NULL) {
			return IO_ERROR;
		}
		move_code = parse_move(game, move);
		switch (move_code) {
		case ILLEGAL_MOVE:
			frontend->report_msg(frontend->aux, "Illegal move!");
			continue;
		case MISSING_PROMOTION:
			frontend->report_error(frontend->aux, MISSING_PROMOTION);
			continue;
		}
		break;
	}
	if (write(peer, move, strlen(move)+1) < 5) {
		return IO_ERROR;
	}
	return move_code;
}
