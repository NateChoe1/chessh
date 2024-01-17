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

#include <client/chess.h>

/* XXX: This isn't a very good function */
struct game *new_game() {
	struct game *ret;
	if ((ret = malloc(sizeof *ret)) == NULL) {
		return NULL;
	}

	ret->duration = 0;
	ret->last_capture = 0;

	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			ret->board.board[i][j].has_moved = false;
			ret->board.board[i][j].can_pessant = false;
			ret->board.board[i][j].type = EMPTY;
		}
	}

	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 8; ++j) {
			ret->board.board[i][j].player = BLACK;
			ret->board.board[7-i][j].player = WHITE;
		}
	}

#define FIRST_ROW_PIECE(index, ptype) \
	ret->board.board[0][index].type = ptype; \
	ret->board.board[0][7-index].type = ptype; \
	ret->board.board[7][index].type = ptype; \
	ret->board.board[7][7-index].type = ptype;
	FIRST_ROW_PIECE(0, ROOK);
	FIRST_ROW_PIECE(1, KNIGHT);
	FIRST_ROW_PIECE(2, BISHOP);
#undef FIRST_ROW_PIECE
	ret->board.board[0][3].type = QUEEN;
	ret->board.board[0][4].type = KING;
	ret->board.board[7][3].type = QUEEN;
	ret->board.board[7][4].type = KING;

	return ret;
}

void free_game(struct game *game) {
	free(game);
}
