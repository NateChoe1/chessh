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

static int run_sequence(struct game *game, char *sequence);
static void calculate_perft(struct game *game, int curr_depth, int max_depth, unsigned long long *results,
		int r_pi, int c_pi, int r_pf, int c_pf, bool autotest);
static void add_node(struct game *game, int curr_depth, int max_depth, unsigned long long *results,
		int r_i, int c_i, int r_f, int c_f, enum piece_type promotion, bool autotest);
static void print_move(int ri, int ci, int rf, int cf, unsigned long long diff);

int run_perft(int level, char *start_pos, char *start_sequence, bool autotest) {
	unsigned long long *results;
	struct game *game;

	results = alloca(level * sizeof *results);
	memset(results, 0, level * sizeof *results);
	game = new_game();

	if (start_pos != NULL) {
		if (init_game(game, start_pos)) {
			fputs("Failed to initialize game\n", stderr);
			return 1;
		}
	}

	if (start_sequence != NULL) {
		int code;
		code = run_sequence(game, start_sequence);
		if (code != 0) {
			return code;
		}
	}

	if (game == NULL || results == NULL) {
		fputs("Failed to initialize variables\n", stderr);
		return 1;
	}

	calculate_perft(game, 0, level, results, 0, 0, 0, 0, autotest);

	free_game(game);

	if (!autotest) {
		for (int i = 0; i < level; ++i) {
			printf("%llu\n", results[i]);
		}
	}
	else {
		putchar('\n');
		printf("%llu\n", results[level-1]);
	}

	return 0;
}

static int run_sequence(struct game *game, char *sequence) {
	for (int i = 0; sequence[i] != '\0'; ++i) {
		char buff[10];
		int j;
		for (j = 0; sequence[i+j] != ' ' && sequence[i+j] != '\0' && j < (ssize_t) sizeof buff-1; ++j) {
			buff[j] = sequence[i+j];
		}
		buff[j] = '\0';
		if (parse_move(game, buff) < 0) {
			return 1;
		}
		i += j;
		if (sequence[i] == '\0') {
			break;
		}
	}
	return 0;
}

static void calculate_perft(struct game *game, int curr_depth, int max_depth, unsigned long long *results,
		int r_pi, int c_pi, int r_pf, int c_pf, bool autotest) {
	unsigned long long old_depth;

	if (curr_depth >= max_depth) {
		return;
	}

	if (autotest && curr_depth == 1) {
		old_depth = results[max_depth - 1];
	}

	++results[curr_depth];

	/* This code is intentionally very inefficient, it's designed to weed
	 * out bugs in my engine, not to efficiently calculate perft values. I
	 * could do a lot of pruning, but that would be testing the test
	 * function, and not the real function. */
	for (int r_i = 0; r_i < 8; r_i++) {
		for (int c_i = 0; c_i < 8; c_i++) {
			for (int r_f = 0; r_f < 8; ++r_f) {
				for (int c_f = 0; c_f < 8; ++c_f) {
					add_node(game, curr_depth, max_depth, results, r_i, c_i, r_f, c_f, EMPTY, autotest);
				}
			}
		}
	}

	if (autotest && curr_depth == 1) {
		unsigned long long diff;
		diff = results[max_depth - 1] - old_depth;
		if (diff == 0) {
			return;
		}
		print_move(r_pi, c_pi, r_pf, c_pf, diff);
	}
}
static void add_node(struct game *game, int curr_depth, int max_depth, unsigned long long *results,
		int r_i, int c_i, int r_f, int c_f, enum piece_type promotion, bool autotest) {
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
		add_node(&backup, curr_depth, max_depth, results, r_i, c_i, r_f, c_f, ROOK, autotest);
		add_node(&backup, curr_depth, max_depth, results, r_i, c_i, r_f, c_f, KNIGHT, autotest);
		add_node(&backup, curr_depth, max_depth, results, r_i, c_i, r_f, c_f, BISHOP, autotest);
		add_node(&backup, curr_depth, max_depth, results, r_i, c_i, r_f, c_f, QUEEN, autotest);
		break;
	default:
		calculate_perft(&backup, curr_depth+1, max_depth, results, r_i, c_i, r_f, c_f, autotest);
	}
}

static void print_move(int ri, int ci, int rf, int cf, unsigned long long diff) {
	printf("%c%d%c%d %llu\n", ci+'a', 8 - ri, cf+'a', 8 - rf, diff);
}
