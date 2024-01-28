#ifndef HAVE_UTIL
#define HAVE_UTIL

#include <stdbool.h>

#define UNUSED(param) do { (void) param ; } while (0)

static inline bool is_in_bounds(int val, int min, int max) {
	return min <= val && val < max;
}

static inline bool is_oob(int val, int min, int max) {
	return !is_in_bounds(val, min, max);
}

extern void noop(void);

#endif
