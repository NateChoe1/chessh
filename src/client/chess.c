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

/* precondition: game, move, captured, castle are all valid pointers
 *               *captured == *castle == NULL
 * returns: whether this move is illegal
 * postcondition: *captured MAY be set to some extra casualty of this move, such
 *                as a pawn taken by en pessant (holy hell)
 *                *castle MAY be set to some move that also happens, probably
 *                due to castling */
static int is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle);

/* precondition: game, move, captured, and castle are all valid pointers
 *               *captured == *castle = NULL
 *               the moving piece and the destination are owned by different
 *               players
 *               the moving piece is actually of the specified type
 * returns: whether a move made by this type of piece is illegal
 * postcondition: see is_illegal */
static int rook_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle);
static int knight_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle);
static int bishop_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle);
static int queen_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle);
static int king_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle);
static int pawn_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle);

static void move_unchecked(struct game *game, struct move *move, struct piece *captured, bool should_advance_clock);

#define PARSE_MOVE(game, move, src, dst) \
	do { \
		src = &game->board.board[move->r_i][move->c_i]; \
		dst = &game->board.board[move->r_f][move->c_f]; \
	} while (0)

struct game *new_game() {
	struct game *ret;
	if ((ret = malloc(sizeof *ret)) == NULL) {
		return NULL;
	}

	ret->duration = 0;
	ret->last_big_move = 0;

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

int make_move(struct game *game, struct move *move) {
	struct piece *captured;
	struct move *castle;
	int error_code;

	captured = NULL;
	castle = NULL;

	if ((error_code = is_illegal(game, move, &captured, &castle))) {
		return error_code;
	}

	move_unchecked(game, move, captured, true);
	if (castle != NULL) {
		move_unchecked(game, move, NULL, false);
	}

	return 0;
}

static int is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle) {
	struct piece *piece, *dst;

	/* reject out-of-bounds moves */
	if (is_oob(move->r_i, 0, 8) || is_oob(move->c_i, 0, 8) ||
	    is_oob(move->r_f, 0, 8) || is_oob(move->c_f, 0, 8)) {
		return ILLEGAL_MOVE;
	}

	PARSE_MOVE(game, move, piece, dst);

	/* reject out of sequence moves */
	if (!!(piece->player == WHITE) != !!(game->duration % 2 == 0)) {
		return ILLEGAL_MOVE;
	}

	/* reject moves where white takes white or black takes black */
	/* this also rejects noop moves like h4h4 */
	if (dst->type != EMPTY && dst->player == piece->player) {
		return ILLEGAL_MOVE;
	}

	switch (piece->type) {
	case ROOK:
		return rook_is_illegal(game, move, captured, castle);
	case KNIGHT:
		return knight_is_illegal(game, move, captured, castle);
	case BISHOP:
		return bishop_is_illegal(game, move, captured, castle);
	case QUEEN:
		return queen_is_illegal(game, move, captured, castle);
	case KING:
		return king_is_illegal(game, move, captured, castle);
	case PAWN:
		return pawn_is_illegal(game, move, captured, castle);
	case EMPTY:
		return ILLEGAL_MOVE;
	}

	return ILLEGAL_MOVE;
}

static int rook_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle) {
	int min, max;

	UNUSED(captured);
	UNUSED(castle);

	if ((move->r_i - move->r_f) * (move->c_i - move->c_f) != 0) {
		return ILLEGAL_MOVE;
	}

	min = MIN(move->r_i, move->r_f);
	max = MAX(move->r_i, move->r_f);
	for (int i = min+1; i < max; ++i) {
		if (game->board.board[i][move->c_i].type != EMPTY) {
			return ILLEGAL_MOVE;
		}
	}

	min = MIN(move->c_i, move->c_f);
	max = MAX(move->c_i, move->c_f);
	for (int i = min+1; i < max; ++i) {
		if (game->board.board[move->r_i][i].type != EMPTY) {
			return ILLEGAL_MOVE;
		}
	}

	return 0;
}

static int knight_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle) {
	int dr, dc;

	UNUSED(game);
	UNUSED(captured);
	UNUSED(castle);

	dr = abs(move->r_f - move->r_i);
	dc = abs(move->c_f - move->c_i);

	return (MIN(dr, dc) == 1 && MAX(dr, dc) == 2) ? 0 : ILLEGAL_MOVE;
}

static int bishop_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle) {
	int dr, dc, cr, cc;

	UNUSED(captured);
	UNUSED(castle);

	dr = move->r_f - move->r_i;
	dc = move->c_f - move->c_i;
	if (abs(dr) != abs(dc)) {
		return ILLEGAL_MOVE;
	}

	cr = (dr < 0) ? -1 : 1;
	cc = (dc < 0) ? -1 : 1;

	int r = move->r_i + cr;
	int c = move->c_i + cc;
	while (r != move->r_f) {
		if (game->board.board[r][c].type != EMPTY) {
			return ILLEGAL_MOVE;
		}
		r += cr;
		c += cc;
	}
	return 0;
}

static int queen_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle) {
	int dr, dc;

	UNUSED(castle);

	dr = abs(move->r_f - move->r_i);
	dc = abs(move->c_f - move->c_i);
	if (dr * dc == 0) {
		return rook_is_illegal(game, move, captured, castle);
	}
	if (dr == dc) {
		return bishop_is_illegal(game, move, captured, castle);
	}
	return ILLEGAL_MOVE;
}

/* TODO: Castling */
static int king_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle) {
	int dr, dc;

	UNUSED(captured);

	dr = abs(move->r_f - move->r_i);
	dc = abs(move->c_f - move->c_i);

	/* regular king moves */

	if (dr <= 1 && dc <= 1) {
		return 0;
	}

	/* castling */

	UNUSED(game);
	UNUSED(castle);
	/*
	if (piece->moves != 0 ||
	    dr != 0 ||
	    dc != 2) {
		return false;
	}
	*/

	return ILLEGAL_MOVE;
}

static int pawn_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move **castle) {
	struct piece *piece, *dst;
	int direction;

	UNUSED(castle);

	PARSE_MOVE(game, move, piece, dst);

	direction = piece->player == WHITE ? -1 : 1;

	/* Regular moves where pawns don't capture */
	if (move->c_f == move->c_i) {
		struct piece *p1, *p2;

		p1 = &game->board.board[move->r_i+direction][move->c_i];
		p2 = &game->board.board[move->r_i+direction*2][move->c_i];

		if (p1->type != EMPTY) {
			return ILLEGAL_MOVE;
		}
		if (move->r_i + direction == move->r_f) {
			goto promote_pawn;
		}
		if (move->r_i + direction*2 == move->r_f &&
		    p2->type == EMPTY &&
		    p1->moves == 0) {
			goto promote_pawn;
		}

		return ILLEGAL_MOVE;
	}

	/* Pawn capture moves */
	if (abs(move->c_f - move->c_i) == 1) {
		struct piece *pessant;

		if (move->r_i + direction != move->r_f) {
			return ILLEGAL_MOVE;
		}

		pessant = &game->board.board[move->r_i][move->c_f];
		if (dst->type != EMPTY) {
			goto promote_pawn;
		}
		if (pessant->type == PAWN &&
		    pessant->player != piece->player &&
		    pessant->moves == 1 &&
		    pessant->last_move == game->duration) {
			*captured = pessant;
			goto promote_pawn;
		}
		return ILLEGAL_MOVE;
	}

	return ILLEGAL_MOVE;
promote_pawn:

	if ((piece->player == WHITE && move->r_f == 7) ||
	    (piece->player == BLACK && move->r_f == 0)) {
		switch (move->promotion) {
		case ROOK: case KNIGHT: case BISHOP: case QUEEN:
			piece->type = move->promotion;
			break;
		default:
			return MISSING_PROMOTION;
		}
	}
	return 0;
}

static void move_unchecked(struct game *game, struct move *move, struct piece *captured, bool should_advance_clock) {
	struct piece *src, *dst;

	PARSE_MOVE(game, move, src, dst);
	if (should_advance_clock) {
		++game->duration;
	}
	if (dst->type != EMPTY || captured != NULL) {
		game->last_big_move = game->duration;
	}
	if (captured != NULL) {
		captured->type = EMPTY;
	}
	memcpy(dst, src, sizeof *dst);
	++dst->moves;
	dst->last_move = game->duration;
	src->type = EMPTY;
}
