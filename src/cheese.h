#ifndef CHEESE_H_HEADER_GUARD
#define CHEESE_H_HEADER_GUARD

#include <string.h> /* memset */

#include <SDL2/SDL.h>

#include "common.h"
#include "particles.h"
#include "config.h"

struct cheese {
	SDL_Point at;
	bool      spawned;

	struct particles *particles;
};

#define CHEESE_PARTICLE_MIN_TIME 120
#define CHEESE_PARTICLE_MAX_TIME 200

void cheese_spawn(struct cheese *c, int x, int y);
void cheese_bite(struct cheese *c);
void cheese_render(struct cheese *c, SDL_Texture *texture, SDL_Renderer *ren);
void cheese_eat(struct cheese *c);

#define CHEESE_CAPACITY 256

struct cheese_pool {
	struct cheese    get[CHEESE_CAPACITY];
	struct particles particles;
};

void cheese_pool_init(struct cheese_pool *c);
void cheese_pool_update(struct cheese_pool *c);
void cheese_pool_render(struct cheese_pool *c, SDL_Texture *texture, SDL_Renderer *ren);

#endif
