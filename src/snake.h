#ifndef SNAKE_H_HEADER_GUARD
#define SNAKE_H_HEADER_GUARD

#include <assert.h>  /* static_assert */

#include <SDL2/SDL.h>

#include "common.h"
#include "timer.h"
#include "config.h"

enum dir {
	UP = 0,
	LEFT,
	DOWN,
	RIGHT,
};

enum dir dir_from_a_to_b(SDL_Point a, SDL_Point b);
double   dir_to_angle(enum dir dir);

#define MAX_SNAKE_LEN 256

struct snake_textures {
	SDL_Texture *eyes, *eyes_dead, *tongue;
};

#define SNAKE_TONGUE_TIME      30
#define SNAKE_TONGUE_MOVE_TIME 10

#define SNAKE_TONGUE_MIN_DELAY 300
#define SNAKE_TONGUE_MAX_DELAY 600

static_assert(SNAKE_TONGUE_MAX_DELAY > SNAKE_TONGUE_MIN_DELAY);

enum tongue_state {
	TONGUE_HIDDEN = 0,
	TONGUE_SHOWING,
	TONGUE_SHOWN,
	TONGUE_HIDING,
};

struct snake {
	SDL_Point  body[MAX_SNAKE_LEN];
	SDL_Point *head;
	SDL_Point  prev;

	size_t   len;
	size_t   requested_grow;
	enum dir dir, next_dir;
	float    offset;

	struct timer      tongue_timer;
	enum tongue_state tongue_state;

	struct snake_textures textures;
	int r, g, b;

	bool dead;
};

void snake_init(struct snake *s, SDL_Point start,
                struct snake_textures textures, int r, int g, int b);
void snake_update(struct snake *s);
bool snake_move(struct snake *s, float by);
void snake_grow(struct snake *s);
void snake_shrink_to(struct snake *s, size_t len);
void snake_change_dir(struct snake *s, enum dir dir);
void snake_render(struct snake *s, SDL_Renderer *ren);

#endif
