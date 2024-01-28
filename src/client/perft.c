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
#include <stdlib.h>
#include <string.h>

#include <util.h>

#include <client/perft.h>
#include <client/chess.h>

static void calculate_perft(struct game *game, int curr_depth, int max_depth, unsigned long long *results);
static void add_node(struct game *game, int curr_depth, int max_depth, unsigned long long *results,
		int r_i, int c_i, int r_f, int c_f, enum piece_type promotion);

extern int run_perft(int level, char *start_pos) {
	unsigned long long *results;
	struct game *game;

	results = alloca(level * sizeof *results);
	memset(results, 0, level * sizeof *results);
	game = new_game();

	if (start_pos != NULL) {
		puts(start_pos);
		if (init_game(game, start_pos)) {
			fputs("Failed to initialize game\n", stderr);
			return 1;
		}
	}

	if (game == NULL || results == NULL) {
		fputs("Failed to initialize variables\n", stderr);
		return 1;
	}

	calculate_perft(game, 0, level, results);

	free_game(game);

	for (int i = 0; i < level; ++i) {
		printf("%llu\n", results[i]);
	}

	return 0;
}

static void calculate_perft(struct game *game, int curr_depth, int max_depth, unsigned long long *results) {
	++results[curr_depth];

	if (curr_depth >= max_depth-1) {
		return;
	}
	/* This code is intentionally very inefficient, it's designed to weed
	 * out bugs in my engine, not to efficiently calculate perft values. I
	 * could do a lot of pruning, but that would be testing the test
	 * function, and not the real function. */
	for (int r_i = 0; r_i < 8; r_i++) {
		for (int c_i = 0; c_i < 8; c_i++) {
			for (int r_f = 0; r_f < 8; ++r_f) {
				for (int c_f = 0; c_f < 8; ++c_f) {
					add_node(game, curr_depth, max_depth, results, r_i, c_i, r_f, c_f, EMPTY);
				}
			}
		}
	}
}
static void add_node(struct game *game, int curr_depth, int max_depth, unsigned long long *results,
		int r_i, int c_i, int r_f, int c_f, enum piece_type promotion) {
	struct move move;
	struct game backup;
	int code;

	memcpy(&backup, game, sizeof backup);

	move.r_i = r_i;
	move.c_i = c_i;
	move.r_f = r_f;
	move.c_f = c_f;
	move.promotion = promotion;

	code = make_move(&backup, &move);

	switch (code) {
	case ILLEGAL_MOVE:
		return;
	case BLACK_WIN: case WHITE_WIN: case FORCED_DRAW:
		++results[curr_depth+1];
		return;
	case MISSING_PROMOTION:
		if (promotion != EMPTY) {
			fputs("ENGINE ERROR!!! make_move() returned MISSING_PROMOTION on non-empty promotion type\n", stderr);
			exit(EXIT_FAILURE);
		}
		add_node(&backup, curr_depth, max_depth, results, r_i, c_i, r_f, c_f, ROOK);
		add_node(&backup, curr_depth, max_depth, results, r_i, c_i, r_f, c_f, KNIGHT);
		add_node(&backup, curr_depth, max_depth, results, r_i, c_i, r_f, c_f, BISHOP);
		add_node(&backup, curr_depth, max_depth, results, r_i, c_i, r_f, c_f, QUEEN);
		break;
	default:
		calculate_perft(&backup, curr_depth+1, max_depth, results);
	}
}
