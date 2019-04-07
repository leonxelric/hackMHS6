#include "game.h"
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
#include <sched.h>
#include <stdio.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

struct player *players;
struct {
	struct realm *dirty_realms;
	uint8_t ndirty_realms;
} world;

time_t next_tick;
pthread_mutex_t ticklock;

#define NOISE_GRID_SIZE (256)
float random_grid[NOISE_GRID_SIZE][NOISE_GRID_SIZE];
static float randomf(){//[-1.0, 1.0]
	return ((float)arc4random() / (float) (1UL << 31)) - 1.0;
}
static void initrandom(){
	size_t i;
	for(i = 0; i < NOISE_GRID_SIZE * NOISE_GRID_SIZE; i++){
		random_grid[i / NOISE_GRID_SIZE][i % NOISE_GRID_SIZE] = randomf();
	}
}

static float smoothstep(float a, float b, float w){
	return b + (a - b) * w * w * w * (w * (w * 6.0 - 15.0) + 10.0);
}

//x,y in [0, 1] loops for values outside
static float random2d(float x, float y){
	x = fmodf(x * NOISE_GRID_SIZE, NOISE_GRID_SIZE);
	y = fmodf(y * NOISE_GRID_SIZE, NOISE_GRID_SIZE);
	float res, x1, x2;
	unsigned i, ix, iy;
	res = 0;
	for(i = 0; i < 4; i++){
		ix = (unsigned) x;
		iy = (unsigned) y;
		x1 = smoothstep(random_grid[ix][iy], random_grid[ix][iy + 1], 
				x - (float)ix);
		x2 = smoothstep(random_grid[ix + 1][iy], random_grid[ix + 1][iy + 1],
				x - (float)ix);
		res += smoothstep(x1, x2, y - (float) iy) / ((float) (2 << i));
		x = fmodf(x * 2, NOISE_GRID_SIZE);
		y = fmodf(y * 2, NOISE_GRID_SIZE);
	}
	return res;
}

static char treecheck(uint16_t x, uint16_t y, float f, float c, float l){
	float a;
	unsigned i;
	a = (x * y);
	for(i = 0; i < 5; i++){
		a = f * a * a;
		a = fmodf(a, c);
	}
	return (a / c) < l;
}

static void genTile(uint16_t x, uint16_t y, struct tile *res){
	float here;
	memset(&res->influencing_players, 0, sizeof(res->influencing_players));
	here = random2d((float) x / 65535.0, (float) y / 65535.0);
	if(here < -0.2){
		res->type = WATER;
	} else if(here < 0.2){
		if(treecheck(x, y, 5.4, 100, 0.3))
			res->type = TREE;
		else if(treecheck(x, y, 27.3, 87, 0.32))
			res->type = GRASS;
		else
			res->type = EMPTY;
	} else if(here < 0.4){
		res->type = DIRT;
	} else if(treecheck(x, y, 12.5, 54.1, 0.2)){
		res->type = ORE;
	} else {
		res->type = STONE;
	}
}

void getTile(uint16_t x, uint16_t y, struct tile *res){
	size_t i;
	struct realm *rp;
	struct domain *dp;
	struct chunk *cp;
	struct tile *tp;
	rp = NULL;
	dp = NULL;
	cp = NULL;
	tp = NULL;

	for(i = 0; i < world.ndirty_realms; i++){
		if(world.dirty_realms[i].x == x >> 12 && \
				world.dirty_realms[i].y == y >> 12){
			rp = &world.dirty_realms[i];
			break;
		}
	}
	if(!rp)
		goto not_dirty;

	for(i = 0; i < rp->ndirty_domains; i++){
		if(rp->dirty_domains[i].x == ((x >> 8) & 15) &&
				rp->dirty_domains[i].y == ((y >> 8) & 15)){
			dp = &rp->dirty_domains[i];
			break;
		}
	}
	if(!dp)
		goto not_dirty;

	for(i = 0; i < dp->ndirty_chunks; i++){
		if(dp->dirty_chunks[i].x == ((x >> 4) & 15) &&
				dp->dirty_chunks[i].y == ((y >> 4) & 15)){
			cp = &dp->dirty_chunks[i];
		}
	}
	if(!cp)
		goto not_dirty;

	for(i = 0; i < cp->ndirty_tiles; i++){
		if(cp->dirty_tiles[i].x == (x & 15) &&
				cp->dirty_tiles[i].y == (y & 15)){
			*res = cp->dirty_tiles[i];
			return;
		}
	}

not_dirty:
	genTile(x, y, res);
}

int setTile(uint16_t x, uint16_t y, struct tile *t){
	struct realm *rp;
	struct domain *dp;
	struct chunk *cp;
	struct tile *tp;
	unsigned i;
	void *tmp;

	rp = NULL;
	dp = NULL;
	cp = NULL;
	tp = NULL;

	for(i = 0; i < world.ndirty_realms; i++){
		if(world.dirty_realms[i].x == (x >> 12) &&
				world.dirty_realms[i].y == (y >> 12)){
			rp = &world.dirty_realms[i];
			break;
		}
	}
	if(!rp){
		tmp = realloc(world.dirty_realms, 
				(world.ndirty_realms + 1) * sizeof(struct realm));
		if(!tmp)
			return 1;
		world.dirty_realms = tmp;
		rp = &world.dirty_realms[world.ndirty_realms++];
		rp->x = x >> 12;
		rp->y = y >> 12;
		rp->ndirty_domains = 0;
		rp->dirty_domains = NULL;
	}
	
	for(i = 0; i < rp->ndirty_domains; i++){
		if(rp->dirty_domains[i].x == ((x >> 8) & 15) &&
				rp->dirty_domains[i].y == ((y >> 8) & 15)){
			dp = &rp->dirty_domains[i];
			break;
		}
	}

	if(!dp){
		tmp = realloc(rp->dirty_domains,
				(rp->ndirty_domains + 1) * sizeof(struct domain));
		if(!tmp)
			return 1;
		rp->dirty_domains = tmp;
		dp = &rp->dirty_domains[rp->ndirty_domains++];
		dp->x = (x >> 8) & 15;
		dp->y = (y >> 8) & 15;
		dp->ndirty_chunks = 0;
		dp->dirty_chunks = NULL;
	}

	for(i = 0; i < dp->ndirty_chunks; i++){
		if(dp->dirty_chunks[i].x == ((x >> 4) & 15) &&
				dp->dirty_chunks[i].y == ((y >> 4) & 15)){
			cp = &dp->dirty_chunks[i];
			break;
		}
	}

	if(!cp){
		tmp = realloc(dp->dirty_chunks,
				(dp->ndirty_chunks + 1) * sizeof(struct domain));
		if(!tmp)
			return 1;
		dp->dirty_chunks = tmp;
		cp = &dp->dirty_chunks[dp->ndirty_chunks++];
		cp->x = (x >> 4) & 15;
		cp->y = (y >> 4) & 15;
		cp->ndirty_tiles = 0;
		cp->dirty_tiles = NULL;
	}
	
	for(i = 0; i < cp->ndirty_tiles; i++){
		if(cp->dirty_tiles[i].x == (x & 15) &&
				cp->dirty_tiles[i].y == (y & 15)){
			tp = &cp->dirty_tiles[i];
			break;
		}
	}
	
	if(!tp){
		tmp = realloc(cp->dirty_tiles,
				(cp->ndirty_tiles + 1) * sizeof(struct tile));
		if(!tmp)
			return 1;
		cp->dirty_tiles = tmp;
		tp = &cp->dirty_tiles[cp->ndirty_tiles++];
	}
	*tp = *t;
	return 0;
}
static void newpos(uint16_t *x, uint16_t *y, uint8_t direction){
	if(direction == ACTION_UP)
		(*y)++;
	else if(direction == ACTION_RIGHT)
		(*x)++;
	else if(direction == ACTION_DOWN)
		(*y)--;
	else if(direction == ACTION_LEFT)
		(*x)--;	
}

static char multiple_influencers(const struct tile *t){
	int res;
	if(t->influencing_players[0])
		res++;
	if(t->influencing_players[1])
		res++;
	if(t->influencing_players[2])
		res++;
	if(t->influencing_players[3])
		res++;
	return res > 1;
}

//assumes space is avaliable
static char addtoinventory(struct player *p, uint8_t item, uint8_t amt){
	unsigned i;
	for(i = 0; i < 8; i++){
		if(p->inventory[i].item == item){
			if(p->inventory[i].amount <= 255 - amt){
				p->inventory[i].amount++;
				return 0;
			}
		}
	}
	for(i = 0; i < 8; i++){
		if(p->inventory[i].item == EMPTY){
			p->inventory[i].item = item;
			p->inventory[i].amount = amt;
			return 0;
		}
	}
	return 1; 
}

void *worldmanager_routine(void * arg){
	time_t current_time;
	struct player *p;
	uint16_t x,y;
	struct tile t;
	uint8_t item;
	unsigned i;
	players = NULL;
	world.ndirty_realms = 0;

	initrandom();
	next_tick = (time(NULL) / TICKLEN) * TICKLEN + TICKLEN;
	for(;;){
		while(time(NULL) < next_tick)
			sched_yield();
		pthread_mutex_lock(&ticklock);
		//Tick
		printf("tick\n");
		for(p = players; p; p = p->next){
			if(p->hunger){
				p->hunger--;
			} else if(p->health){
				p->health--;
			}
			switch(p->queued_action.action){
			case ACTION_MOVE:
				x = p->x;
				y = p->y;
				newpos(&x, &y, p->queued_action.data.move_data.direction);
				getTile(x, y, &t);
				if(t.type != EMPTY || p->queued_action.data.move_data.direction > 3){
					p->queued_action.action = ACTION_NONE;
					p->last_action_status = STATUS_INVALID;
					break;
				}
				t.influencing_players[p->queued_action.data.move_data.direction] = p;
				if(setTile(x, y, &t)){
					fprintf(stderr, "setTile(): OOM\n");
					p->last_action_status = STATUS_SERVER_ERROR;
					p->queued_action.action = ACTION_NONE;
				}
				break;
			
			case ACTION_USE:
				item = p->inventory[p->queued_action.data.use_data.inven_slot].item;
				if(!IS_USABLE(item)){
					p->last_action_status = STATUS_INVALID;
					p->queued_action.action = ACTION_NONE;
					break;
				}
				x = p->x;
				y = p->y;
				newpos(&x, &y, p->queued_action.data.use_data.direction);
				getTile(x, y, &t);
				if(!IS_USABLE_ON(item, t.type)){
					p->last_action_status = STATUS_INVALID;
					p->queued_action.action = ACTION_NONE;
					break;
				}
				t.influencing_players[p->queued_action.data.use_data.direction] = p;
				if(setTile(x, y, &t)){
					fprintf(stderr, "setTile(): OOM\n");
					p->last_action_status = STATUS_SERVER_ERROR;
					p->queued_action.action = ACTION_NONE;
				}
				if(item == METAL_SWORD){
					t.data.player->health -= MIN(t.data.player->health, METAL_SWORD_DAMAGE);
					p->queued_action.action = ACTION_NONE;
				} else if(item == WOOD_SWORD){
					t.data.player->health -= MIN(t.data.player->health, WOOD_SWORD_DAMAGE);
					p->queued_action.action = ACTION_NONE;
				}
				break;

			case ACTION_EAT:
			case ACTION_BUY:
			case ACTION_SELL:
			case ACTION_LOOT:
			case ACTION_REFINE:
			case ACTION_SEARCH:
			case ACTION_NONE:
			default:
				p->queued_action.action = ACTION_NONE;
				break;
			}
		}

		for(p = players; p; p = p->next){
			if(p->queued_action.action == ACTION_NONE)
				continue;
			x = p->x;
			y = p->y;
			if(p->queued_action.action == ACTION_MOVE)
				newpos(&x, &y, p->queued_action.data.move_data.direction);
			else if(p->queued_action.action == ACTION_USE)
				newpos(&x, &y, p->queued_action.data.use_data.direction);
			getTile(x, y, &t);
			if(multiple_influencers(&t)){
				for(i = 0; i < 4; i++){
					if(t.influencing_players[i]){
						t.influencing_players[i]->last_action_status = \
							STATUS_CONFLICT;
						t.influencing_players[i]->queued_action.action = \
							ACTION_NONE;
					}
				}
			}
			if(p->queued_action.action == ACTION_MOVE){
				t.type = PLAYER;
				t.data.player = p;
				//I know the tile is already alloc'ed so it can't fail
				setTile(x, y, &t);
				getTile(p->x, p->y, &t);
				t.type = EMPTY;
				setTile(p->x, p->y, &t);
			}
			if(p->queued_action.action == ACTION_USE){
				i = p->queued_action.data.use_data.inven_slot;
				switch(p->inventory[i].item){
				case DIRT:
				case STONE:
					t.type = p->inventory[i].item;
					if(--p->inventory[i].amount == 0){
						p->inventory[i].item = EMPTY;
					}
					break;

				case WOOD_SCYTHE:
				case METAL_SCYTHE:
					t.type = EMPTY;
					if(addtoinventory(p, PLANT, 1) || addtoinventory(p, SEED, 1)){
						p->last_action_status = STATUS_NO_SPACE;
					}
					break;
				default:
					addtoinventory(p, t.type, 1);
					t.type = EMPTY;
					break;
				}
				//cant fail
				setTile(x, y, &t);
			}
			p->queued_action.action = ACTION_NONE;
			
		}

		next_tick += TICKLEN;
		pthread_mutex_unlock(&ticklock);
	}
	return NULL;
}

