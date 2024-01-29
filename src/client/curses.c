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

#include <stdlib.h>
#include <stdbool.h>

#include <curses.h>

#include <util.h>

#include <client/frontend.h>

struct aux {
	struct move move;
	char **piecesyms_white;
	char **piecesyms_black;
	bool has_color;
	/* always foreground, background. wb has a white foreground and a black
	 * background. keep in mind these aren't literally #000 and #fff. */
};

/* WB = white foreground, black background */
#define WW 1
#define WB 2
#define BW 3
#define BB 4

static void draw_piece(void *aux, struct game *game, int row, int col);
static void display_board(void *aux, struct game *game, enum player player);
static void free_frontend(struct frontend *this);
static void reset_move(struct move *move);

/* TODO: Implement this */
extern struct frontend *new_curses_frontend(char **piecesyms_white, char **piecesyms_black) {
	struct frontend *ret;
	struct aux *aux;

	if ((ret = malloc(sizeof *ret)) == NULL) {
		return NULL;
	}
	if ((aux = malloc(sizeof *aux)) == NULL) {
		return NULL;
	}

	initscr();
	cbreak();
	noecho();

	ret->display_board = display_board;
	ret->free = free_frontend;
	ret->aux = aux;
	reset_move(&aux->move);
	aux->piecesyms_white = piecesyms_white;
	aux->piecesyms_black = piecesyms_black;
	if (has_colors()) {
		aux->has_color = true;
		start_color();
		init_pair(WW, COLOR_WHITE, COLOR_MAGENTA);
		init_pair(WB, COLOR_WHITE, COLOR_BLUE);
		init_pair(BW, COLOR_BLACK, COLOR_MAGENTA);
		init_pair(BB, COLOR_BLACK, COLOR_BLUE);
	}
	else {
		aux->has_color = false;
	}
	return ret;
}

static void draw_piece(void *aux, struct game *game, int row, int col) {
	struct piece *piece = &game->board.board[row][col];
	int white_foreground, white_background;
	int pair;
	struct aux *aux_decomposed = (struct aux *) aux;
	white_background = (row+col)%2 == 0;
	white_foreground = piece->type == EMPTY || piece->player == WHITE;

	/* This has got to be my favorite way to implement a binary truth table. */
	switch (white_foreground << 1 | white_background) {
	case 0: pair = BB; break;
	case 1: pair = BW; break;
	case 2: pair = WB; break;
	case 3: pair = WW; break;
	}

	attron(COLOR_PAIR(pair));

	if (piece->type == EMPTY || piece->player == WHITE) {
		addstr(aux_decomposed->piecesyms_white[piece->type]);
	}
	else {
		addstr(aux_decomposed->piecesyms_black[piece->type]);
	}

	attroff(COLOR_PAIR(pair));
}

static void display_board(void *aux, struct game *game, enum player player) {
	UNUSED(aux);

	erase();

	mvaddstr(0, 0, player == WHITE ? "  a b c d e f g h" : "  h g f e d c b a");
	for (int i = 0; i < 8; ++i) {
		int row = player == WHITE ? i : 7 - i;
		mvaddch(i+1, 0, 8-row + '0');
		move(i+1, 2);
		for (int j = 0; j < 8; ++j) {
			int col = player == WHITE ? j : 7-j;
			draw_piece(aux, game, row, col);
		}
	}
	refresh();

	UNUSED(game);
	UNUSED(player);
}

static void free_frontend(struct frontend *this) {
	endwin();
	free(this->aux);
	free(this);
}

static void reset_move(struct move *move) {
	move->r_i = move->c_i = move->r_f = move->c_f = -1;
	move->promotion = EMPTY;
}
