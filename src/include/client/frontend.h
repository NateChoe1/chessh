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

#ifndef HAVE_CLIENT__FRONTEND
#define HAVE_CLIENT__FRONTEND

#include <client/chess.h>

struct frontend {
	char *(*get_move)(void *aux);

	/* Used for things that the frontend can fix, currently only used when a
	 * pawn is missing a promotion. The error always refers to the last
	 * move. Another call to `get_move` will be made after `report_error`. */
	void (*report_error)(void *aux, int code);

	void (*report_msg)(void *aux, char *msg);
	void (*display_board)(void *aux, struct game *game, enum player player);
	void (*free)(struct frontend *this);
	void *aux;
};

extern struct frontend *new_curses_frontend(char **piecesyms_white, char **piecesyms_black);
extern struct frontend *new_text_frontend(char **piecesyms_white, char **piecesyms_black);

#endif
