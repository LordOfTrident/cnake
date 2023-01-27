#ifndef COMMON_H_HEADER_GUARD
#define COMMON_H_HEADER_GUARD

#include <math.h>   /* pow, floor */
#include <assert.h> /* assert */
#include <stdlib.h> /* rand */

#include <SDL2/SDL.h>

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

#define UNUSED(VAR)      (void)VAR
#define UNREACHABLE(MSG) assert(0 && "Unreachable: "MSG);

extern int    argc;
extern char **argv;

inline void args(int argc_, char **argv_) {
	argc = argc_;
	argv = argv_;
}

int   rand_irange(int min, int max);
float rand_frange(float min, float max, int precision);

void iswap(int *a, int *b);

void SDL_RenderCopyShadowEx(SDL_Renderer *ren, SDL_Texture *texture,
                            SDL_Rect *src, SDL_Rect *dest, double angle,
                            SDL_Point *center, SDL_RendererFlip flip, int offset, int a);
void SDL_RenderCopyShadow(SDL_Renderer *ren, SDL_Texture *texture,
                          SDL_Rect *src, SDL_Rect *dest, int offset, int a);

#endif
