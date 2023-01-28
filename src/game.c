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
	texture->sdl = SDL_CreateTextureFromSurface(g->ren, s);
	if (texture->sdl == NULL) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	SDL_QueryTexture(texture->sdl, NULL, NULL, &texture->w, &texture->h);

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

#define ASSETS_FOLDER "cnake_assets"

static char *texture_paths[TEXTURES_COUNT] = {
	[TEXTURE_EYES]      = ASSETS_FOLDER"/imgs/eyes.png",
	[TEXTURE_EYES_DEAD] = ASSETS_FOLDER"/imgs/eyes_dead.png",
	[TEXTURE_TONGUE]    = ASSETS_FOLDER"/imgs/tongue.png",
	[TEXTURE_GRASS1]    = ASSETS_FOLDER"/imgs/grass1.png",
	[TEXTURE_GRASS2]    = ASSETS_FOLDER"/imgs/grass2.png",
	[TEXTURE_CHEESE]    = ASSETS_FOLDER"/imgs/cheese.png",
	[TEXTURE_TUTORIAL]  = ASSETS_FOLDER"/imgs/tutorial.png",
	[TEXTURE_PAUSED]    = ASSETS_FOLDER"/imgs/paused.png",
	[TEXTURE_YOU_LOST]  = ASSETS_FOLDER"/imgs/you_lost.png",
	[TEXTURE_SPACEBAR]  = ASSETS_FOLDER"/imgs/spacebar.png",
};

static char *prefix_path(const char *path, const char *prefix) {
	size_t size = strlen(prefix) + strlen(path) + 2;
	char  *buf  = (char*)malloc(size);
	memset(buf, 0, size);

	strcat(buf, prefix);
	strcat(buf, "/");
	strcat(buf, path);

	return buf;
}

static void game_load_assets(struct game *g) {
	char  *exec_folder_path = get_exec_folder_path();

	for (size_t i = 0; i < TEXTURES_COUNT; ++ i) {
		char *path = prefix_path(texture_paths[i], exec_folder_path);
		game_load_texture(g, i, path);
		free(path);
	}

	char *path = prefix_path(ASSETS_FOLDER"/fonts/deja_vu_sans.tff", exec_folder_path);
	g->font = TTF_OpenFont(path, SCORE_FONT_SIZE * 2);
	if (g->font == NULL) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	} else
		SDL_Log("Loaded font from '%s'", path);

	free(path);

	free(exec_folder_path);
}

static void game_restart(struct game *g) {
	g->darken_screen = true;
	g->state         = STATE_TUTORIAL;
	g->score         = 0;

	struct snake_textures textures = {
		.eyes      = g->get_texture[TEXTURE_EYES].sdl,
		.eyes_dead = g->get_texture[TEXTURE_EYES_DEAD].sdl,
		.tongue    = g->get_texture[TEXTURE_TONGUE].sdl,
	};

	SDL_Point start = {
		.x = 5,
		.y = ROWS / 2,
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

	if (TTF_Init() < 0) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	} else
		SDL_Log("Initialized SDL_ttf");

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

	if (SDL_RenderSetLogicalSize(g->ren, WIN_W, WIN_H) != 0) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	} else
		SDL_Log("Render logical size was set");

	g->keyboard = SDL_GetKeyboardState(NULL);

	g->map_rect.x = WIN_W / 2 - MAP_W / 2;
	g->map_rect.y = PADDING * 2 + INFO_H;
	g->map_rect.w = MAP_W;
	g->map_rect.h = MAP_H;

	game_load_assets(g);

	SDL_Log("Loaded assets");

	particles_init(&g->particles);

	for (size_t i = 0; i < TIMERS_COUNT; ++ i)
		timer_init(&g->get_timer[i], timer_times[i]);

	game_restart(g);

	SDL_Log("Initialized");
}

void game_free_assets(struct game *g) {
	for (size_t i = 0; i < TEXTURES_COUNT; ++ i)
		SDL_DestroyTexture(g->get_texture[i].sdl);

	SDL_DestroyTexture(g->score_texture.sdl);
	TTF_CloseFont(g->font);
}

void game_finish(struct game *g) {
	SDL_Log("--------------------------------");

	game_free_assets(g);
	SDL_Log("Destroyed assets");

	SDL_DestroyTexture(g->map);
	SDL_Log("Destroyed the map texture");

	SDL_DestroyRenderer(g->ren);
	SDL_Log("Destroyed the renderer");

	SDL_DestroyWindow(g->win);
	SDL_Log("Destroyed the window");

	SDL_Log("Finalized");

	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

static void game_render_map_grass(struct game *g) {
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
			               TEXTURE_GRASS1].sdl, NULL, &r);
		}
	}

	SDL_Rect v = {
		.x = 0,
		.y = 0,
		.w = SHADOW_OFFSET * 2,
		.h = MAP_H,
	};

	SDL_Rect h = v;
	h.h  = h.w;
	h.w  = MAP_W;
	h.x += SHADOW_OFFSET * 2;
	h.w -= SHADOW_OFFSET * 2;

	SDL_SetRenderDrawColor(g->ren, 0, 0, 0, SHADOW_ALPHA);
	SDL_RenderFillRect(g->ren, &v);
	SDL_RenderFillRect(g->ren, &h);
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

static void game_render_tutorial_ui(struct game *g) {
	game_render_screen_fade(g);

	struct texture *texture = &g->get_texture[TEXTURE_TUTORIAL];
	SDL_Rect r = {
		.x = MAP_W / 2 - texture->w / 2,
		.y = MAP_H - texture->h * 1.5 - sin((float)g->tick / 10) * 5,
		.w = texture->w,
		.h = texture->h,
	};

	SDL_RenderCopyShadow(g->ren, texture->sdl, NULL, &r, SHADOW_OFFSET, SHADOW_ALPHA);
	SDL_RenderCopy(g->ren, texture->sdl, NULL, &r);
}

static void game_render_paused_ui(struct game *g) {
	game_render_screen_fade(g);

		struct texture *texture = &g->get_texture[TEXTURE_PAUSED];
		SDL_Rect r = {
			.x = MAP_W / 2 - texture->w / 2,
			.y = MAP_H / 2 - texture->h / 2,
			.w = texture->w,
			.h = texture->h,
		};

		SDL_RenderCopyShadow(g->ren, texture->sdl, NULL, &r, SHADOW_OFFSET, SHADOW_ALPHA);
		SDL_RenderCopy(g->ren, texture->sdl, NULL, &r);
}

static void game_render_dead_ui(struct game *g) {
	if (!g->darken_screen)
		return;

	game_render_screen_fade(g);

	struct texture *texture = &g->get_texture[TEXTURE_YOU_LOST];
	SDL_Rect r = {
		.x = MAP_W / 2 - texture->w / 2,
		.y = texture->h * 2.5,
		.w = texture->w,
		.h = texture->h,
	};

	float angle = sin((float)g->tick / 20) * 3;

	SDL_RenderCopyShadowEx(g->ren, texture->sdl, NULL, &r, angle, NULL, SDL_FLIP_NONE,
	                       SHADOW_OFFSET, SHADOW_ALPHA);
	SDL_RenderCopyEx(g->ren, texture->sdl, NULL, &r, angle, NULL, SDL_FLIP_NONE);

	texture = &g->get_texture[TEXTURE_SPACEBAR];
	r.x = MAP_W / 2 - texture->w / 2;
	r.y = MAP_H - texture->h * 2.5 - sin((float)g->tick / 10) * 5;
	r.w = texture->w;
	r.h = texture->h;

	SDL_RenderCopyShadow(g->ren, texture->sdl, NULL, &r, SHADOW_OFFSET, SHADOW_ALPHA);
	SDL_RenderCopy(g->ren, texture->sdl, NULL, &r);
}

static void game_render_transition_ui(struct game *g) {
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

static void game_render_ui(struct game *g) {
	switch (g->state) {
	case STATE_TUTORIAL: game_render_tutorial_ui(g); break;
	case STATE_PAUSED:   game_render_paused_ui(g);   break;
	case STATE_DEAD:     game_render_dead_ui(g);     break;
	default: break;
	}

	game_render_transition_ui(g);
}

static void game_render_map(struct game *g) {
	game_render_map_grass(g);
	cheese_pool_render(&g->cheese_pool, g->get_texture[TEXTURE_CHEESE].sdl, g->ren);
	snake_render(&g->snake, g->ren);
	particles_render(&g->particles, g->ren);
}

static void game_fade_out(struct game *g) {
	g->darken_screen = true;
	timer_start(&g->get_timer[TIMER_FADE_OUT]);
}

static void game_fade_in(struct game *g) {
	timer_start(&g->get_timer[TIMER_FADE_IN]);
}

static void game_render_create_score_texture(struct game *g) {
	char text[16] = {0};
	snprintf(text, sizeof(text), "%zu", g->score);
	SDL_Color color = {
		.r = 255,
		.g = 255,
		.b = 255,
		.a = 200,
	};
	SDL_Surface *surface = TTF_RenderText_Solid(g->font, text, color);
	if (surface == NULL) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	g->score_texture.sdl = SDL_CreateTextureFromSurface(g->ren, surface);
	if (g->score_texture.sdl == NULL) {
		SDL_Log("%s", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	g->score_texture.w = surface->w;
	g->score_texture.h = surface->h;

	SDL_FreeSurface(surface);
}

static void game_render_score(struct game *g) {
	struct texture *texture = &g->get_texture[TEXTURE_CHEESE];
	SDL_Rect r = {
		.x = PADDING,
		.y = PADDING,
		.w = texture->w,
		.h = texture->h,
	};
	SDL_RenderCopyShadow(g->ren, texture->sdl, NULL, &r, SHADOW_OFFSET / 1.5, SHADOW_ALPHA);
	SDL_SetTextureAlphaMod(texture->sdl, 220);
	SDL_RenderCopy(g->ren, texture->sdl, NULL, &r);
	SDL_SetTextureAlphaMod(texture->sdl, 255);

	if (g->prev_score != g->score || g->score_texture.sdl == NULL) {
		SDL_DestroyTexture(g->score_texture.sdl);
		game_render_create_score_texture(g);
	}

	r.x += r.w + 10;
	r.y -= 2;
	r.w  = g->score_texture.w / 2;
	r.h  = g->score_texture.h / 2;
	SDL_RenderCopyShadow(g->ren, g->score_texture.sdl, NULL, &r, SHADOW_OFFSET / 1.5, SHADOW_ALPHA);
	SDL_RenderCopy(g->ren, g->score_texture.sdl, NULL, &r);
}

void game_render(struct game *g) {
	SDL_SetRenderDrawColor(g->ren, BG_COLOR_EXPAND, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(g->ren);

	SDL_RenderSetViewport(g->ren, NULL);

	game_render_score(g);

	SDL_SetRenderTarget(g->ren, g->map);
	game_render_map(g);
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

	game_render_ui(g);
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

static void game_update_scr_shake(struct game *g) {
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

static bool game_spawn_cheese(struct game *g) {
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

	return success;
}

static void game_update_gameplay(struct game *g) {
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
			++ g->score;
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

	if (g->tick % CHEESE_SPAWN_TICK_DELAY == 0)
		game_spawn_cheese(g);
}

void game_update(struct game *g) {
	++ g->tick;
	g->prev_score = g->score;

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

		if (g->state != STATE_DEAD)
			game_update_gameplay(g);

		game_update_scr_shake(g);
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
