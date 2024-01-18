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

#include <string.h>
#include <stdlib.h>

#include <sys/param.h>

#include <util.h>
#include <client/chess.h>

static void move_unchecked(struct game *game,
		struct piece *src, struct piece *dst);

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
			ret->board.board[i][j].moves = 0;
			ret->board.board[i][j].type = EMPTY;
		}
	}

	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 8; ++j) {
			ret->board.board[i][j].player = BLACK;
			ret->board.board[7-i][j].player = WHITE;
		}
	}

	for (int i = 0; i < 8; ++i) {
		ret->board.board[1][i].type = PAWN;
		ret->board.board[6][i].type = PAWN;
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

/* This code SUCKS! */
int make_move(struct game *game, struct move *move) {
	struct piece *piece, *dst;

	/* reject out-of-bounds moves */
	if (is_oob(move->r_i, 0, 8) || is_oob(move->c_i, 0, 8) ||
	    is_oob(move->r_f, 0, 8) || is_oob(move->c_f, 0, 8)) {
		return -1;
	}

	piece = &game->board.board[move->r_i][move->c_i];
	dst = &game->board.board[move->r_f][move->c_f];

	/* reject out of sequence moves */
	if (!!(piece->player == WHITE) != !!(game->duration % 2 == 0)) {
		return -1;
	}

	/* reject moves where white takes white or black takes black */
	/* this also rejects noop moves like h4h4 */
	if (dst->type != EMPTY && dst->player == piece->player) {
		return -1;
	}

	/* XXX: This switch statement sucks */
	switch (piece->type) {
	case ROOK: rook: {
		int min, max;
		if ((move->r_i - move->r_f) * (move->c_i - move->c_f) != 0) {
			return -1;
		}

		min = MIN(move->r_i, move->r_f);
		max = MAX(move->r_i, move->r_f);
		for (int i = min+1; i < max; ++i) {
			if (game->board.board[i][move->c_i].type != EMPTY) {
				return -1;
			}
		}

		min = MIN(move->c_i, move->c_f);
		max = MAX(move->c_i, move->c_f);
		for (int i = min+1; i < max; ++i) {
			if (game->board.board[move->r_i][i].type != EMPTY) {
				return -1;
			}
		}
		break;
	}
	case KNIGHT: {
		int dr, dc;
		dr = abs(move->r_f - move->r_i);
		dc = abs(move->c_f - move->c_i);
		if (MIN(dr, dc) != 1 || MAX(dr, dc) != 2) {
			return -1;
		}
		break;
	}
	case BISHOP: bishop: {
		int dr, dc, cr, cc;
		dr = move->r_f - move->r_i;
		dc = move->c_f - move->c_i;
		if (abs(dr) != abs(dc)) {
			return -1;
		}

		cr = (dr < 0) ? -1 : 1;
		cc = (dc < 0) ? -1 : 1;

		int r = move->r_i + cr;
		int c = move->c_i + cc;
		while (r != move->r_f) {
			if (game->board.board[r][c].type != EMPTY) {
				return -1;
			}
			r += cr;
			c += cc;
		}
		break;
	}
	case QUEEN: {
		int dr, dc;
		dr = abs(move->r_f - move->r_i);
		dc = abs(move->c_f - move->c_i);
		if (dr * dc == 0) {
			goto rook;
		}
		if (dr == dc) {
			goto bishop;
		}
		return -1;
	}
	case KING: {
		int dr, dc;
		dr = abs(move->r_f - move->r_i);
		dc = abs(move->c_f - move->c_i);
		if (dr > 1 || dc > 1) {
			return -1;
		}
		break;
	}
	case PAWN: {
		int direction = piece->player == WHITE ? -1 : 1;
		if (move->c_f == move->c_i) {
			struct piece *p1, *p2;

			p1 = &game->board.board[move->r_i+direction][move->c_i];
			p2 = &game->board.board[move->r_i+direction*2][move->c_i];

			/* XXX: This sucks */
			if (p1->type != EMPTY) {
				return -1;
			}
			if (move->r_i + direction == move->r_f) {
				break;
			}
			if (move->r_i + direction*2 == move->r_f &&
			    p2->type == EMPTY &&
			    p1->moves == 0) {
				break;
			}

			return -1;
		}
		if (abs(move->c_f - move->c_i) == 1) {
			if (move->r_i + direction != move->r_f) {
				return -1;
			}

			struct piece *dst, *pessant;

			dst = &game->board.board[move->r_f][move->c_f];
			pessant = &game->board.board[move->r_i][move->c_f];
			if (dst->player != piece->player) {
				break;
			}
			/* holy hell */
			if (dst->type == EMPTY &&
			    pessant->type == PAWN &&
			    pessant->player != piece->player &&
			    pessant->moves == 1) {
				pessant->type = EMPTY;
				break;
			}
			return -1;
		}
		return -1;
	}
	case EMPTY:
		return -1;
	}

	move_unchecked(game, piece, dst);

	return 0;
}

static void move_unchecked(struct game *game,
		struct piece *src, struct piece *dst) {
	++game->duration;
	if (dst->type != EMPTY) {
		game->last_capture = game->duration;
	}
	memcpy(dst, src, sizeof *dst);
	++dst->moves;
	src->type = EMPTY;
}
