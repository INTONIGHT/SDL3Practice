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
const int MAP_ROWS = 5;
const int MAP_COLS = 50;
const int TILE_SIZE = 32;

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
	const int ANIM_PLAYER_RUN = 1;
	vector<Animation> playerAnims;
	vector<SDL_Texture*> textures;
	SDL_Texture* texIdle, *texRun, *texBrick, *texGrass, *texGround, *texPanel;

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
		playerAnims[ANIM_PLAYER_RUN] = Animation(4, 0.5f);
		texIdle = loadTexture(state.renderer, "data/idle.png");
		texRun = loadTexture(state.renderer, "data/run.png");
		texBrick = loadTexture(state.renderer, "data/tiles/brick.png");
		texGrass = loadTexture(state.renderer, "data/tiles/grass.png");
		texGround = loadTexture(state.renderer, "data/tiles/ground.png");
		texPanel = loadTexture(state.renderer, "data/tiles/panel.png");
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
void createTiles(const SDLState& state, GameState& gs, const Resources& res);
void checkCollision(const SDLState& state, GameState& gs, const Resources& res, GameObject& a, GameObject& b, float deltaTime);

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
	createTiles(state, gs, res);

	
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
	if (obj.dynamic) {
		//apply some gravity
		obj.velocity += vec2(0, 500) * deltaTime;
	}
	
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
				obj.texture = res.texRun;
				obj.currentAnimation = res.ANIM_PLAYER_RUN;
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
				obj.texture = res.texIdle;
				obj.currentAnimation = res.ANIM_PLAYER_IDLE;
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
		
	}
	//add velocity to positionm
	obj.position += obj.velocity * deltaTime;

	//handle coillisions
	//compare memory addresses then handle the collisions
	//this approach isnt effecient at all but we will do it for now
	//can optimize
	for (auto& layer : gs.layers) {
		for (GameObject& objB : layer) {
			if (&obj != &objB) {
				checkCollision(state, gs, res, obj, objB, deltaTime);
			}
		}
	}
}

void collisionResponse(const SDLState& state, GameState& gs, const Resources& res, 
	const SDL_FRect &rectA, const SDL_FRect& rectB, const SDL_FRect& rectC, 
	GameObject& objA, GameObject& objB, float deltaTime) {

	//first check the type of object A
	if (objA.type == ObjectType::player) {
		//object its colliding with
		switch (objB.type) {
		case ObjectType::level: {
			if (rectC.w < rectC.h) {
				//horizontal collision
				//check if velocity is greater than 0
				if (objA.velocity.x > 0) {
					//object must be to the right
					objA.position.x -= rectC.w;
				}
				else if(objA.velocity.x < 0){
					objA.position.x += rectC.w;
				}
				//set velocity to 0  to stop movement
				objA.velocity.x = 0;
			}
			else {
				//vertical collision
				if (objA.velocity.y > 0) {
					objA.position.y -= rectC.h;//going down
				}
				else if (objA.velocity.y < 0){
					objA.position.y += rectC.h;//going up
				}
				objA.velocity.y = 0;
			}
			break;
			}
		}
	}
}

void checkCollision(const SDLState& state, GameState& gs, const Resources& res, GameObject& a, GameObject& b, float deltaTime) {
	SDL_FRect rectA{
		.x = a.position.x,
		.y = a.position.y,
		.w = TILE_SIZE,
		.h = TILE_SIZE
	};
	//using some rectangles to determine the objects positions and a third for overlap
	SDL_FRect rectB{
		.x = b.position.x,
		.y = b.position.y,
		.w = TILE_SIZE,
		.h = TILE_SIZE
	};
	SDL_FRect rectC{ 0 };
	//pass in the first two and then the result gets passed to c
	if (SDL_GetRectIntersectionFloat(&rectA, &rectB, &rectC)) {
		//if its true its an intersection, respond accordingly
		collisionResponse(state, gs, res, rectA, rectB, rectC, a, b,deltaTime);
	}
}

void createTiles(const SDLState &state, GameState &gs, const Resources &res) 
	{
		//yes gotta do it this way but you can copy paste a lot :)
		/*
		*0 - Nothing
		*1 - Ground
		*2 - Panel
		*3 - Enemy
		*4 - Player
		*5 - Grass
		*6 - Brick
		*/
		short map[MAP_COLS][MAP_COLS] = {
			4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			1,1,1,1,1,1,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		};
		//creating a lambda function to take in the state and the texture to then be able to place tiles in the map
		const auto createObject = [&state](int r, int c, SDL_Texture* tex, ObjectType type) {
			GameObject o;
			o.type = type;
			//subtract tile height from the floor need to subtract to avoid being inverted.
			o.position = vec2(c * TILE_SIZE,state.logH - (MAP_ROWS - r) * TILE_SIZE);
			o.texture = tex;
			return o;
		};
		//loop through rows and columns
		for (int r = 0; r < MAP_ROWS; r++) {
			for (int c = 0; c < MAP_COLS; c++) {
				switch (map[r][c]) {
						case 1: {//ground case
							GameObject o = createObject(r, c, res.texGround, ObjectType::level);
							gs.layers[LAYER_IDX_LEVEL].push_back(o);
							break;
						}
						case 2: {//Panel case
							GameObject o = createObject(r, c, res.texPanel, ObjectType::level);
							gs.layers[LAYER_IDX_LEVEL].push_back(o);
							break;
						}
						case 4: { //player case
						//create our player object then pushing it into the layers
						GameObject player = createObject(r,c,res.texIdle, ObjectType::player);
						//set player data in the union to playerdata initialize it with the constructors
						player.data.player = PlayerData();
						player.animations = res.playerAnims;
						player.currentAnimation = res.ANIM_PLAYER_IDLE;
						//arbitrary values
						player.acceleration = glm::vec2(300, 0);
						player.maxSpeedX = 100;
						player.dynamic = true;
						gs.layers[LAYER_IDX_CHARACTERS].push_back(player);

						break;
						}
					}
				}
			}
	
	}