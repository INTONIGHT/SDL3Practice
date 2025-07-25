#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include<SDL3_image/SDL_image.h>
#include<SDL3/SDL_rect.h>
#include<vector>
#include<string>
#include<array>

#include "GameObject.h"
#include <glm/glm.hpp>
//this sdl main is needed for the sdl to do its thing
using namespace std;
using namespace glm;

struct SDLState {
	SDL_Window *window;
	SDL_Renderer *renderer;
	int width, height, logW, logH;
	const bool *keys;
	SDLState() : keys(SDL_GetKeyboardState(nullptr)){}
};


//structure for the gamestate
const size_t LAYER_IDX_LEVEL = 0;
const size_t LAYER_IDX_CHARACTERS = 1;
struct GameState {
	//ccreating an array of vectors of game objects as vectors allow some flexibility but arrays will be constant
	array<vector<GameObject>, 2> layers;
	int playerIndex;
	GameState() {
		playerIndex = 0; //WILL CHANGE WHEN WE LOAD MAPS

	}
};
//this resources is helping both for setup as well as any other parts of the animation so the main can be neater
struct Resources {
	const int ANIM_PLAYER_IDLE = 0;
	vector<Animation> playerAnims;
	vector<SDL_Texture*> textures;
	SDL_Texture* texIdle;

	SDL_Texture* loadTexture(SDL_Renderer *renderer,const string& filepath) {
		//"data/AnimationSheet_Character.png"
		//needs to use this c.string
		SDL_Texture *tex = IMG_LoadTexture(renderer, filepath.c_str());
		SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
		textures.push_back(tex);
		return tex;
	}

	void load(SDLState& state) {
		playerAnims.resize(5);
		playerAnims[ANIM_PLAYER_IDLE] = Animation(8, 1.6f);
		texIdle = loadTexture(state.renderer, "data/AnimationSheet_Character.png");

	}
	void unload() {
		for (SDL_Texture* tex : textures) {
			SDL_DestroyTexture(tex);
		}
	}
};

void cleanup(SDLState& state);
bool initialize(SDLState& state);
void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float deltaTime);
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);

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
	Resources res;
	res.load(state);
	//setup game data
	GameState gs;
	//create our player object then pushing it into the layers
	GameObject player;
	player.type = ObjectType::player;
	//set player data in the union to playerdata initialize it with the constructors
	player.data.player = PlayerData();
	player.texture = res.texIdle;
	player.animations = res.playerAnims;
	player.currentAnimation = res.ANIM_PLAYER_IDLE;
	//arbitrary values
	player.acceleration = glm::vec2(300, 0);
	player.maxSpeedX = 100;
	gs.layers[LAYER_IDX_CHARACTERS].push_back(player);

	
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
		//update all objects
		for (auto& layer : gs.layers) {
			for (GameObject& obj : layer) {
				update(state, gs, res, obj, deltaTime);
				//update the animation
				if (obj.currentAnimation != -1) {
					//this ties the core game loop to animations
					obj.animations[obj.currentAnimation].step(deltaTime);
				}
			}
		}
		
		
		//perform drawing
		SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
		SDL_RenderClear(state.renderer);
		//so they were using intialiazers which i dont have not sure how to update to latest version of C++
		//but x,y,width height are whats being used here
		//draw all objects
		for (auto& layer : gs.layers) {
			for (GameObject& obj : layer) {
				drawObject(state, gs, obj, deltaTime);
			}
		}
		//swap buffer
		SDL_RenderPresent(state.renderer);
		//dont necessarily need it here but makes it readable
		prevTime = nowTime;
	}

	res.unload();
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
//taking these by reference
void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float deltaTime) {
	const float spriteSize = 32;
	//sees if its animated if it does its going to try to grab the current frame
	float srcX = obj.currentAnimation != -1 ? obj.animations[obj.currentAnimation].currentFrame() * spriteSize : 0.0f;
	//you can directly instantiate ie srcx,0,sprite size within rect but this looks cleaner
	SDL_FRect src{ 
		.x = srcX,
		.y = 0,
		.w = spriteSize,
		.h = spriteSize 
	};
	SDL_FRect dst{ 
		.x = obj.position.x,
		.y = obj.position.y,
		.w = spriteSize,
		.h = spriteSize };
	//how to render the first one
	//SDL_RenderTexture(state.renderer, idleTex, &src, &dst);
	//be able to flip the sprite
	//takes an angle of rotation, centerpoint as well as direction you want to flip it
	//using a ternary to determine when it should be flipped
	SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);
}
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime) {
	//if the object type is the player lets update the player
	if (obj.type == ObjectType::player) {
		float currentDirection = 0;
		//checking if a or d to add or take away 1
		if (state.keys[SDL_SCANCODE_A]) {
			currentDirection += -1;
		}
		if (state.keys[SDL_SCANCODE_D]) {
			currentDirection += 1;
		}
		if (currentDirection) {
			obj.direction = currentDirection;
		}
		//player specific data we take the object the data within that and access the player then access the player state
		switch (obj.data.player.state) {
			//for when the player is idle
		case PlayerState::idle: {
			//if the user is moving then the player state should be running
			if (currentDirection) {
				obj.data.player.state = PlayerState::running;
			}
			else {
				//decelerate
				if (obj.velocity.x) {
					//if velocity decelerate negative and vice verse
					//the factor will slow it down quickly
					const float factor = obj.velocity.x > 0 ? -1.5f : 1.5f;
					float amount = factor * obj.acceleration.x * deltaTime;
					//complete stop if its greater
					if (abs(obj.velocity.x) < abs(amount)) {
						obj.velocity.x = 0;
					}
					else {
						//will then add an inverse amount to the velocity
						obj.velocity.x += amount;
					}
				}
			}
			break;
			}
		case PlayerState::running: {
			if (!currentDirection) {
				obj.data.player.state = PlayerState::idle;
				}
			break;
			}
		}
		//add acceleration to velocity
		obj.velocity += currentDirection * obj.acceleration * deltaTime;
		//check to see if the absolute value of x is greater than maxspeed to then cap the speed
		if (abs(obj.velocity.x) > obj.maxSpeedX) {
			obj.velocity.x = currentDirection * obj.maxSpeedX;
		}
		//add velocity to positionm
		obj.position += obj.velocity * deltaTime;

	}
}