#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include<SDL3_image/SDL_image.h>
#include<SDL3/SDL_rect.h>
//this sdl main is needed for the sdl to do its thing
using namespace std;

struct SDLState {
	SDL_Window* window;
	SDL_Renderer* renderer;
	int width, height, logW, logH;
};

void cleanup(SDLState &state);
bool initialize(SDLState& state);


int main(int argc, char* argv[]) {
//it needs this argc and argv as well as its pulling it from the command line
//NB You can only have one main file in your project like this otherwise it gets a little confused :)
	SDLState state;
	state.width = 1600;
	state.height = 900;
	state.logW = 640;
	state.logH = 320;
	if (!initialize(state)) {
		return 1;
	}
	//load game assets
	SDL_Texture* idleTex = IMG_LoadTexture(state.renderer, "data/AnimationSheet_Character.png");
	SDL_SetTextureScaleMode(idleTex, SDL_SCALEMODE_NEAREST);
	//setup game data
	const bool *keys = SDL_GetKeyboardState(nullptr);
	float playerX = 150;
	const float floor = state.logH;
	uint64_t prevTime = SDL_GetTicks();

	//start the game loop
	bool running = true;
	while (running) {
		//64 bit unsited interger
		//we need the previous time and current time so we can do some math
		uint64_t nowTime = SDL_GetTicks();
		//we need this as its in milliseconds to convert to seconds
		float deltaTime = (nowTime - prevTime) / 1000.0f;
		SDL_Event event{ 0 };
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				//for when the user wants to quit out themselves
			case SDL_EVENT_QUIT:
				running = false;
				break;
			case SDL_EVENT_WINDOW_RESIZED:
				state.width = event.window.data1;
				state.height = event.window.data2;
				break;
			}
		}
		//handle movement
		float moveAmount = 0;
		//this scancode is taking the input from the user ie A or d then doing something with it
		//these two if statements are done deliberately in case the user holds a and d at the same time
		//we want to move things according to time not frames otherwise every frame it will move this amount which is a lot
		//code things according to time!!
		if (keys[SDL_SCANCODE_A]) {
			moveAmount -= 75.0f;
		}
		if (keys[SDL_SCANCODE_D]) {
			moveAmount += 75.0f;
		}
		playerX += moveAmount * deltaTime;
		//perform drawing
		SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
		SDL_RenderClear(state.renderer);
		//so they were using intialiazers which i dont have not sure how to update to latest version of C++
		//but x,y,width height are whats being used here
		const int spriteSize = 32;
		SDL_FRect src{ 0,0,spriteSize,spriteSize };
		SDL_FRect dst{ playerX,floor - spriteSize ,spriteSize,spriteSize };
		SDL_RenderTexture(state.renderer, idleTex, &src, &dst);
		//swap buffer
		SDL_RenderPresent(state.renderer);
		//dont necessarily need it here but makes it readable
		prevTime = nowTime;
	}

	SDL_DestroyTexture(idleTex);
	cleanup(state);
	return 0;
}
void cleanup(SDLState &state) {
	SDL_DestroyRenderer(state.renderer);
	SDL_DestroyWindow(state.window);
	SDL_Quit();
}
bool initialize(SDLState& state) {
	bool initSuccess = true;
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", nullptr);
		cleanup(state);
		initSuccess = false;
		return 1;
	}
	//create the window
	
	state.window = SDL_CreateWindow("SDL3 practice", state.width, state.height, SDL_WINDOW_RESIZABLE);
	if (!state.window) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error initializing SDL3", state.window);
		cleanup(state);
		initSuccess = false;
		return 1;
	}
	//create the Renderer
	state.renderer = SDL_CreateRenderer(state.window, nullptr);
	if (!state.renderer) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Error Creating Renderer", state.window);
		cleanup(state);
		initSuccess = false;
		return 1;
	}
	//configure presentation
	
	SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	return initSuccess;
}