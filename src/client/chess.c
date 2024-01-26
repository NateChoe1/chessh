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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/param.h>

#include <util.h>
#include <client/chess.h>

/* precondition: game, move, captured, castle are all valid pointers
 * precondition: *captured == NULL
 * precondition: (*castle)->r_i == (*castle)->r_f ==
 *               (*castle)->c_i == * (*castle)->c_f == -1
 * returns: the corresponding `PIECE_is_illegal` function's return value
 * postcondition: *captured MAY be set to some extra casualty of this move, such
 *                as a pawn taken by en pessant (holy hell)
 * postcondition: *castle MAY be set to some move that also happens, probably
 *                due to castling. */
static int is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle, enum player player);

/* precondition: game, move, captured, and castle are all valid pointers
 *               *captured == *castle = NULL
 * precondition: the moving piece and the destination are owned by different
 *               players
 * precondition: the moving piece is actually of the specified type
 * precondition: `move` doesn't start and end at the same spot
 * returns: <0 if a move made by this type of piece is illegal
 * postcondition: see `is_illegal` */
static int rook_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle);
static int knight_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle);
static int bishop_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle);
static int queen_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle);
static int king_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle);
static int pawn_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle);

/* does `move`, completely unchecked. captures `captured`, possibly advances the
 * clock */
static void move_unchecked(struct game *game, struct move *move, struct piece *captured, bool should_advance_clock);

/* checks if game->board.board[r][c] is attacked by the person playing AGAINST
 * player. This means that if `player` is WHITE, then `piece_is_attacked` would
 * check if BLACK is attacking a certain tile.
 * XXX: this function breaks with en pessant */
static bool piece_is_attacked(struct game *game, int r, int c, enum player player);

/* checks if `player` is in check */
static bool is_in_check(struct game *game, enum player player);

/* checks if `player` has a valid move to make */
static bool can_make_move(struct game *game, enum player player);

/* checks if the piece at [row][col] can make a move */
static bool piece_can_move(struct game *game, int row, int col);

enum player get_player(struct game *game);

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
	struct move castle;
	int error_code;
	struct game backup;
	enum player curr_player, other_player;

	curr_player = get_player(game);
	other_player = curr_player == WHITE ? BLACK : WHITE;

	captured = NULL;
	castle.r_i = castle.r_f = castle.c_i = castle.c_f = -1;

	if ((error_code = is_illegal(game, move, &captured, &castle, curr_player)) < 0) {
		return error_code;
	}

	memcpy(&backup, game, sizeof backup);

	move_unchecked(game, move, captured, true);
	if (castle.r_i != -1) {
		move_unchecked(game, &castle, NULL, false);
	}

	if (is_in_check(game, curr_player)) {
		memcpy(game, &backup, sizeof *game);
		return ILLEGAL_MOVE;
	}

	if (game->duration - game->last_big_move >= 150) {
		return FORCED_DRAW;
	}

	if (game->duration - game->last_big_move >= 100) {
		return DRAW_OFFER;
	}

	if (!can_make_move(game, other_player)) {
		if (is_in_check(game, other_player)) {
			return curr_player == WHITE ?  WHITE_WIN : BLACK_WIN;
		}
		return FORCED_DRAW;
	}


	return error_code;
}

static int is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle, enum player player) {
	struct piece *piece, *dst;

	/* reject out-of-bounds moves */
	if (is_oob(move->r_i, 0, 8) || is_oob(move->c_i, 0, 8) ||
	    is_oob(move->r_f, 0, 8) || is_oob(move->c_f, 0, 8)) {
		return ILLEGAL_MOVE;
	}

	PARSE_MOVE(game, move, piece, dst);

	/* reject out of sequence moves */
	if (piece->player != player) {
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

static int rook_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle) {
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

static int knight_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle) {
	int dr, dc;

	UNUSED(game);
	UNUSED(captured);
	UNUSED(castle);

	dr = abs(move->r_f - move->r_i);
	dc = abs(move->c_f - move->c_i);

	return (MIN(dr, dc) == 1 && MAX(dr, dc) == 2) ? 0 : ILLEGAL_MOVE;
}

static int bishop_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle) {
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

static int queen_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle) {
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

static int king_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle) {
	int dr, dc;
	int cc;
	int c;
	struct piece *piece, *rook;

	UNUSED(captured);

	dr = move->r_f - move->r_i;
	dc = move->c_f - move->c_i;

	/* regular king moves */

	if (abs(dr) <= 1 && abs(dc) <= 1) {
		return 0;
	}

	/* castling */

	piece = &game->board.board[move->r_i][move->c_i];

	/* the king has already moved or this would be an improper castle */
	if (piece->moves != 0 ||
	    dr != 0 ||
	    abs(dc) != 2) {
		return ILLEGAL_MOVE;
	}

	cc = (dc < 0) ? -1:1;

	/* find the next piece (presumably a rook) that goes in the proper
	 * direction */
	for (c = move->c_i + cc;
			0 <= c &&
			c <= 8 &&
			game->board.board[move->r_i][c].type == EMPTY;
			c += cc) ;

	/* there is no piece */
	if (c < 0 || c >= 8) {
		return ILLEGAL_MOVE;
	}

	rook = &game->board.board[move->r_i][c];

	/* the next piece in the proper direction isn't a rook */
	if (rook->type != ROOK) {
		return ILLEGAL_MOVE;
	}
	/* the rook has already moved */
	if (rook->moves != 0) {
		return ILLEGAL_MOVE;
	}

	/* we're under attack*/
	if (piece_is_attacked(game, move->r_i, move->c_i, piece->player) ||
	    piece_is_attacked(game, move->r_i, move->c_i + cc, piece->player) ||
	    piece_is_attacked(game, move->r_i, move->c_i + cc*2, piece->player)) {
		return ILLEGAL_MOVE;
	}

	castle->r_i = castle->r_f = move->r_i;
	castle->c_i = c;;
	castle->c_f = move->c_i + cc;

	return 0;
}

static int pawn_is_illegal(struct game *game, struct move *move, struct piece **captured, struct move *castle) {
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
		    piece->moves == 0) {
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

	if ((piece->player == WHITE && move->r_f == 0) ||
	    (piece->player == BLACK && move->r_f == 7)) {
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

static bool piece_is_attacked(struct game *game, int r, int c, enum player player) {
	enum player other_player = player == WHITE ? BLACK : WHITE;
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			struct move move;
			struct piece *captured;
			struct move castle;

			if (game->board.board[i][j].type == EMPTY ||
			    game->board.board[i][j].player == player) {
				continue;
			}

			captured = NULL;
			castle.r_i = castle.c_i = castle.r_f = castle.c_f = -1;
			move.r_i = i;
			move.c_i = j;
			move.r_f = r;
			move.c_f = c;
			move.promotion = QUEEN;
			if (is_illegal(game, &move, &captured, &castle, other_player) >= 0) {
				return true;
			}
		}
	}
	return false;
}

static bool is_in_check(struct game *game, enum player player) {
	int kr, kc;
	for (kr = 0; kr < 8; ++kr) {
		for (kc = 0; kc < 8; ++kc) {
			if (game->board.board[kr][kc].type == KING &&
			    game->board.board[kr][kc].player == player) {
				goto found_king;
			}
		}
	}
	/* somehow the king is gone? */
	return true;
found_king:
	return piece_is_attacked(game, kr, kc, player);
}

static bool can_make_move(struct game *game, enum player player) {
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j){
			if (game->board.board[i][j].type == EMPTY ||
			    game->board.board[i][j].player != player) {
				continue;
			}
			if (piece_can_move(game, i, j)) {
				return true;
			}
		}
	}
	return false;
}

static bool piece_can_move(struct game *game, int row, int col) {
	struct piece *piece = &game->board.board[row][col];
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			struct move move;
			struct piece *captured;
			struct move castle;
			move.r_i = row;
			move.c_i = col;
			move.r_f = i;
			move.c_f = j;
			captured = NULL;
			castle.r_i = castle.r_f = castle.c_i = castle.c_f = -1;
			move.promotion = QUEEN;
			if (is_illegal(game, &move, &captured, &castle, piece->player) >= 0) {
				printf("%d %d %d %d\n", row, col, i, j);
				printf("%d %d %d %d\n", move.r_i, move.c_i, move.r_f, move.c_f);
				return true;
			}
		}
	}
	return false;
}

enum player get_player(struct game *game) {
	return game->duration % 2 == 0 ? WHITE : BLACK;
}
