#include "cheese.h"

void cheese_spawn(struct cheese *c, int x, int y) {
	c->spawned = true;
	c->at.x    = x;
	c->at.y    = y;
}

void cheese_bite(struct cheese *c) {
	size_t count = PARTICLES_ON_BITE;
	for (size_t i = 0; i < PARTICLES_CAPACITY && count > 0; ++ i) {
		if (particle_active(&c->particles->get[i]))
			continue;

		-- count;

		float  vel   = rand_frange(PARTICLE_MIN_VEL * 4, PARTICLE_MAX_VEL * 4, 2);
		float  angle = rand_irange(0, 360);
		size_t time  = rand_irange(CHEESE_PARTICLE_MIN_TIME, CHEESE_PARTICLE_MAX_TIME);
		int    size  = rand_irange(PARTICLE_MIN_SIZE / 1.1, PARTICLE_MAX_SIZE / 1.1);

		SDL_Rect dims = {
			.x = rand_irange(c->at.x * RECT_SIZE, (c->at.x + 1) * RECT_SIZE),
			.y = rand_irange(c->at.y * RECT_SIZE, (c->at.y + 1) * RECT_SIZE),
			.w = size,
			.h = size,
		};

		particle_start(&c->particles->get[i], vel, 0.9, angle, time,
		               dims, CHEESE_PARTICLE_COLOR_EXPAND);
	}
}

void cheese_render(struct cheese *c, SDL_Texture *texture, SDL_Renderer *ren) {
	if (c->spawned) {
		SDL_Rect r = {
			.x = c->at.x * RECT_SIZE,
			.y = c->at.y * RECT_SIZE,
			.w = RECT_SIZE,
			.h = RECT_SIZE,
		};

		SDL_RenderCopyShadow(ren, texture, NULL, &r, SHADOW_OFFSET / 1.5, SHADOW_ALPHA);
		SDL_RenderCopy(ren, texture, NULL, &r);
	}
}

void cheese_eat(struct cheese *c) {
	c->spawned = false;
}

void cheese_pool_init(struct cheese_pool *c) {
	particles_init(&c->particles);

	memset(c->get, 0, sizeof(c->get));
	for (size_t i = 0; i < CHEESE_CAPACITY; ++ i)
		c->get[i].particles = &c->particles;
}

void cheese_pool_update(struct cheese_pool *c) {
	particles_update(&c->particles);
}

void cheese_pool_render(struct cheese_pool *c, SDL_Texture *texture, SDL_Renderer *ren) {
	particles_render(&c->particles, ren);

	for (size_t i = 0; i < CHEESE_CAPACITY; ++ i)
		cheese_render(&c->get[i], texture, ren);
}
