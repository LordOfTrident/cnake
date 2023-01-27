#include <stdlib.h>  /* EXIT_SUCCESS */

#include "game.h"

int main(int argc_, char **argv_) {
	args(argc_, argv_);

	struct game g = {0};
	game_init(&g);

	while (g.state != STATE_QUIT) {
		game_render(&g);
		game_handle_events(&g);
		game_update(&g);

		SDL_Delay(1000 / 60);
	}

	game_finish(&g);
	return EXIT_SUCCESS;
}
