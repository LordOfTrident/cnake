#include "snake.h"

enum dir dir_from_a_to_b(SDL_Point a, SDL_Point b) {
	if (a.x == b.x)
		return a.y < b.y? DOWN : UP;
	else if (a.y == b.y)
		return a.x < b.x? RIGHT : LEFT;
	else
		UNREACHABLE("Impossible direction");
}

double dir_to_angle(enum dir dir) {
	switch (dir) {
	case UP:    return 0;
	case LEFT:  return 270;
	case DOWN:  return 180;
	case RIGHT: return 90;

	default: UNREACHABLE("Impossible direction");
	}
}

static void snake_delay_tongue(struct snake *s) {
	size_t total = SNAKE_TONGUE_MOVE_TIME * 2 + SNAKE_TONGUE_TIME;
	size_t time  = rand_irange(SNAKE_TONGUE_MIN_DELAY, SNAKE_TONGUE_MAX_DELAY + total);

	s->tongue_state = TONGUE_HIDDEN;
	timer_init(&s->tongue_timer, time);
	timer_start(&s->tongue_timer);
}

void snake_init(struct snake *s, SDL_Point start,
                struct snake_textures textures, int r, int g, int b) {
	memset(s, 0, sizeof(*s));

	s->head     = s->body;
	s->len      = 2;
	s->offset   = 1;
	s->dir      = RIGHT;
	s->next_dir = s->dir;

	s->head->x   = start.x;
	s->head->y   = start.y;
	s->body[1].x = start.x - 1;
	s->body[1].y = start.y;
	s->prev.x    = start.x - 2;
	s->prev.y    = start.y;

	snake_delay_tongue(s);

	s->textures = textures;
	s->r = r;
	s->g = g;
	s->b = b;
}

void snake_update(struct snake *s) {
	timer_update(&s->tongue_timer);
	if (timer_just_ended(&s->tongue_timer)) {
		switch (s->tongue_state) {
		case TONGUE_SHOWING:
			s->tongue_state = TONGUE_SHOWN;
			timer_init(&s->tongue_timer, SNAKE_TONGUE_TIME);
			timer_start(&s->tongue_timer);

			break;

		case TONGUE_SHOWN:
			s->tongue_state = TONGUE_HIDING;
			timer_init(&s->tongue_timer, SNAKE_TONGUE_MOVE_TIME);
			timer_start(&s->tongue_timer);

			break;

		case TONGUE_HIDING: snake_delay_tongue(s); break;
		case TONGUE_HIDDEN:
			s->tongue_state = TONGUE_SHOWING;
			timer_init(&s->tongue_timer, SNAKE_TONGUE_MOVE_TIME);
			timer_start(&s->tongue_timer);

			break;
		}
	}
}

bool snake_move(struct snake *s, float by) {
	s->offset += by;
	if (s->offset >= 1) {
		s->offset = 0;
		s->dir  = s->next_dir;
		s->prev = s->body[s->len - 1];

		for (; s->requested_grow > 0; -- s->requested_grow) {
			s->body[s->len] = s->body[s->len - 1];
			++ s->len;
		}

		for (size_t i = s->len - 1; i > 0; -- i)
			s->body[i] = s->body[i - 1];

		switch (s->dir) {
		case UP:    -- s->head->y; break;
		case LEFT:  -- s->head->x; break;
		case DOWN:  ++ s->head->y; break;
		case RIGHT: ++ s->head->x; break;
		}

		return true;
	} else
		return false;
}

void snake_grow(struct snake *s) {
	++ s->requested_grow;
}

void snake_shrink_to(struct snake *s, size_t len) {
	if (len >= s->len)
		UNREACHABLE("Invalid shrink");

	s->len  = len;
	s->prev = s->body[len];
}

void snake_change_dir(struct snake *s, enum dir dir) {
	if ((s->dir - dir) % 2 == 0)
		return;

	s->next_dir = dir;
}

static SDL_Rect snake_offset_part_rect(struct snake *s, SDL_Point pos, enum dir dir, bool inv) {
	int size = s->offset * RECT_SIZE;
	if (inv)
		size = RECT_SIZE - size;

	SDL_Rect r;
	r.w = RECT_SIZE;
	r.h = size;
	r.x = pos.x * RECT_SIZE;
	r.y = pos.y * RECT_SIZE;

	switch (dir) {
	case UP: r.y = (pos.y + 1) * RECT_SIZE - size; break;
	case LEFT:
		iswap(&r.w, &r.h);
		r.x = (pos.x + 1) * RECT_SIZE - size;
		break;

	case DOWN:  break;
	case RIGHT: iswap(&r.w, &r.h); break;
	}

	return r;
}

static void snake_render_shadow(struct snake *s, SDL_Rect front, SDL_Rect back, SDL_Renderer *ren) {
	SDL_Rect r;
	r.w = RECT_SIZE;
	r.h = RECT_SIZE;

	SDL_SetRenderDrawColor(ren, 0, 0, 0, SHADOW_ALPHA);

	SDL_Rect front_shadow = front;
	front_shadow.x += SHADOW_OFFSET;
	front_shadow.y += SHADOW_OFFSET;
	SDL_RenderFillRect(ren, &front_shadow);

	if (s->prev.x != s->body[s->len - 1].x || s->prev.y != s->body[s->len - 1].y) {
		SDL_Rect back_shadow = back;
		back_shadow.x += SHADOW_OFFSET;
		back_shadow.y += SHADOW_OFFSET;
		SDL_RenderFillRect(ren, &back_shadow);
	}

	for (size_t i = 1; i < s->len; ++ i) {
		if (s->body[i].x == s->body[i - 1].x && s->body[i].y == s->body[i - 1].y)
			continue;

		r.x = s->body[i].x * RECT_SIZE + SHADOW_OFFSET;
		r.y = s->body[i].y * RECT_SIZE + SHADOW_OFFSET;
		SDL_RenderFillRect(ren, &r);
	}
}

static void snake_fade_color(struct snake *s, size_t i, int *r, int *g, int *b) {
	*r = s->r - i;
	*g = s->g - i;
	*b = s->b - i / 2;

	if (*r < 0)
		*r = 0;
	if (*g < 0)
		*g = 0;
	if (*b < 0)
		*b = 0;
}

static void snake_render_body(struct snake *s, SDL_Rect front, SDL_Rect back, SDL_Renderer *ren) {
	SDL_Rect r_;
	r_.w = RECT_SIZE;
	r_.h = RECT_SIZE;

	SDL_SetRenderDrawColor(ren, s->r, s->g, s->b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(ren, &front);

	int r, g, b;
	snake_fade_color(s, s->len, &r, &g, &b);
	SDL_SetRenderDrawColor(ren, r, g, b, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(ren, &back);

	for (size_t i = 1; i < s->len; ++ i) {
		if (s->body[i].x == s->body[i - 1].x && s->body[i].y == s->body[i - 1].y)
			continue;

		r_.x = s->body[i].x * RECT_SIZE;
		r_.y = s->body[i].y * RECT_SIZE;

		snake_fade_color(s, i, &r, &g, &b);
		SDL_SetRenderDrawColor(ren, r, g, b, SDL_ALPHA_OPAQUE);
		SDL_RenderFillRect(ren, &r_);
	}
}

static void snake_render_face(struct snake *s, SDL_Renderer *ren) {
	int offset = s->offset * RECT_SIZE;

	SDL_Rect eyes = {
		.w = RECT_SIZE,
		.h = RECT_SIZE,
		.x = s->head->x * RECT_SIZE,
		.y = s->head->y * RECT_SIZE,
	};

	int tongue_offset = timer_unit(&s->tongue_timer, s->tongue_state != TONGUE_HIDING) * RECT_SIZE;
	SDL_Rect tongue = eyes, src = {
		.x = 0,
		.y = 0,
		.w = RECT_SIZE,
		.h = tongue_offset,
	};

	double angle = dir_to_angle(s->dir);
	switch (s->dir) {
	case UP:
		eyes.y   += RECT_SIZE - offset;
		tongue.y -= offset;

		break;

	case LEFT:
		eyes.x   += RECT_SIZE - offset;
		tongue.x -= offset;

		break;

	case DOWN:
		eyes.y   -= RECT_SIZE - offset;
		tongue.y += offset;

		break;

	case RIGHT:
		eyes.x   -= RECT_SIZE - offset;
		tongue.x += offset;

		break;
	}

	SDL_RenderCopyEx(ren, s->dead? s->textures.eyes_dead : s->textures.eyes,
	                 NULL, &eyes, angle, NULL, SDL_FLIP_NONE);

	if (s->tongue_state != TONGUE_HIDDEN)
		SDL_RenderCopyEx(ren, s->textures.tongue, s->tongue_state == TONGUE_SHOWN? NULL : &src,
		                 &tongue, angle, NULL, SDL_FLIP_NONE);
}

void snake_render(struct snake *s, SDL_Renderer *ren) {
	enum dir dir = dir_from_a_to_b(s->body[s->len - 1], s->prev);
	SDL_Rect front = snake_offset_part_rect(s, *s->head, s->dir, false);
	SDL_Rect back  = snake_offset_part_rect(s, s->prev,  dir,    true);

	snake_render_shadow(s, front, back, ren);
	snake_render_body(s, front, back, ren);
	snake_render_face(s, ren);
}
