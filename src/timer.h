#ifndef TIMER_H_HEADER_GUARD
#define TIMER_H_HEADER_GUARD

#include <stdlib.h>  /* size_t */
#include <string.h>  /* memset */
#include <stdbool.h> /* bool, true, false */

struct timer {
	size_t now, time;
	bool   just_ended;
};

void  timer_init(struct timer *t, size_t time);
void  timer_start(struct timer *t);
void  timer_update(struct timer *t);
float timer_unit(struct timer *t, bool reverse);

inline bool timer_active(struct timer *t) {
	return t->now > 0;
}

inline bool timer_just_ended(struct timer *t) {
	return t->just_ended;
}

#endif
