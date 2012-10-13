#include "robotmario.h"

#define MAX_SPRITES 32
// The buffer to store the OAM entries between vblanks
OBJ_ATTR obj_buffer[128];
SPRITE sprite_buffer[128];
int last_sprite = 0;
char tmpstr[32];
FIXED horiz_friction;
FIXED vert_friction;
FIXED horiz_terminal_velocity;
FIXED vert_terminal_velocity;
FIXED gravity;
int frame = 0;
int fps = 0;
int vbcount = 0;
int seconds = 0;
int score = 0;
int lives = 3;
SPRITE *player = NULL;

void reset_sprites() {
	int i=0;
	for (i=0; i<128; i++) {
		obj_set_attr(&obj_buffer[i],
			ATTR0_TALL | ATTR0_8BPP | ATTR0_HIDE,
			ATTR1_SIZE_16x32,
			ATTR2_PALBANK(0)
		);	
	}
}

void start_game() {
	se_puts((VID_WIDTH>>1)-44,(VID_HEIGHT>>1)-4,"           ",0);
	
	last_sprite = 0;
	score = 0;
	lives = 3;
	reset_sprites();
	spawn_player();
	spawn_mario();
}

void start_attract() {
	int i=0;
	last_sprite = 0;
	score = 0;
	lives = 0;
	reset_sprites();
	for (i=0; i<8; i++)
		spawn_mario();
}

void game_over() {
	int i=0;
	int j=0;
	int r;
	int g;
	int b;
	char msg[] = "GAME OVER";
	for (j=32; j>=0; j-=2) {
		for (i=0; i<255; i++) {
			r = pal_bg_mem[i] & 31;
			g = (pal_bg_mem[i] & 992) >> 5;
			b = pal_bg_mem[i] >> 10;
			
			if (r>j) r = j;
			if (g>j) g = j;
			if (b>j) b = j;
			
			pal_bg_mem[i] = RGB15(r,g,b);
		}
		vid_vsync();
	}
	
	pal_bg_mem[1] = RGB15(31,31,31);
	se_puts((VID_WIDTH>>1)-36,(VID_HEIGHT>>1)-4,msg,0);
	siprintf(tmpstr,"%-6d",score);
	se_puts(8,8,tmpstr,0);
	siprintf(tmpstr,"M:%d",lives);
	se_puts(VID_WIDTH - 32,8,tmpstr,0);

	for (i=0; i<300; i++) {
		vid_vsync();
	}

	se_puts((VID_WIDTH>>1)-36,(VID_HEIGHT>>1)-4,"         ",0);
	memcpy(pal_bg_mem, (u16*)PAL_BG, 256*2);	
	
	player = NULL;
	start_attract();
}

void vblank() {
	vbcount++;
	if (vbcount == 60) {
		vbcount = 0;
		seconds++;
	}
}

void init_sprite(SPRITE *sprite, const ANIMATION *animation, void (*behavior)(void *), OBJ_ATTR *attr, FIXED x, FIXED y, int i) {
	sprite->attr = attr;
	sprite->animation = animation;
	sprite->x = x;
	sprite->y = y;
	sprite->frame = 0;
	sprite->step = 0;
	sprite->loop = 0;
	sprite->dead = 0;
	sprite->behavior = behavior;
	sprite->num = i;
	
	obj_set_attr(sprite->attr,
		ATTR0_TALL | ATTR0_8BPP,
		ATTR1_SIZE_16x32,
		ATTR2_PALBANK(0) | animation->steps[0].tileno
	);
	obj_set_pos(sprite->attr,x,y);
}

void run_animation(SPRITE *sprite) {
	sprite->frame += last_sprite/32 + 1;	
	
	if (sprite->frame > sprite->animation->steps[sprite->step].frames) {
		sprite->frame = 0;
		sprite->step++;
		
		if (sprite->step >= sprite->animation->length) {
			sprite->step = 0;
			sprite->loop++;
		}

		sprite->attr->attr2 = ATTR2_PALBANK(0) | sprite->animation->steps[sprite->step].tileno;		
	}
}

FIXED friction_offset(FIXED vel, FIXED friction, FIXED terminal_velocity) {
	if (vel > 0) {
		if (vel < friction) {
			return -vel;
		}
		else if (vel >= terminal_velocity) {
			return terminal_velocity - vel;
		}
		else {
			return -friction;
		}
	}
	else if (vel < 0) {
		if (vel > -friction) {
			return vel;
		}
		else if (vel <= -terminal_velocity) {
			return -vel - terminal_velocity;
		}
		else {
			return friction;
		}
	}
	
	return 0;
}

void run_physics(SPRITE *sprite) {
	sprite->x += sprite->x_vel;
	sprite->y += sprite->y_vel;
	
	sprite->x_vel += sprite->x_accel;
	sprite->y_vel += sprite->y_accel;
	
	sprite->x_accel = 0;
	sprite->y_accel = 0;
	
	sprite->y_vel += GRAVITY;
	
	sprite->x_vel += friction_offset(sprite->x_vel,horiz_friction,horiz_terminal_velocity);
	sprite->y_vel += friction_offset(sprite->y_vel,vert_friction,vert_terminal_velocity);
}

void run_collisions(SPRITE *sprite) {
	int y = fx2int(sprite->y);
	int x = fx2int(sprite->x);
	int b = y+30;
	int r = x+16;
	int x2;
	int y2;
	int b2;
	int r2;
	int i;
	int tmp;

	// Colisions with other sprites
	for (i=0; i<last_sprite; i++) {
		SPRITE *sprite2 = &sprite_buffer[i];
		
		// Don't collide with yourself!
		if (sprite2 != sprite && !sprite2->dead) {
			if (sprite->num < sprite2->num) {
				x2 = fx2int(sprite2->x);
				y2 = fx2int(sprite2->y);
				b2 = y2+30;
				r2 = x2+16;
			
				if (b >= y2 && y <= b2 && r >= x2 && x <= r2) {
					if (x<x2) {
						tmp = sprite->x_vel;
						sprite->x_vel = -abs(sprite2->x_vel) - (abs(sprite2->y_vel)>>1);
						sprite2->x_vel = abs(tmp) + (abs(sprite->y_vel)>>1) + int2fx(1);
					}
					else {
						tmp = sprite->x_vel;
						sprite->x_vel = abs(sprite2->x_vel) + (abs(sprite2->y_vel)>>1);
						sprite2->x_vel = -abs(tmp) - (abs(sprite->y_vel)>>1) - int2fx(1);						
					}

					if (b < x2+4)
						sprite->y_vel = -abs(sprite->y_vel);
					else if (b2 < x+4) 
						sprite2->y_vel = abs(sprite->y_vel);
				}
			}
		}
	}

	// Collisions with the floor
	if (sprite->y_vel > 0) {
		if (y > 84 && x > 5 && x < 220 && y < 95) {
			sprite->y = int2fx(84);
			sprite->y_vel = 0;
			//sprite->x_vel = sprite->x_vel >> 1;
		}
	}

}

void remove_dead_sprite(SPRITE *sprite) {
	if (sprite->x > int2fx(240+64) || sprite->x < int2fx(-64) || sprite->y > int2fx(160) || sprite->y < int2fx(-64)) {
		if (sprite == player) {
			sprite->x = int2fx(120);
		}
		else {
			sprite->x = int2fx(rand() % (VID_WIDTH-64));
		}
		sprite->y = 0 - int2fx(rand() % 32);//int2fx(rand() % (VID_HEIGHT-32));
		sprite->frame = rand() % 30;
		sprite->dead = 0;
		sprite->x_vel = 0;
		sprite->y_vel = 0;
		sprite->x_accel = 0;
		sprite->y_accel = 0;
		
		if (sprite != player) {
			score += 100;
			if (score % 2500 == 0)
				lives++;
		}
		else {
			lives--;
			if (lives <= 0) {
				game_over();
			}
		}
	}
}

void run_sprite(SPRITE *sprite) {
	if (sprite->behavior != NULL) {
		sprite->behavior(sprite);
	}
	
	run_animation(sprite);

	if (!sprite->dead) {
		run_physics(sprite);

		run_collisions(sprite);
	
		obj_set_pos(
			sprite->attr,
			fx2int(sprite->x) + sprite->animation->steps[sprite->step].xofs,
			fx2int(sprite->y) + sprite->animation->steps[sprite->step].yofs
		);

		remove_dead_sprite(sprite);
	}

}

void spawn_mario() {
	SPRITE *mario;
	if (last_sprite < MAX_SPRITES) {
		//int x = int2fx(120 + (3 - (rand() % 6)));
		int x = int2fx(rand() % (VID_WIDTH-16));
		int y = 0 - int2fx(rand() % 32);
		mario = &sprite_buffer[last_sprite];
		init_sprite(mario,
			&ANIM_STAND_STILL,
			//(rand() % 5 == 0) ? &behavior_kill_mario : &behavior_get_to_center,
			&behavior_kill_mario,
			&obj_buffer[last_sprite],
			x,
			y,
			last_sprite
		);
		last_sprite++;
	}
}

void spawn_player() {
	player = &sprite_buffer[0];
	last_sprite=1;
	int x = int2fx(120);
	int y = int2fx(-32);
	
	init_sprite(player,&ANIM_PLAYER_STAND_STILL,&behavior_player,&obj_buffer[0],x,y,0);
}

void behavior_clone_after_loop(SPRITE *sprite) {
	if (sprite->loop > 0) {
		sprite->loop = 0;
		spawn_mario();
		//sprite->x = int2fx(120 + (3 - (rand() % 6)));
		sprite->x = int2fx(rand() % (VID_WIDTH-16));
		sprite->y = -100;
		sprite->frame = rand() % 30;
		sprite->dead = 0;

	}
}

void set_animation(SPRITE *sprite, ANIMATION *animation) {
	if (sprite->animation != animation) {
		sprite->animation = animation;
		sprite->frame = 1000;
		sprite->step = 0;
		sprite->loop = 0;
	}
}

void behavior_player(SPRITE *sprite) {
	int direction = key_tri_horz();
	sprite->x_accel = WALK_SPEED * direction;
	
	if (sprite->y_vel == 0 && key_is_down(KEY_A) && sprite->y>int2fx(83)) {
		sprite->y_accel = int2fx(-10);
		if (direction < 0) {
			set_animation(sprite,&ANIM_PLAYER_JUMP_LEFT);
		}
		else {
			set_animation(sprite,&ANIM_PLAYER_JUMP_RIGHT);
		}
	}
	else if (direction < 0) {
		set_animation(sprite,(sprite->x_vel > 0) ? &ANIM_PLAYER_SLIDE_LEFT : &ANIM_PLAYER_WALK_LEFT);
	}
	else if (direction > 0) {
		set_animation(sprite,(sprite->x_vel < 0) ? &ANIM_PLAYER_SLIDE_RIGHT : &ANIM_PLAYER_WALK_RIGHT);		
	}
	else if (!sprite->x_vel) {
		set_animation(sprite,&ANIM_PLAYER_STAND_STILL);
	}
}

void behavior_kill_mario(SPRITE *sprite) {
	if (player != NULL) {
		int jump = sprite->y_vel == 0 && sprite->y>int2fx(83) && rand() % 16 == 0;
		
		if (player->x > sprite->x) {
			sprite->x_accel = ROBOT_SPEED;
		
			if (jump) {
				sprite->y_accel = int2fx(-10);
				set_animation(sprite,&ANIM_JUMP_RIGHT);
			}
			else {
				set_animation(sprite,&ANIM_WALK_RIGHT);
			}
		}
		else {
			sprite->x_accel = -ROBOT_SPEED;
			
			if (jump) {
				sprite->y_accel = int2fx(-10);
				set_animation(sprite,&ANIM_JUMP_LEFT);
			}
			else {
				set_animation(sprite,&ANIM_WALK_LEFT);
			}
		}
	}
	else {
		behavior_get_to_center(sprite);
	}
}

void behavior_get_to_center(SPRITE *sprite) {
	int jump = sprite->y_vel == 0 && sprite->y>int2fx(83) && rand() % 16 == 0;
	int center = (VID_WIDTH<<7);
	
	if (sprite->x < center) {
		if (sprite->x_vel < HORIZ_TERMINAL_VELOCITY || rand() % 64 == 0)
			sprite->x_accel = WALK_SPEED;
		if (jump) {
			sprite->y_accel = int2fx(-10);
			set_animation(sprite,&ANIM_JUMP_RIGHT);
		}
		else {
			set_animation(sprite,&ANIM_WALK_RIGHT);
		}
	}
	else {
		if (sprite->x_vel > -HORIZ_TERMINAL_VELOCITY || rand() % 64 == 0)
			sprite->x_accel = -WALK_SPEED;
		if (jump) {
			sprite->y_accel = int2fx(-10);
			set_animation(sprite,&ANIM_JUMP_LEFT);
		}
		else {
			set_animation(sprite,&ANIM_WALK_LEFT);
		}
	}	
}


int init() {
	int i;
	
	// base text inits
	txt_init_std();
	// tiled bg specific inits
	txt_init_se(0, BG_CBB(1)|BG_SBB(31), 0, CLR_YELLOW, 0);

	memcpy(pal_obj_mem, marioPal, marioPalLen);
	memcpy(&tile_mem[4][0],marioTiles,marioTilesLen);

	memcpy(&tile_mem[0][0], (u8*)TS_BG, 40*128);
	memcpy(pal_bg_mem, (u16*)PAL_BG, 256*2);
	memcpy(&se_mem[24], (u16*)MAP_BG1, 32*20*2);
	memcpy(&se_mem[26], (u16*)MAP_BG2, 32*20*2);
	memcpy(&se_mem[28], (u16*)MAP_BG3, 32*20*2);
	
	// Set up our OAM entry buffer
	oam_init(obj_buffer, 128);
	
	// Set mode 0, bg 0, sprites enabled, 1d sprites
	REG_DISPCNT = DCNT_OBJ | DCNT_OBJ_1D | DCNT_MODE(0) | DCNT_BG1 | DCNT_BG2 | DCNT_BG3 | DCNT_BG0;

	// Set bg 0
	REG_BG1CNT = BG_8BPP | BG_CBB(0) | BG_SBB(24) | BG_REG_32x32;
	REG_BG2CNT = BG_8BPP | BG_CBB(0) | BG_SBB(26) | BG_REG_32x32;
	REG_BG3CNT = BG_8BPP | BG_CBB(0) | BG_SBB(28) | BG_REG_32x32;

	start_attract();
		
	gravity = GRAVITY;
	horiz_terminal_velocity = HORIZ_TERMINAL_VELOCITY;
	vert_terminal_velocity = VERT_TERMINAL_VELOCITY;
	horiz_friction = HORIZ_FRICTION;
	vert_friction = VERT_FRICTION;
	
	irq_init(NULL);
	/*
	irq_add(IRQ_VBLANK,vblank);
	irq_enable(IRQ_VBLANK);
	REG_DISPSTAT |= DSTAT_VBL_IRQ;
	*/
	irq_add(IRQ_VCOUNT, vblank);
	irq_enable(IRQ_VCOUNT);
   // BF_SET(REG_DISPSTAT, 0, DSTAT_VCT_IRQ);
	REG_DISPSTAT |= DSTAT_VCT_IRQ;
 
}

int main() {
	int i=0;
	int j=0;
	int f = 0;
	char str[32];
	
	init();
	
	while(1) {
		frame++;
		f++;
		vid_vsync();
		key_poll();

		if (key_hit(KEY_START) && player == NULL) {
			start_game();
		}
		
		j=last_sprite;
		for (i=0; i<j; i++) {
			run_sprite(&sprite_buffer[i]);
		}
			
		oam_copy(oam_mem, obj_buffer, 128);	
		
		if (f == 2000) {
			f = 0;
			spawn_mario();
		}
		
		if (player != NULL) {
			siprintf(str,"%-6d",score);
			se_puts(8,8,str,0);
			siprintf(str,"M:%d",lives);
			se_puts(VID_WIDTH - 32,8,str,0);
		}
		else {
			se_puts((VID_WIDTH>>1)-44,(VID_HEIGHT>>1)-4,"PRESS START",0);
		}
	}

	return 0;
}

