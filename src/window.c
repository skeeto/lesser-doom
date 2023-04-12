typedef struct Window* Window;

struct Window {

    int width;
    int height;

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture *pixels;
};

Window windowCreate(int width, int height) {

    Window window = malloc(sizeof(struct Window));

    window->height = height;
    window->width = width;

    return window;
}

void windowDestroy(Window window) {
    
    SDL_DestroyTexture(window->pixels);
	window->pixels = NULL;
    SDL_DestroyRenderer(window->renderer);
	window->renderer = NULL;
	SDL_DestroyWindow(window->window);
	window->window = NULL;

	// Quit SDL subsystems
	SDL_Quit();
}

bool windowInit(Window window) {

    bool success = true;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        
		SDL_Log("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;

	} else {

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

		window->window = SDL_CreateWindow(
            "Lesser Doom", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
			window->width, window->height, SDL_WINDOW_RESIZABLE
		);

		if(window->window == NULL) {

			SDL_Log("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;

		} else {

            window->renderer = SDL_CreateRenderer(
                window->window, -1, SDL_RENDERER_PRESENTVSYNC
            );

			if(window->renderer == NULL) {
				SDL_Log("SDL: renderer could not be created!\nSDL Error: %s\n", SDL_GetError());
				success = false;
			}

			SDL_SetRelativeMouseMode(SDL_TRUE);
		}
	}

	return success;
}

void windowSwap(Window window) {
    SDL_GL_SwapWindow(window->window);
}