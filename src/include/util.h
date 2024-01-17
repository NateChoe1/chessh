#ifndef HAVE_UTIL
#define HAVE_UTIL

#include <stdbool.h>

static inline bool is_in_bounds(int val, int min, int max) {
	return min <= val && val < max;
}

static inline bool is_oob(int val, int min, int max) {
	return !is_in_bounds(val, min, max);
}

#endif
