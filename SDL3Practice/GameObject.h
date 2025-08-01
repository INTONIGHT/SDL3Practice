#pragma once
#include <glm/glm.hpp>
#include<vector>
#include "Animation.h"
#include <SDL3/SDL.h>

enum class PlayerState {
	idle,running,jumping
};

enum class BulletState {
	moving, colliding, inactive
};

enum class EnemyState {
	shambling, damaged, dead
};

struct PlayerData {
	PlayerState state;
	Timer weaponTimer;
	PlayerData(): weaponTimer(0.1f){
		state = PlayerState::idle;
	}
};

struct LevelData {};

struct EnemyData{
	EnemyState state;
	Timer damageTimer;
	int healthPoints;
	EnemyData() : state(EnemyState::shambling),damageTimer(0.5f){
		healthPoints = 100;
	}
};

struct BulletData {
	BulletState state;
	BulletData() : state(BulletState::moving){}
};

//create a sturcture of overlapping data types inside of it
union ObjectData {
	PlayerData player;
	LevelData level;
	EnemyData enemy;
	BulletData bullet;
};
enum class ObjectType {
	player,level,enemy, bullet
};

struct GameObject {
	ObjectType type;
	ObjectData data;
	
	glm::vec2 position, velocity, acceleration;
	float direction;
	float maxSpeedX;
	std::vector<Animation> animations;
	int currentAnimation;
	SDL_Texture *texture;
	bool dynamic;
	bool grounded;
	SDL_FRect collider;
	Timer flashTimer;
	bool shouldFlash;
	int spriteFrame;

	GameObject() : data{ .level = LevelData() }, collider{ 0 },flashTimer(0.05f) {
		data.level = LevelData();
		type = ObjectType::level;
		direction = 1;
		maxSpeedX = 0;
		position = velocity = acceleration = glm::vec2(0);
		currentAnimation = -1;
		texture = nullptr;
		dynamic = false;
		grounded = false;
		shouldFlash = false;
		spriteFrame = 1;
	}
};