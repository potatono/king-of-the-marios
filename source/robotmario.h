#ifndef TONC_MAIN
#include <tonc.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "mario.h"
#include "bg.h"

#define HORIZ_FRICTION float2fx(0.2)
#define VERT_FRICTION float2fx(0.10)
#define GRAVITY float2fx(0.35)
#define WALK_SPEED float2fx(0.30)
#define ROBOT_SPEED float2fx(0.22)
#define HORIZ_TERMINAL_VELOCITY float2fx(4)
#define VERT_TERMINAL_VELOCITY float2fx(7)

typedef struct ANIM_STEP {
	int tileno;
	int frames;
	int xofs;
	int yofs;
} ANIM_STEP;

typedef struct ANIMATION {
	int length;
	ANIM_STEP steps[];
} ANIMATION;

const ANIMATION ANIM_LOOKAROUND = {
	// Animation length
	8,
	// Animation steps
	{ 
		// tile, frames, xofs, yofs
		{ 22*16, 30, 0, 0 },
		{ 23*16, 30, 0, -1 },
		{ 24*16, 30, 0, -1 },
		{ 25*16, 30, -1, 0 },
		{ 3*16, 30, 0, -1 },
		{ 28*16, 30, 0, 0 },
		{ 30*16, 30, -1, -1 },
		{ 31*16, 30, 0, 0 }
	}
};

const ANIMATION ANIM_STAND_STILL = {
	1,
	{
		{ 55*16, 30, 0, 0 }
	}
};

const ANIMATION ANIM_WALK_RIGHT = {
	4,
	{
		{ 55*16, 2, 0, 0 },
		{ 54*16, 2, 0, 0 },
		{ 55*16, 2, 0, 0 },
		{ 59*16, 2, 0, 0 }
	}
};

const ANIMATION ANIM_WALK_LEFT = {
	4,
	{
		{ 52*16, 2, 0, 0 },
		{ 53*16, 2, 0, 0 },
		{ 52*16, 2, 0, 0 },
		{ 56*16, 2, 0, 0 }	
	}
};

const ANIMATION ANIM_JUMP_RIGHT = {
	1,
	{
		{ 58*16, 30, 0, 0 }
	}
};

const ANIMATION ANIM_JUMP_LEFT = {
	1,
	{
		{ 57*16, 30, 0, 0 }
	}
};

const ANIMATION ANIM_PLAYER_WALK_LEFT = {
	4,
	{
		{ 48*16, 2, 0, 0 },
		{ 49*16, 2, 0, 0 },
		{ 48*16, 2, 0, 0 },
		{ 50*16, 2, 0, 0 }	
	}
};

const ANIMATION ANIM_PLAYER_WALK_RIGHT = {
	4,
	{
		{ 9*16, 2, 0, 0 },
		{ 8*16, 2, 0, 0 },
		{ 9*16, 2, 0, 0 },
		{ 16*16, 2, 0, 0 }	
	}
};

const ANIMATION ANIM_PLAYER_JUMP_LEFT = {
	1,
	{
		{ 14*16, 30, 0, 0 }
	}
};

const ANIMATION ANIM_PLAYER_JUMP_RIGHT = {
	1,
	{
		{ 15*16, 30, 0, 0 }
	}
};

const ANIMATION ANIM_PLAYER_STAND_STILL = {
	1,
	{
		{ 11*16, 30, 0, 0 }
	}
};

const ANIMATION ANIM_PLAYER_SLIDE_LEFT = {
	1,
	{
		{ 10*16, 30, 0, 0 }
	}
};

const ANIMATION ANIM_PLAYER_SLIDE_RIGHT = {
	1,
	{
		{ 19*16, 30, 0, 0 }
	}
};

typedef struct SPRITE {
	OBJ_ATTR *attr;
	const ANIMATION *animation;
	void (*behavior)(void*);
	int frame;
	int step;
	int loop;
	int num;
	FIXED x;
	FIXED y;
	FIXED x_vel;
	FIXED y_vel;
	FIXED x_accel;
	FIXED y_accel;
	int dead;
} SPRITE;

void spawn_mario();
void behavior_clone_after_loop(SPRITE *sprite);
void behavior_get_to_center(SPRITE *sprite);
void behavior_kill_mario(SPRITE *sprite);
void behavior_player(SPRITE *sprite);
