#include "game.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#	define PLATFORM_WINDOWS
#elif defined(__APPLE__)
#	define PLATFORM_APPLE
#elif defined(__linux__) || defined(__gnu_linux__) || defined(linux)
#	define PLATFORM_LINUX
#elif defined(__unix__) || defined(unix)
#	define PLATFORM_UNIX
#else
#	define PLATFORM_UNKNOWN
#endif

#ifdef PLATFORM_LINUX
# 	define D_POSIX_C_SOURCE 200809L
#	include <unistd.h>
#endif

#ifdef PLATFORM_WINDOWS
#	ifndef MAX_PATH
#		define PATH_MAX 1024
#	else
#		define PATH_MAX MAX_PATH
#	endif
#else
#	ifndef PATH_MAX
#		define PATH_MAX 1024
#	endif
#endif

static char *get_exec_folder_path(void) {
	char *buf = (char*)malloc(PATH_MAX);
	if (buf == NULL)
		UNREACHABLE("malloc() fail");

#if defined(PLATFORM_LINUX)
	ssize_t len = readlink("/proc/self/exe", buf, PATH_MAX);
	if (len != -1)
		buf[len] = '\0';
	else
		strcpy(buf, argv[0]);
#elif defined(PLATFORM_APPLE)
	uint32_t size = PATH_MAX;
	if (_NSGetExecutablePath(buffer, &size) != 0)
		strcpy(buf, argv[0]);
#elif defined(PLATFORM_WINDOWS)
	GetModuleFileName(NULL, buf, PATH_MAX);
#elif defined(PLATFORM_UNIX) || defined(PLATFORM_UNKNOWN)
	strcpy(buf, argv[0]);
#endif

	for (size_t i = len - 1; i > 0; -- i) {
		if (buf[i] == '/' || buf[i] == '\\') {
			buf[i] = '\0';
			break;
		}
	}

	return buf;
}

static void game_load_texture(struct game *g, int key, const char *path) {
	SDL_Surface *s = IMG_Load(path);
	if (s == NULL) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	struct texture *texture = &g->get_texture[key];
	texture->ptr = SDL_CreateTextureFromSurface(g->ren, s);
	if (texture->ptr == NULL) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	SDL_QueryTexture(texture->ptr, NULL, NULL, &texture->w, &texture->h);

	SDL_FreeSurface(s);
	SDL_Log("Loaded texture from '%s'", path);
}

static size_t timer_times[TIMERS_COUNT] = {
	[TIMER_SCR_SHAKE]  = SCR_SHAKE_TIME,
	[TIMER_FADE_IN]    = FADE_IN_TIME,
	[TIMER_FADE_OUT]   = FADE_OUT_TIME,
	[TIMER_DEAD]       = DEAD_TIME,
	[TIMER_TRANSITION] = TRANSITION_TIME,
};

static char *texture_paths[TEXTURES_COUNT] = {
	[TEXTURE_EYES]      = "cnake_res/imgs/eyes.png",
	[TEXTURE_EYES_DEAD] = "cnake_res/imgs/eyes_dead.png",
	[TEXTURE_TONGUE]    = "cnake_res/imgs/tongue.png",
	[TEXTURE_GRASS1]    = "cnake_res/imgs/grass1.png",
	[TEXTURE_GRASS2]    = "cnake_res/imgs/grass2.png",
	[TEXTURE_CHEESE]    = "cnake_res/imgs/cheese.png",
	[TEXTURE_TUTORIAL]  = "cnake_res/imgs/tutorial.png",
	[TEXTURE_PAUSED]    = "cnake_res/imgs/paused.png",
	[TEXTURE_YOU_LOST]  = "cnake_res/imgs/you_lost.png",
	[TEXTURE_SPACEBAR]  = "cnake_res/imgs/spacebar.png",
};

static void game_load_textures(struct game *g) {
	char  *path = get_exec_folder_path();
	size_t len  = strlen(path);
	for (size_t i = 0; i < TEXTURES_COUNT; ++ i) {
		size_t size = strlen(texture_paths[i]) + len + 2;
		char  *buf  = (char*)malloc(size);
		memset(buf, 0, size);

		strcat(buf, path);
		strcat(buf, "/");
		strcat(buf, texture_paths[i]);

		game_load_texture(g, i, buf);

		free(buf);
	}

	free(path);
}

static void game_restart(struct game *g) {
	g->darken_screen = true;
	g->state         = STATE_TUTORIAL;

	struct snake_textures textures = {
		.eyes      = g->get_texture[TEXTURE_EYES].ptr,
		.eyes_dead = g->get_texture[TEXTURE_EYES_DEAD].ptr,
		.tongue    = g->get_texture[TEXTURE_TONGUE].ptr,
	};

	SDL_Point start = {
		.x = 5,
		.y = COLS / 2,
	};

	snake_init(&g->snake, start, textures, SNAKE_COLOR_EXPAND);
	cheese_pool_init(&g->cheese_pool);
}

void game_init(struct game *g) {
	memset(g, 0, sizeof(*g));
	srand(time(NULL));

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	} else
		SDL_Log("Initialized SDL");

	if (IMG_Init(IMG_INIT_PNG) < 0) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	} else
		SDL_Log("Initialized SDL_image");

	g->win = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                          WIN_W, WIN_H, SDL_WINDOW_SHOWN);
	if (g->win == NULL) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	} else
		SDL_Log("Created the window");

	g->ren = SDL_CreateRenderer(g->win, -1, SDL_RENDERER_ACCELERATED);
	if (g->ren == NULL) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	} else
		SDL_Log("Created the renderer");

	g->map = SDL_CreateTexture(g->ren, SDL_PIXELFORMAT_RGBA8888,
	                           SDL_TEXTUREACCESS_TARGET, MAP_W, MAP_H);
	if (g->map == NULL) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	} else
		SDL_Log("Created the map texture");

	if (SDL_SetRenderDrawBlendMode(g->ren, SDL_BLENDMODE_BLEND) < 0) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	} else
		SDL_Log("Render draw blend mode was set");

	if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear")) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	} else
		SDL_Log("Hint 'linear' for SDL_HINT_RENDER_SCALE_QUALITY was set");

	g->keyboard = SDL_GetKeyboardState(NULL);

	g->map_rect.x = WIN_W / 2 - MAP_W / 2;
	g->map_rect.y = PADDING * 3 + INFO_H;
	g->map_rect.w = MAP_W;
	g->map_rect.h = MAP_H;

	game_load_textures(g);

	SDL_Log("Loaded textures");

	particles_init(&g->particles);

	for (size_t i = 0; i < TIMERS_COUNT; ++ i)
		timer_init(&g->get_timer[i], timer_times[i]);

	game_restart(g);

	SDL_Log("Initialized");
}

void game_finish(struct game *g) {
	SDL_Log("--------------------------------");

	for (size_t i = 0; i < TEXTURES_COUNT; ++ i)
		SDL_DestroyTexture(g->get_texture[i].ptr);

	SDL_Log("Destroyed assets");

	SDL_DestroyTexture(g->map);
	SDL_Log("Destroyed the map texture");

	SDL_DestroyRenderer(g->ren);
	SDL_Log("Destroyed the renderer");

	SDL_DestroyWindow(g->win);
	SDL_Log("Destroyed the window");

	SDL_Log("Finalized");

	IMG_Quit();
	SDL_Quit();
}

static void game_render_map(struct game *g) {
	SDL_Rect r = {
		.x = 0,
		.y = 0,
		.w = RECT_SIZE,
		.h = RECT_SIZE,
	};

	for (int y = 0; y < ROWS; ++ y) {
		r.y = y * RECT_SIZE;

		for (int x = 0; x < COLS; ++ x) {
			bool alt = x % 2 == 0;
			if (y % 2 == 0)
				alt = !alt;

			r.x = x * RECT_SIZE;

			SDL_RenderCopy(g->ren, g->get_texture[alt? TEXTURE_GRASS2 :
			               TEXTURE_GRASS1].ptr, NULL, &r);
		}
	}

	SDL_Rect v = {
		.x = 0,
		.y = 0,
		.w = SHADOW_OFFSET * 2,
		.h = MAP_H,
	};

	SDL_Rect h = v;
	iswap(&h.w, &h.h);
	h.x += SHADOW_OFFSET * 2;
	h.w -= SHADOW_OFFSET * 2;

	SDL_SetRenderDrawColor(g->ren, 0, 0, 0, SHADOW_ALPHA);
	SDL_RenderFillRect(g->ren, &v);
	SDL_RenderFillRect(g->ren, &h);
}

static void game_fade_out(struct game *g) {
	g->darken_screen = true;
	timer_start(&g->get_timer[TIMER_FADE_OUT]);
}

static void game_fade_in(struct game *g) {
	timer_start(&g->get_timer[TIMER_FADE_IN]);
}

static void game_render_screen_fade(struct game *g) {
	if (!g->darken_screen)
		return;

	SDL_Rect r = {
		.x = 0,
		.y = 0,
		.w = MAP_W,
		.h = MAP_H,
	};

	int a = DARKEN_SCR_ALPHA;

	struct timer *fade_in  = &g->get_timer[TIMER_FADE_IN];
	struct timer *fade_out = &g->get_timer[TIMER_FADE_OUT];

	if (timer_active(fade_in))
		a = timer_unit(fade_in, false) * 110;
	else if (timer_active(fade_out))
		a = timer_unit(fade_out, true) * 110;

	SDL_SetRenderDrawColor(g->ren, 0, 0, 0, a);
	SDL_RenderFillRect(g->ren, &r);
}

static void game_render_transition(struct game *g) {
	if (!timer_active(&g->get_timer[TIMER_TRANSITION]))
		return;

	SDL_Rect r = {
		.x = 0,
		.y = 0,
		.w = MAP_W,
		.h = MAP_H,
	};

	int a = timer_unit(&g->get_timer[TIMER_TRANSITION], g->state == STATE_DEAD) * 255;

	SDL_SetRenderDrawColor(g->ren, 10, 10, 10, a);
	SDL_RenderFillRect(g->ren, &r);
}

void game_render(struct game *g) {
	SDL_SetRenderDrawColor(g->ren, BG_COLOR_EXPAND, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(g->ren);

	SDL_SetRenderTarget(g->ren, g->map);
	game_render_map(g);
	cheese_pool_render(&g->cheese_pool, g->get_texture[TEXTURE_CHEESE].ptr, g->ren);
	snake_render(&g->snake, g->ren);
	particles_render(&g->particles, g->ren);
	SDL_SetRenderTarget(g->ren, NULL);

	SDL_RenderSetViewport(g->ren, &g->map_rect);
	SDL_Rect back = {
		.x = 0,
		.y = 0,
		.w = MAP_W,
		.h = MAP_H,
	};
	SDL_SetRenderDrawColor(g->ren, MAP_BG_COLOR_EXPAND, SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(g->ren, &back);

	SDL_Rect r = back;
	r.x = g->map_shake_pos.x;
	r.x = g->map_shake_pos.x;
	SDL_RenderCopy(g->ren, g->map, NULL, &r);

	game_render_screen_fade(g);

	switch (g->state) {
	case STATE_TUTORIAL: {
		struct texture *texture = &g->get_texture[TEXTURE_TUTORIAL];
		SDL_Rect r = {
			.x = MAP_W / 2 - texture->w / 2,
			.y = MAP_H - texture->h * 1.5 - sin((float)g->tick / 10) * 5,
			.w = texture->w,
			.h = texture->h,
		};

		SDL_RenderCopyShadow(g->ren, texture->ptr, NULL, &r, SHADOW_OFFSET, SHADOW_ALPHA);
		SDL_RenderCopy(g->ren, texture->ptr, NULL, &r);
	} break;

	case STATE_PAUSED: {
		struct texture *texture = &g->get_texture[TEXTURE_PAUSED];
		SDL_Rect r = {
			.x = MAP_W / 2 - texture->w / 2,
			.y = MAP_H / 2 - texture->h / 2,
			.w = texture->w,
			.h = texture->h,
		};

		SDL_RenderCopyShadow(g->ren, texture->ptr, NULL, &r, SHADOW_OFFSET, SHADOW_ALPHA);
		SDL_RenderCopy(g->ren, texture->ptr, NULL, &r);
	} break;

	case STATE_DEAD: {
		if (!g->darken_screen)
			break;

		struct texture *texture = &g->get_texture[TEXTURE_YOU_LOST];
		SDL_Rect r = {
			.x = MAP_W / 2 - texture->w / 2,
			.y = texture->h * 4,
			.w = texture->w,
			.h = texture->h,
		};

		float angle = sin((float)g->tick / 20) * 3;

		SDL_RenderCopyShadowEx(g->ren, texture->ptr, NULL, &r, angle, NULL, SDL_FLIP_NONE,
		                       SHADOW_OFFSET, SHADOW_ALPHA);
		SDL_RenderCopyEx(g->ren, texture->ptr, NULL, &r, angle, NULL, SDL_FLIP_NONE);

		texture = &g->get_texture[TEXTURE_SPACEBAR];
		r.x = MAP_W / 2 - texture->w / 2;
		r.y = MAP_H - texture->h * 3 - sin((float)g->tick / 10) * 5;
		r.w = texture->w;
		r.h = texture->h;

		SDL_RenderCopyShadow(g->ren, texture->ptr, NULL, &r, SHADOW_OFFSET, SHADOW_ALPHA);
		SDL_RenderCopy(g->ren, texture->ptr, NULL, &r);
	} break;

	default: break;
	}

	game_render_transition(g);

	SDL_RenderSetViewport(g->ren, NULL);
	SDL_RenderPresent(g->ren);
}

static void game_snake_change_dir(struct game *g, enum dir dir) {
	if (g->state == STATE_TUTORIAL && !timer_active(&g->get_timer[TIMER_FADE_IN]))
		game_fade_in(g);
	else if (g->state == STATE_GAMEPLAY)
		snake_change_dir(&g->snake, dir);
}

void game_handle_events(struct game *g) {
	while (SDL_PollEvent(&g->evt)) {
		switch (g->evt.type) {
		case SDL_QUIT: g->state = STATE_QUIT; break;
		case SDL_KEYDOWN:
			switch (g->evt.key.keysym.sym) {
			case SDLK_ESCAPE: g->state = STATE_QUIT; break;

			case SDLK_w: game_snake_change_dir(g, UP);    break;
			case SDLK_a: game_snake_change_dir(g, LEFT);  break;
			case SDLK_s: game_snake_change_dir(g, DOWN);  break;
			case SDLK_d: game_snake_change_dir(g, RIGHT); break;

#ifdef CNAKE_DEBUG
			case SDLK_r: snake_shrink_to(&g->snake, 1); break;
			case SDLK_e: snake_grow(&g->snake);         break;

			case SDLK_q: timer_start(&g->get_timer[TIMER_SCR_SHAKE]); break;
#endif

			case SDLK_SPACE: {
				bool fading = timer_active(&g->get_timer[TIMER_FADE_IN]) ||
				              timer_active(&g->get_timer[TIMER_FADE_OUT]);

				if (g->state == STATE_DEAD && !fading && g->darken_screen)
					timer_start(&g->get_timer[TIMER_TRANSITION]);
				else if (g->state == STATE_PAUSED && !fading)
					game_fade_in(g);
				else if (g->state == STATE_GAMEPLAY && !fading) {
					g->state = STATE_PAUSED;
					game_fade_out(g);
				}

			} break;

			default: break;
			}

		default: break;
		}
	}
}

static void game_emit_snake_particles_at(struct game *g, int x, int y, size_t count) {
	for (size_t i = 0; i < PARTICLES_CAPACITY && count > 0; ++ i) {
		if (particle_active(&g->particles.get[i]))
			continue;

		-- count;

		float  vel   = rand_frange(PARTICLE_MIN_VEL, PARTICLE_MAX_VEL, 2);
		float  angle = rand_irange(0, 360 - 1);
		size_t time  = rand_irange(PARTICLE_MIN_TIME, PARTICLE_MAX_TIME);
		int    size  = rand_irange(PARTICLE_MIN_SIZE, PARTICLE_MAX_SIZE);

		SDL_Rect dims = {
			.x = rand_irange(x * RECT_SIZE, (x + 1) * RECT_SIZE),
			.y = rand_irange(y * RECT_SIZE, (y + 1) * RECT_SIZE),
			.w = size,
			.h = size,
		};

		particle_start(&g->particles.get[i], vel, 0.95, angle, time,
		               dims, SNAKE_PARTICLE_COLOR_EXPAND);
	}
}

#define MAX_RETRIES 10

static bool game_get_new_cheese_pos(struct game *g, SDL_Point *ret) {
	for (int i = 0; i < MAX_RETRIES; ++ i) {
		int x = rand_irange(0, COLS - 1);
		int y = rand_irange(0, ROWS - 1);

		for (size_t i = 0; i < CHEESE_CAPACITY; ++ i) {
			struct cheese *c = &g->cheese_pool.get[i];
			if (c->spawned && c->at.x == x && c->at.y == y)
				goto retry;
		}

		for (size_t i = 0; i < g->snake.len; ++ i) {
			if (g->snake.body[i].x == x && g->snake.body[i].y == y)
				goto retry;
		}

		ret->x = x;
		ret->y = y;
		return true;

	retry:
		continue;
	}

	SDL_Log("Failed to get new cheese position after %i retries", MAX_RETRIES);
	return false;
}

void game_update(struct game *g) {
	++ g->tick;
	for (size_t i = 0; i < TIMERS_COUNT; ++ i) {
		if (i == TIMER_SCR_SHAKE)
			continue;

		timer_update(&g->get_timer[i]);
	}

	cheese_pool_update(&g->cheese_pool);

	if (g->state == STATE_GAMEPLAY || g->state == STATE_DEAD) {
		timer_update(&g->get_timer[TIMER_SCR_SHAKE]);
		particles_update(&g->particles);
		snake_update(&g->snake);

		if (g->state != STATE_DEAD) {
			SDL_Point *head = g->snake.head;

			int prev_head_x = head->x;
			int prev_head_y = head->y;

			struct cheese *cheese = NULL;
			for (size_t i = 0; i < CHEESE_CAPACITY; ++ i) {
				struct cheese *c = &g->cheese_pool.get[i];
				if (c->at.x == prev_head_x && c->at.y == prev_head_y) {
					if (g->tick % 1 == 0)
						cheese_bite(c);

					cheese = c;
					break;
				}
			}

			if (snake_move(&g->snake, SNAKE_SPEED)) {
				if (cheese != NULL) {
					cheese_eat(cheese);
					snake_grow(&g->snake);
				}
			}

			for (size_t i = 1; i < g->snake.len; ++ i) {
				if (g->snake.body[i].x == head->x && g->snake.body[i].y == head->y) {
					game_emit_snake_particles_at(g, g->snake.body[i].x, g->snake.body[i].y,
					                             PARTICLES_ON_SHRINK);
					timer_start(&g->get_timer[TIMER_SCR_SHAKE]);

					snake_shrink_to(&g->snake, i);
					break;
				}
			}

			if (head->x < 0 || head->x >= COLS || head->y < 0 || head->y >= ROWS) {
				game_emit_snake_particles_at(g, prev_head_x, prev_head_y, PARTICLES_ON_SHRINK);
				timer_start(&g->get_timer[TIMER_SCR_SHAKE]);

				g->snake.dead = true;
				g->state = STATE_DEAD;
				timer_start(&g->get_timer[TIMER_DEAD]);
			}

			if (g->tick % CHEESE_SPAWN_TICK_DELAY == 0) {
				size_t i;
				for (i = 0; i < CHEESE_CAPACITY; ++ i) {
					if (!g->cheese_pool.get[i].spawned)
						break;
				}

				if (i >= CHEESE_CAPACITY)
					UNREACHABLE("Cannot spawn any more cheese");

				bool success = game_get_new_cheese_pos(g, &g->cheese_pool.get[i].at);
				if (success)
					g->cheese_pool.get[i].spawned = true;
			}
		}

		struct timer *shake = &g->get_timer[TIMER_SCR_SHAKE];

		int shake_size = timer_unit(shake, false) * SCR_SHAKE_INTENSITY;
		if (shake_size > 0) {
			g->map_shake_pos.x = shake_size / 2 - rand() % shake_size;
			g->map_shake_pos.y = shake_size / 2 - rand() % shake_size;
		} else if (timer_just_ended(shake)) {
			g->map_shake_pos.x = 0;
			g->map_shake_pos.y = 0;
		}
	}

	if (timer_just_ended(&g->get_timer[TIMER_FADE_IN])) {
		g->darken_screen = false;
		g->state         = STATE_GAMEPLAY;
	}

	if (timer_just_ended(&g->get_timer[TIMER_DEAD]))
		game_fade_out(g);

	if (timer_just_ended(&g->get_timer[TIMER_TRANSITION]) && g->state == STATE_DEAD) {
		game_restart(g);
		timer_start(&g->get_timer[TIMER_TRANSITION]);
	}
}
