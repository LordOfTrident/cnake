#ifndef PARTICLES_H_HEADER_GUARD
#define PARTICLES_H_HEADER_GUARD

#include <string.h> /* memset */

#include <SDL2/SDL.h>

#include "timer.h"

struct particle {
	float x, y, w, h, dx, dy;
	float vel, fric;

	struct timer timer;

	int r, g, b;
};

void particle_start(struct particle *p, float vel, float fric, float angle, size_t time,
                    SDL_Rect dims, int r, int g, int b);
bool particle_active(struct particle *p);
void particle_update(struct particle *p);
void particle_render(struct particle *p, SDL_Renderer *ren);

#define PARTICLES_CAPACITY 256

struct particles {
	struct particle get[PARTICLES_CAPACITY];
};

void particles_init(struct particles *p);
void particles_update(struct particles *p);
void particles_render(struct particles *p, SDL_Renderer *ren);

#endif
