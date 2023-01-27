#include "common.h"

int    argc = 0;
char **argv = NULL;

int rand_irange(int min, int max) {
	if (max == min)
		return max;

	assert(max > min);
	return min + rand() % (max - min + 1);
}

float rand_frange(float min, float max, int precision) {
	assert(precision > 0);
	precision = pow(10, precision);

	if (max == min)
		return floor(max * precision) / precision;

	assert(max > min);
	min = min * precision;
	max = max * precision;

	return (min + rand() % (int)(max - min + 1)) / precision;
}

void iswap(int *a, int *b) {
	int tmp = *a;
	*a = *b;
	*b = tmp;
}

void SDL_RenderCopyShadowEx(SDL_Renderer *ren, SDL_Texture *texture,
                            SDL_Rect *src, SDL_Rect *dest, double angle,
                            SDL_Point *center, SDL_RendererFlip flip, int offset, int a) {
	SDL_Rect r = *dest;
	r.x += offset;
	r.y += offset;

	SDL_SetTextureColorMod(texture, 0, 0, 0);
	SDL_SetTextureAlphaMod(texture, a);
	SDL_RenderCopyEx(ren, texture, src, &r, angle, center, flip);
	SDL_SetTextureColorMod(texture, 255, 255, 255);
	SDL_SetTextureAlphaMod(texture, 255);
}

void SDL_RenderCopyShadow(SDL_Renderer *ren, SDL_Texture *texture,
                          SDL_Rect *src, SDL_Rect *dest, int offset, int a) {
	SDL_RenderCopyShadowEx(ren, texture, src, dest, 0, NULL, SDL_FLIP_NONE, offset, a);
}
