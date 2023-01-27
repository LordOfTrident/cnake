#include "timer.h"

void timer_init(struct timer *t, size_t time) {
	memset(t, 0, sizeof(*t));

	t->time = time;
}

void timer_start(struct timer *t) {
	t->now        = t->time;
	t->just_ended = false;
}

void timer_update(struct timer *t) {
	if (t->now > 0) {
		-- t->now;
		if (t->now <= 0)
			t->just_ended = true;
	} else {
		if (t->just_ended)
			t->just_ended = false;
	}
}

float timer_unit(struct timer *t, bool reverse) {
	float tmp = reverse? t->time - t->now : t->now;
	return tmp / t->time;
}
