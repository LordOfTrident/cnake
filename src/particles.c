#include "particles.h"

void particle_start(struct particle *p, float vel, float fric, float angle, size_t time,
                    SDL_Rect dims, int r, int g, int b) {
	p->x = dims.x;
	p->y = dims.y;
	p->w = dims.w;
	p->h = dims.h;

	p->dx = cos(angle * (M_PI / 180));
	p->dy = sin(angle * (M_PI / 180));

	p->vel  = vel;
	p->fric = fric;

	timer_init(&p->timer, time);
	timer_start(&p->timer);

	p->r = r;
	p->g = g;
	p->b = b;
}

bool particle_active(struct particle *p) {
	return timer_active(&p->timer);
}

void particle_update(struct particle *p) {
	if (!timer_active(&p->timer))
		return;

	p->x += p->dx * p->vel;
	p->y += p->dy * p->vel;

	p->vel *= p->fric;

	timer_update(&p->timer);
}

void particle_render(struct particle *p, SDL_Renderer *ren) {
	if (!timer_active(&p->timer))
		return;

	SDL_Rect r = {
		.x = p->x - p->w / 2,
		.y = p->y - p->h / 2,
		.w = p->w,
		.h = p->h,
	};

	SDL_SetRenderDrawColor(ren, p->r, p->g, p->b, timer_unit(&p->timer, false) * 255);
	SDL_RenderFillRect(ren, &r);
}

void particles_init(struct particles *p) {
	memset(p->get, 0, sizeof(p->get));
}

void particles_update(struct particles *p) {
	for (size_t i = 0; i < PARTICLES_CAPACITY; ++ i)
		particle_update(&p->get[i]);
}

void particles_render(struct particles *p, SDL_Renderer *ren) {
	for (size_t i = 0; i < PARTICLES_CAPACITY; ++ i)
		particle_render(&p->get[i], ren);
}
