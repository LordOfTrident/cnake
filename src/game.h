#ifndef GAME_H_HEADER_GUARD_
#define GAME_H_HEADER_GUARD_

#include <stdlib.h>  /* exit, EXIT_FAILURE, rand, srand, malloc, free */
#include <stdbool.h> /* bool, true, false */
#include <time.h>    /* time */
#include <math.h>    /* cos, sin */
#include <string.h>  /* memset, strcpy, strcat */
#include <stdint.h>  /* uint32_t */

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "common.h"
#include "timer.h"
#include "particles.h"
#include "snake.h"
#include "cheese.h"

enum {
	TEXTURE_EYES = 0,
	TEXTURE_EYES_DEAD,
	TEXTURE_TONGUE,
	TEXTURE_GRASS1,
	TEXTURE_GRASS2,
	TEXTURE_CHEESE,
	TEXTURE_TUTORIAL,
	TEXTURE_PAUSED,
	TEXTURE_YOU_LOST,
	TEXTURE_SPACEBAR,

	TEXTURES_COUNT,
};

struct texture {
	SDL_Texture *ptr;
	int w, h;
};

enum {
	TIMER_SCR_SHAKE = 0,
	TIMER_FADE_IN,
	TIMER_FADE_OUT,
	TIMER_DEAD,
	TIMER_TRANSITION,

	TIMERS_COUNT,
};

enum state {
	STATE_QUIT = 0,
	STATE_GAMEPLAY,
	STATE_PAUSED,
	STATE_TUTORIAL,
	STATE_DEAD,
};

struct game {
	enum state state;
	size_t     tick;

	struct particles particles;

	SDL_Window   *win;
	SDL_Renderer *ren;

	SDL_Event    evt;
	const Uint8 *keyboard;

	struct snake       snake;
	struct cheese_pool cheese_pool;

	SDL_Texture *map;
	SDL_Rect     map_rect;

	SDL_Point map_shake_pos;

	bool darken_screen;

	struct timer   get_timer[TIMERS_COUNT];
	struct texture get_texture[TEXTURES_COUNT];
};

void game_init(struct game *g);
void game_finish(struct game *g);
void game_render(struct game *g);
void game_handle_events(struct game *g);
void game_update(struct game *g);

#endif
