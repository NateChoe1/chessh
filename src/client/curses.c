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

#include <ctype.h>
#include <locale.h>
#include <stdlib.h>
#include <stdbool.h>

#include <curses.h>

#include <util.h>

#include <client/frontend.h>

struct aux {
	struct move move;
	wchar_t **piecesyms_white;
	wchar_t **piecesyms_black;
	bool has_color;
	char *msg;
};

/* WB = white foreground, black background */
#define WW 1
#define WB 2
#define BW 3
#define BB 4

#define CREDIT "made by nate choe <nate@natechoe.dev>, https://github.com/natechoe1/chessh"

static char *get_move(void *aux, enum player player);
static void select_square(int *r_ret, int *c_ret, enum player player);
static void report_error(void *aux, int code);
static void report_msg(void *aux, char *msg);
static void draw_piece(struct aux *aux, struct game *game, int row, int col);
static void display_board(void *aux, struct game *game, enum player player);
static void show_credit();
static void free_frontend(struct frontend *this);
static void reset_move(struct move *move);
static void drawmsg(struct aux *aux, char *msg);

struct frontend *new_curses_frontend(wchar_t **piecesyms_white, wchar_t **piecesyms_black) {
	struct frontend *ret;
	struct aux *aux;
	WINDOW *win;

	if ((ret = malloc(sizeof *ret)) == NULL) {
		return NULL;
	}
	if ((aux = malloc(sizeof *aux)) == NULL) {
		return NULL;
	}

	win = initscr();
	cbreak();
	noecho();
	curs_set(0);
	keypad(win, TRUE);

	ret->get_move = get_move;
	ret->report_msg = report_msg;
	ret->report_error = report_error;
	ret->display_board = display_board;
	ret->free = free_frontend;
	ret->aux = aux;
	reset_move(&aux->move);
	aux->piecesyms_white = piecesyms_white;
	aux->piecesyms_black = piecesyms_black;
	aux->msg = NULL;
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

static char *get_move(void *aux, enum player player) {
	struct aux *aux_decomposed = (struct aux *) aux;
	struct move *move;
	move = &aux_decomposed->move;
	if (move->promotion != EMPTY) {
		goto end;
	}

	drawmsg(aux_decomposed, "Select the start location for your move");
	select_square(&move->r_i, &move->c_i, player);
	drawmsg(aux_decomposed, "Select the end location for your move");
	select_square(&move->r_f, &move->c_f, player);

end:
	/* Reset the promotion for the next move */
	aux_decomposed->move.promotion = EMPTY;
	return move_to_string(move);
}

static void select_square(int *r_ret, int *c_ret, enum player player) {
	int r, c;
	r = c = 0;
	curs_set(1);
	for (;;) {
		int ch;
		MEVENT mevent;
		move(r+1, c*2+2);
		switch (ch = getch()) {
		case 'h': c -= 1; break;
		case 'j': r += 1; break;
		case 'k': r -= 1; break;
		case 'l': c += 1; break;
		case KEY_LEFT: c -= 1 ; break;
		case KEY_UP: r -= 1 ; break;
		case KEY_RIGHT: c += 1 ; break;
		case KEY_DOWN: r += 1 ; break;

		case KEY_MOUSE:
			getmouse(&mevent);
			if (!(mevent.bstate & BUTTON1_PRESSED)) {
				break;
			}
			r = mevent.y - 1;
			c = (mevent.x - 2) / 2;
			/* fallthrough */
		case ' ': case KEY_ENTER: case '\r': case '\n':
			*r_ret = player == WHITE ? r : 7-r;
			*c_ret = player == WHITE ? c : 7-c;
			curs_set(0);
			return;
		}
		r = (r+8) % 8;
		c = (c+8) % 8;
	}
}

static void report_error(void *aux, int code) {
	struct aux *aux_decomposed = (struct aux *) aux;
	switch (code) {
	case MISSING_PROMOTION:
		drawmsg(aux_decomposed, "What would you like to promote to? [qrnb]");
		for (;;) {
			switch (tolower(getch())) {
			case 'q': aux_decomposed->move.promotion = QUEEN; break;
			case 'r': aux_decomposed->move.promotion = ROOK; break;
			case 'n': aux_decomposed->move.promotion = KNIGHT; break;
			case 'b': aux_decomposed->move.promotion = BISHOP; break;
			default: drawmsg(aux_decomposed, "Invalid promotion [qrnb]"); continue;
			}
			break;
		}
		break;
	default:
		drawmsg(aux_decomposed, "Warning: Unknown warning received from backend");
	}
}

static void report_msg(void *aux, char *msg) {
	drawmsg((struct aux *) aux, msg);
	show_credit();
}

static void draw_piece(struct aux *aux, struct game *game, int row, int col) {
	struct piece *piece = &game->board.board[row][col];
	int white_foreground, white_background;
	int pair;
	white_background = (row+col)%2 == 0;
	white_foreground = piece->type == EMPTY || piece->player == WHITE;

	/* This has got to be my favorite way to implement a binary truth table. */
	switch (white_foreground << 1 | white_background) {
	case 0: pair = BB; break;
	case 1: pair = BW; break;
	case 2: pair = WB; break;
	case 3: pair = WW; break;
	}

	if (aux->has_color) {
		attron(COLOR_PAIR(pair));
	}

	if (piece->type == EMPTY || piece->player == WHITE) {
		addwstr(aux->piecesyms_white[piece->type]);
	}
	else {
		addwstr(aux->piecesyms_black[piece->type]);
	}

	attroff(COLOR_PAIR(pair));
}

static void display_board(void *aux, struct game *game, enum player player) {
	struct aux *aux_decomposed = (struct aux *) aux;

	erase();

	mvaddstr(0, 0, player == WHITE ? "  a b c d e f g h" : "  h g f e d c b a");
	for (int i = 0; i < 8; ++i) {
		int row = player == WHITE ? i : 7 - i;
		mvaddch(i+1, 0, 8-row + '0');
		move(i+1, 2);
		for (int j = 0; j < 8; ++j) {
			int col = player == WHITE ? j : 7-j;
			draw_piece(aux_decomposed, game, row, col);
		}
	}
	drawmsg(aux_decomposed, aux_decomposed->msg);
	show_credit();
	refresh();
}

static void show_credit() {
	move(LINES-1, 0);
	clrtoeol();
	addstr(CREDIT);
}

static void free_frontend(struct frontend *this) {
	struct aux *aux_decomposed = (struct aux *) this->aux;
	endwin();
	free(aux_decomposed->msg);
	free(aux_decomposed);
	free(this);
}

static void reset_move(struct move *move) {
	move->r_i = move->c_i = move->r_f = move->c_f = -1;
	move->promotion = EMPTY;
}

static void drawmsg(struct aux *aux, char *msg) {
	if (msg == NULL) {
		return;
	}
	move(LINES-2, 0);
	clrtoeol();
	addstr(msg);
	show_credit();
	refresh();
	aux->msg = msg;
}
