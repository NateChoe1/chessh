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

#ifndef HAVE_CLIENT__CHESS
#define HAVE_CLIENT__CHESS

#include <stdbool.h>

enum piece_type {
	ROOK,
	KNIGHT,
	BISHOP,
	QUEEN,
	KING,
	PAWN,
	EMPTY
};

enum player {
	WHITE,
	BLACK
};

struct piece {
	enum piece_type type;
	enum player player;
	bool has_moved;
	bool can_pessant; /* only valid for pawns */
};

struct board {
	/* board[0][N] = first row black
	 * board[1][N] = second row black
	 * board[0][3] = black queen
	 *
	 * Basically in reading order from white's perspective */
	struct piece board[8][8];
};

struct game {
	struct board board;
	int duration; /* no. of turns played, includes both black and white's
			 moves */
	int last_capture; /* for the 50 move draw rule */
};

extern struct game *new_game();
extern void free_game(struct game *game);

#endif
