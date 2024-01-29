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

/* The goal of chessh has always been to make a chess program that works on as
 * many devices as possible. That's why I used plaintext instead of HTML and
 * Javascript. I don't want our friends at MIT using a PDP-11 with a literal
 * typewriter and telnet to access the internet to get left out, so this
 * frontend (which doesn't use ncurses) will be supported for the forseeable
 * future. */

#include <stdio.h>
#include <stdlib.h>

#include <readline/readline.h>

#include <util.h>

#include <client/chess.h>
#include <client/frontend.h>

struct aux {
	char **piecesyms_white;
	char **piecesyms_black;
};

static char *get_move(void *aux, enum player player);
static void report_error(void *aux, int code);
static void report_msg(void *aux, char *msg);
static void print_letters(enum player player);
static void display_board(void *aux, struct game *game, enum player player);
static void free_frontend(struct frontend *this);

struct frontend *new_text_frontend(char **piecesyms_white, char **piecesyms_black) {
	struct frontend *ret;
	struct aux *aux;
	if ((ret = malloc(sizeof *ret)) == NULL) {
		return NULL;
	}
	if ((aux = malloc(sizeof *aux)) == NULL) {
		free(ret);
		return NULL;
	}
	ret->get_move      = get_move;
	ret->report_error  = report_error;
	ret->report_msg    = report_msg;
	ret->display_board = display_board;
	ret->free          = free_frontend;
	ret->aux           = aux;
	aux->piecesyms_white = piecesyms_white;
	aux->piecesyms_black = piecesyms_black;
	return ret;
}

static char *get_move(void *aux, enum player player) {
	char *move;
	UNUSED(aux);
	UNUSED(player);
	move = readline("Your move: ");
	if (move == NULL) {
		return NULL;
	}
	return move;
}

static void report_error(void *aux, int code) {
	UNUSED(aux);
	switch (code) {
	case MISSING_PROMOTION:
		puts("You're missing a promotion!");
		break;
	default:
		puts("Warning: Received unknown error from backend");
		break;
	}
}

static void report_msg(void *aux, char *msg) {
	UNUSED(aux);
	puts(msg);
}

static void print_letters(enum player player) {
	if (player == WHITE) {
		puts("  a b c d e f g h");
	}
	else {
		puts("  h g f e d c b a");
	}
}

static void display_board(void *aux, struct game *game, enum player player) {
	struct aux *dissected = (struct aux *) aux;
	print_letters(player);
	for (int i = 0; i < 8; ++i) {
		int row = player == 0 ? i : 7-i;
		printf("%d ", 8 - row);
		for (int j = 0; j < 8; ++j) {
			int col = player == 0 ? j : 7-j;
			struct piece *piece = &game->board.board[row][col];
			char **piecesyms = (piece->type == EMPTY || piece->player == WHITE) ? dissected->piecesyms_white : dissected->piecesyms_black;
			char *sym = piecesyms[piece->type];

			fputs(sym, stdout);
		}
		printf(" %d\n", 8 - row);
	}
	print_letters(player);
}

static void free_frontend(struct frontend *this) {
	free(this->aux);
	free(this);
}
