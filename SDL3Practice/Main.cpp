#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include<SDL3_image/SDL_image.h>
#include<SDL3/SDL_rect.h>
#include<vector>
#include<string>
#include<array>
#include<format>

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
	vector<GameObject> backgroundTiles;
	vector<GameObject> foregroundTiles;
	vector<GameObject> bullets;
	int playerIndex;
	SDL_FRect mapViewport;
	float bg2Scroll, bg3Scroll, bg4Scroll;
	bool debugMode;

	GameState(const SDLState &state) {
		//represent none
		playerIndex = -1; //WILL CHANGE WHEN WE LOAD MAPS
		mapViewport = SDL_FRect{
			.x = 0,
			.y = 0,
			.w = static_cast<float>(state.logW),
			.h = static_cast<float>(state.logH)
		};
		bg2Scroll = bg3Scroll = bg4Scroll = 0;
		debugMode = false;
	}

	GameObject& player() { return layers[LAYER_IDX_CHARACTERS][playerIndex]; }
};

//this resources is helping both for setup as well as any other parts of the animation so the main can be neater
struct Resources {
	const int ANIM_PLAYER_IDLE = 0;
	const int ANIM_PLAYER_RUN = 1;
	const int ANIM_PLAYER_SLIDE = 2;
	const int ANIM_PLAYER_SHOOT = 3;
	const int ANIM_PLAYER_SLIDE_SHOOT = 4;
	vector<Animation> playerAnims;
	const int ANIM_BULLET_MOVING = 0;
	const int ANIM_BULLET_HIT = 1;
	vector<Animation> bulletAnims;

	vector<SDL_Texture*> textures;
	SDL_Texture* texIdle, *texRun, *texBrick, *texGrass, *texGround, *texPanel, *texSlide,
		*texBg1, *texBg2, *texBg3, *texBg4, *texBullet, *texBulletHit,
		*texShoot, *texRunShoot, *texSlideShoot;

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
		playerAnims[ANIM_PLAYER_SLIDE] = Animation(1, 1.0f);
		playerAnims[ANIM_PLAYER_SHOOT] = Animation(4, 0.5f);
		playerAnims[ANIM_PLAYER_SLIDE_SHOOT] = Animation(4, 0.5f);
		bulletAnims.resize(2);
		bulletAnims[ANIM_BULLET_MOVING] = Animation(4, 0.05f);
		bulletAnims[ANIM_BULLET_HIT] = Animation(4, 0.15f);

		texIdle = loadTexture(state.renderer, "data/idle.png");
		texRun = loadTexture(state.renderer, "data/run.png");
		texSlide = loadTexture(state.renderer, "data/fall.png");
		texBrick = loadTexture(state.renderer, "data/tiles/brick.png");
		texGrass = loadTexture(state.renderer, "data/tiles/grass.png");
		texGround = loadTexture(state.renderer, "data/tiles/ground.png");
		texPanel = loadTexture(state.renderer, "data/tiles/panel.png");
		texBg1 = loadTexture(state.renderer, "data/background/bg_layer1.png");
		texBg2 = loadTexture(state.renderer, "data/background/bg_layer2.png");
		texBg3 = loadTexture(state.renderer, "data/background/bg_layer3.png");
		texBg4 = loadTexture(state.renderer, "data/background/bg_layer4.png");
		texBullet = loadTexture(state.renderer, "data/bullet.png");
		texBulletHit = loadTexture(state.renderer, "data/bullet_hit.png");
		texShoot = loadTexture(state.renderer, "data/shoot.png");
		texRunShoot = loadTexture(state.renderer, "data/shoot_run.png");
		texSlideShoot = loadTexture(state.renderer, "data/slide_shoot.png");

	}
	void unload() {
		for (SDL_Texture* tex : textures) {
			SDL_DestroyTexture(tex);
		}
	}
};

//function decleration area
void cleanup(SDLState& state);
bool initialize(SDLState& state);
void drawObject(const SDLState& state, GameState& gs, GameObject& obj,float width, float height, float deltaTime);
void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime);
void createTiles(const SDLState& state, GameState& gs, const Resources& res);
void checkCollision(const SDLState& state, GameState& gs, const Resources& res, GameObject& a, GameObject& b, float deltaTime);
void handleKeyInput(const SDLState& state, GameState& gs, GameObject& obj, SDL_Scancode key, bool keyDown);
void drawParralaxBackground(SDL_Renderer* renderer, SDL_Texture* texture, float xVelocity, float& scrollPos, float scrollFactor, float deltaTime);

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
	GameState gs(state);
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
			case SDL_EVENT_KEY_DOWN:
				handleKeyInput(state, gs, gs.player(), event.key.scancode, true);
				break;
			case SDL_EVENT_KEY_UP:
				handleKeyInput(state, gs, gs.player(), event.key.scancode, false);
				if (event.key.scancode == SDL_SCANCODE_F12) {
					gs.debugMode = !gs.debugMode;
				}
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

		//update bullets
		for (GameObject& bullet : gs.bullets) {
			update(state, gs, res, bullet, deltaTime);
			//update the animation
			if (bullet.currentAnimation != -1) {
				//this ties the core game loop to animations
				bullet.animations[bullet.currentAnimation].step(deltaTime);
			}
		}
		
		//calculate viewport position
		//generating an x cooredinmate based off the player so that we can center it on the player
		gs.mapViewport.x = (gs.player().position.x + TILE_SIZE / 2) - gs.mapViewport.w / 2;
		
		//perform drawing
		SDL_SetRenderDrawColor(state.renderer, 20, 10, 30, 255);
		SDL_RenderClear(state.renderer);

		//draw Background images
		float scrollFactor = 0.3f;
		SDL_RenderTexture(state.renderer, res.texBg1, nullptr, nullptr);
		drawParralaxBackground(state.renderer, res.texBg4, gs.player().velocity.x, gs.bg4Scroll, scrollFactor / 4, deltaTime);
		drawParralaxBackground(state.renderer, res.texBg3, gs.player().velocity.x, gs.bg3Scroll, scrollFactor / 2, deltaTime);
		drawParralaxBackground(state.renderer, res.texBg2, gs.player().velocity.x, gs.bg2Scroll, scrollFactor, deltaTime);

		//draw background tiles
		for (GameObject& obj : gs.backgroundTiles) {
			SDL_FRect dst{
				.x = obj.position.x - gs.mapViewport.x,
				.y = obj.position.y,
				.w = static_cast<float>(obj.texture->w),
				.h = static_cast<float>(obj.texture->h)
			};
			SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
		 }

		
		
		//so they were using intialiazers which i dont have not sure how to update to latest version of C++
		//but x,y,width height are whats being used here
		//draw all objects
		for (auto& layer : gs.layers) {
			for (GameObject& obj : layer) {
				drawObject(state, gs, obj,TILE_SIZE,TILE_SIZE, deltaTime);
			}
		}

		//draw bullets
		for (GameObject& bullet : gs.bullets) {
			drawObject(state, gs, bullet, bullet.collider.w, bullet.collider.h, deltaTime);
		}
		
		//draw foreground tiles
		for (GameObject& obj : gs.foregroundTiles) {
			SDL_FRect dst{
				.x = obj.position.x - gs.mapViewport.x,
				.y = obj.position.y,
				.w = static_cast<float>(obj.texture->w),
				.h = static_cast<float>(obj.texture->h)
			};
			SDL_RenderTexture(state.renderer, obj.texture, nullptr, &dst);
		}

		if (gs.debugMode) {
			//display some debug info
			SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 255);
			//need to cast to int then to string so 0,1,2 which will correspond to idle running jumping respectively
			SDL_RenderDebugText(state.renderer, 5, 5,
				format("State: {}, B: {}, G: {}"
					, static_cast<int>(gs.player().data.player.state), gs.bullets.size(), gs.player().grounded).c_str());
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
	//run the vsync always
	//not burn through cpu as much
	SDL_SetRenderVSync(state.renderer, 1);

	//configure presentation
	
	SDL_SetRenderLogicalPresentation(state.renderer, state.logW, state.logH, SDL_LOGICAL_PRESENTATION_LETTERBOX);
	return initSuccess;
}

//taking these by reference
void drawObject(const SDLState& state, GameState& gs, GameObject& obj, float width,float height, float deltaTime) {
	
	//sees if its animated if it does its going to try to grab the current frame
	float srcX = obj.currentAnimation != -1 ? obj.animations[obj.currentAnimation].currentFrame() * width : 0.0f;
	//you can directly instantiate ie srcx,0,sprite size within rect but this looks cleaner
	SDL_FRect src{ 
		.x = srcX,
		.y = 0,
		.w = width,
		.h = height 
	};
	SDL_FRect dst{ 
		.x = obj.position.x - gs.mapViewport.x,
		.y = obj.position.y,
		.w = width,
		.h = height 
	};
	//how to render the first one
	//SDL_RenderTexture(state.renderer, idleTex, &src, &dst);
	//be able to flip the sprite
	//takes an angle of rotation, centerpoint as well as direction you want to flip it
	//using a ternary to determine when it should be flipped
	SDL_FlipMode flipMode = obj.direction == -1 ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
	SDL_RenderTextureRotated(state.renderer, obj.texture, &src, &dst, 0, nullptr, flipMode);

	if (gs.debugMode) {
		SDL_FRect rectA{
		.x = obj.position.x + obj.collider.x - gs.mapViewport.x,
		.y = obj.position.y + obj.collider.y,
		.w = obj.collider.w,
		.h = obj.collider.h
		};
		SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(state.renderer, 255, 0, 0, 150);
		SDL_RenderFillRect(state.renderer, &rectA);
		SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_NONE);
	}
}

void update(const SDLState& state, GameState& gs, Resources& res, GameObject& obj, float deltaTime) {
	if (obj.dynamic && !obj.grounded) {
		//apply some gravity
		obj.velocity += vec2(0, 500) * deltaTime;
	}
	float currentDirection = 0;
	//if the object type is the player lets update the player
	if (obj.type == ObjectType::player) {
		
		//checking if a or d to add or take away 1
		if (state.keys[SDL_SCANCODE_A]) {
			currentDirection += -1;
		}
		if (state.keys[SDL_SCANCODE_D]) {
			currentDirection += 1;
		}
		
		Timer& weaponTimer = obj.data.player.weaponTimer;
		weaponTimer.step(deltaTime);

		const auto handleShooting = [&state, &gs, &res, &obj, &weaponTimer](
			SDL_Texture *tex, SDL_Texture *shootTex, int animIndex, int shootAnimIndex) {

			if (state.keys[SDL_SCANCODE_J]) {
				//set shooting tex
				obj.texture = shootTex;
				obj.currentAnimation = shootAnimIndex;
				if (weaponTimer.isTimeout()) {
					weaponTimer.reset();
				}
				//spawn some bullets
				GameObject bullet;
				bullet.type = ObjectType::bullet;
				bullet.direction = gs.player().direction;
				bullet.texture = res.texBullet;
				bullet.currentAnimation = res.ANIM_BULLET_MOVING;
				bullet.collider = SDL_FRect{
					.x = 0,
					.y = 0,
					.w = static_cast<float>(res.texBullet->h),
					.h = static_cast<float>(res.texBullet->h)
				};
				bullet.velocity = glm::vec2(obj.velocity.x + 600.0f * obj.direction, 0);
				bullet.maxSpeedX = 1000.0f;
				bullet.animations = res.bulletAnims;
				//adjust bullet start position
				const float left = 4;
				const float right = 24;
				const float t = (obj.direction + 1) / 2.0f; //results in a value of 0 or 1
				const float xOffset = left + right * t; //LERP equation
				bullet.position = vec2(
					obj.position.x + xOffset,
					obj.position.y + TILE_SIZE / 2 + 1
				);
				//look for an inactive slot and overwrite with a new bullet
				bool foundInactive = false;
				for (int i = 0; i < gs.bullets.size() && !foundInactive; i++) {
					if (gs.bullets[i].data.bullet.state == BulletState::inactive) {
						foundInactive = true;
						gs.bullets[i] = bullet;
					}
				}
				if (!foundInactive) {
					gs.bullets.push_back(bullet);
				}
				
			}
			else {
				obj.texture = tex;
				obj.currentAnimation = animIndex;
			}
		};
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

			
			
			handleShooting(res.texIdle,res.texShoot,res.ANIM_PLAYER_IDLE,res.ANIM_PLAYER_SHOOT);
			//obj.texture = res.texIdle;
			//obj.currentAnimation = res.ANIM_PLAYER_IDLE;
			break;
		}
		//player state of running
		case PlayerState::running: {
			if (!currentDirection) {
				//switching to idle state
				obj.data.player.state = PlayerState::idle;
				
			}

			
			//moving in opposite direction of velocity, sliding
			if (obj.velocity.x * obj.direction < 0 &&  obj.grounded) {
				handleShooting(res.texSlide,res.texSlideShoot,res.ANIM_PLAYER_SLIDE,res.ANIM_PLAYER_SLIDE_SHOOT);
				
			}
			else {
				handleShooting(res.texRun, res.texRunShoot, res.ANIM_PLAYER_RUN, res.ANIM_PLAYER_RUN);
				
			}
			
			break;
			}
		case PlayerState::jumping: {
			handleShooting(res.texRun, res.texRunShoot, res.ANIM_PLAYER_RUN, res.ANIM_PLAYER_RUN);
			
			break;
			}
		}
	}
	else if (obj.type == ObjectType::bullet) {
	if (obj.position.x - gs.mapViewport.x < 0 //left edge
		|| obj.position.x - gs.mapViewport.x > state.logW || //right edge
		obj.position.y - gs.mapViewport.y <0 || //top edge
		obj.position.y -gs.mapViewport.y > state.logH) //bottom edge
	{ 
		obj.data.bullet.state = BulletState::inactive;
	}
	}

	if (currentDirection) {
		obj.direction = currentDirection;
	}
	//add acceleration to velocity
	obj.velocity += currentDirection * obj.acceleration * deltaTime;
	//check to see if the absolute value of x is greater than maxspeed to then cap the speed
	if (abs(obj.velocity.x) > obj.maxSpeedX) {
		obj.velocity.x = currentDirection * obj.maxSpeedX;
	}
	//add velocity to positionm
	obj.position += obj.velocity * deltaTime;

	//handle coillisions
	//compare memory addresses then handle the collisions
	//this approach isnt effecient at all but we will do it for now
	//can optimize
	bool foundGround = false;
	for (auto& layer : gs.layers) {
		for (GameObject& objB : layer) {
			if (&obj != &objB) {
				checkCollision(state, gs, res, obj, objB, deltaTime);
				if (objB.type == ObjectType::level) {
					//grounded sensor
				//when this hits any object on the ground we know the player has landed
					SDL_FRect sensor{
						.x = obj.position.x + obj.collider.x,
						.y = obj.position.y + obj.collider.y + obj.collider.h,
						.w = obj.collider.w,
						.h = 1
					};
					SDL_FRect rectB{
						.x = objB.position.x + objB.collider.x,
						.y = objB.position.y + objB.collider.y,
						.w = objB.collider.w,
						.h = objB.collider.h
					};
					SDL_FRect rectC{ 0 };
					if (SDL_GetRectIntersectionFloat(&sensor, &rectB, &rectC)) {
						foundGround = true;
					}
				}
				
			}
		}
	}
	if (obj.grounded != foundGround) {
		//switching grounded state
		obj.grounded = foundGround;
		if (foundGround && obj.type == ObjectType::player) {
			obj.data.player.state = PlayerState::running;
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
	else if (objA.type == ObjectType::bullet) {
		switch (objA.data.bullet.state) {

		}
	}
}

void checkCollision(const SDLState& state, GameState& gs, const Resources& res, GameObject& a, GameObject& b, float deltaTime) {
	SDL_FRect rectA{
		.x = a.position.x + a.collider.x,
		.y = a.position.y + a.collider.y,
		.w = a.collider.w,
		.h = a.collider.h
	};
	//using some rectangles to determine the objects positions and a third for overlap
	SDL_FRect rectB{
		.x = b.position.x + b.collider.x,
		.y = b.position.y + b.collider.y,
		.w = b.collider.w,
		.h = b.collider.h
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
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,2,0,0,0,0,2,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,2,2,0,0,0,2,1,1,2,2,2,2,1,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			1,1,1,1,1,1,2,1,2,1,2,2,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		};
		short foreground[MAP_COLS][MAP_COLS] = {
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			5,0,0,0,5,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		};
		short background[MAP_COLS][MAP_COLS] = {
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,6,0,0,0,0,0,0,0,0,0,0,0,0,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		};
		const auto loadMap = [&state, &gs, &res](short layer[MAP_ROWS][MAP_COLS]) {
			//creating a lambda function to take in the state and the texture to then be able to place tiles in the map
			const auto createObject = [&state](int r, int c, SDL_Texture* tex, ObjectType type) {
				GameObject o;
				o.type = type;
				//subtract tile height from the floor need to subtract to avoid being inverted.
				o.position = vec2(c * TILE_SIZE, state.logH - (MAP_ROWS - r) * TILE_SIZE);
				o.texture = tex;
				o.collider = {
					.x = 0,
					.y = 0,
					.w = TILE_SIZE,
					.h = TILE_SIZE
				};
				return o;
			};
			//loop through rows and columns
			for (int r = 0; r < MAP_ROWS; r++) {
				for (int c = 0; c < MAP_COLS; c++) {
					switch (layer[r][c]) {
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
						GameObject player = createObject(r, c, res.texIdle, ObjectType::player);
						//set player data in the union to playerdata initialize it with the constructors
						player.data.player = PlayerData();
						player.animations = res.playerAnims;
						player.currentAnimation = res.ANIM_PLAYER_IDLE;
						//arbitrary values
						player.acceleration = glm::vec2(300, 0);
						player.maxSpeedX = 100;
						player.dynamic = true;
						//may need to play around with these values
						player.collider = {
							.x = 11,
							.y = 6,
							.w = 10,
							.h = 26
						};
						gs.layers[LAYER_IDX_CHARACTERS].push_back(player);
						gs.playerIndex = gs.layers[LAYER_IDX_CHARACTERS].size() - 1;

						break;
						}
					case 5: { //grass
						GameObject o = createObject(r, c, res.texGrass, ObjectType::level);
						gs.foregroundTiles.push_back(o);
						break;
						}
					case 6: { //brick
						GameObject o = createObject(r, c, res.texBrick, ObjectType::level);
						gs.backgroundTiles.push_back(o);
						break;
						}
					}
				}
			}
		};
		loadMap(map);
		loadMap(background);
		loadMap(foreground);
		
		//basically to check to make sure the player was actually created
		assert(gs.playerIndex != -1);
	}

void handleKeyInput(const SDLState& state, GameState& gs, GameObject& obj, SDL_Scancode key, bool keyDown) {
	const float JUMP_FORCE = -200.0f;
	if (obj.type == ObjectType::player) {
		switch (obj.data.player.state) {
			case PlayerState::idle:
				if (key == SDL_SCANCODE_K && keyDown) {
					obj.data.player.state = PlayerState::jumping;
					obj.velocity.y += JUMP_FORCE;
				}
				break;
			case PlayerState::running:
				if (key == SDL_SCANCODE_K && keyDown) {
					obj.data.player.state = PlayerState::jumping;
					obj.velocity.y += JUMP_FORCE;
				}
				break;
		}
	}
}

void drawParralaxBackground(SDL_Renderer* renderer, SDL_Texture* texture, float xVelocity, float& scrollPos, float scrollFactor, float deltaTime) {
	scrollPos -= xVelocity * scrollFactor * deltaTime;
	//moves the scroll inverse to player
	//if the scroll position is greater than the width
	if (scrollPos <= -texture->w) {
		scrollPos = 0;
	}
	
	SDL_FRect dst{
		.x = scrollPos,
		.y = 30,
		.w = texture->w * 2.0f,//setting to ortignal  textures width times 2 so it can scroll
		.h = static_cast<float>(texture->h)
	};
	//will allow us to draw the texture twice so we dont need to call it and then it will alays scroll back without user noticing
	SDL_RenderTextureTiled(renderer, texture, nullptr, 1, &dst);
}