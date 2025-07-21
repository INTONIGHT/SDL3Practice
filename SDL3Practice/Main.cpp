#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
//this sdl main is needed for the sdl to do its thing
using namespace std;

void cleanup(SDL_Window *win);



int main(int argc, char* argv[]) {
//it needs this argc and argv as well as its pulling it from the command line
//NB You can only have one main file in your project like this otherwise it gets a little confused :)
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", nullptr);
		return 1;
	}
	//create the window
	int width = 600;
	int height = 600;
	SDL_Window *win = SDL_CreateWindow("SDL3 practice", width, height, 0);
	if (!win) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", win);
		return 1;
	}
	//start the game loop
	bool running = true;
	while (running) {
		SDL_Event event{ 0 };
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				//for when the user wants to quit out themselves
			case SDL_EVENT_QUIT:
				running = false;
				break;
			}
		}
	}
	cleanup(win);
	return 0;
}
void cleanup(SDL_Window *win) {
	SDL_DestroyWindow(win);
	SDL_Quit();
}