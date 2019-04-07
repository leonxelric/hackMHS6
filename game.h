#ifndef _GAME_H_
#define _GAME_H_

#include <time.h>
#include <pthread.h>
#include <stdint.h>

#define VERSION (1)

#define RESPONSE_ACK (0)
#define RESPONSE_MISSED_TICK (1)
#define RESPONSE_UNKNOWN_USER (2)
#define RESPONSE_BAD_REQUEST (3)
#define RESPONSE_SERVER_ERROR (4)
#define RESPONSE_DEAD (5)
#define RESPONSE_ALREADY_SUBMITTED (6)

#define TICKLEN (3)

#define ACTION_NONE (0)//not a
#define ACTION_REGISTER (0)//typo
#define ACTION_MOVE (1)
#define ACTION_USE (2)
#define ACTION_INFO (3)
#define ACTION_EAT (4)
#define ACTION_BUY (5)
#define ACTION_SELL (6)
#define ACTION_QUERY_PRICE (7)
#define ACTION_QUERY_INVENTORY (8)
#define ACTION_LOOT (9)
#define ACTION_REFINE (10)
#define ACTION_SEARCH (11)

#define EMPTY (0)
#define PLAYER (1)
#define DIRT (2)
#define STONE (3)
#define ORE (4)
#define PLANT (5)
#define TREE (6)
#define GRASS (7)
#define WATER (8)
#define SEED (9)
#define WOOD_SWORD (10)
#define WOOD_AXE (11)
#define WOOD_SHOVEL (12)
#define WOOD_PICKAXE (13)
#define WOOD_SCYTHE (14)
#define STICK (15)
#define METAL (16)
#define METAL_SWORD (17)
#define METAL_AXE (18)
#define METAL_SHOVEL (19)
#define METAL_PICKAXE (20)
#define METAL_SCYTHE (21)

#define METAL_SWORD_DAMAGE (8)
#define WOOD_SWORD_DAMAGE (3)
#define START_HEALTH (20)
#define START_HUNGER (200)

#define ACTION_UP (0)
#define ACTION_RIGHT (1)
#define ACTION_DOWN (2)
#define ACTION_LEFT (3)

#define STATUS_SUCCESS (0)
#define STATUS_INVALID (1)
#define STATUS_CONFLICT (2)
#define STATUS_SERVER_ERROR (3)
#define STATUS_NO_SPACE (4)

#define IS_USABLE(item) ((item) == DIRT || (item) == STONE || (item) == SEED || \
	   (item) == WOOD_SWORD || (item) == WOOD_AXE || (item) == WOOD_SHOVEL || \
	   (item) == WOOD_PICKAXE || (item) == WOOD_SCYTHE || (item) == METAL_SWORD || \
	   (item) == METAL_AXE || (item) == METAL_SHOVEL || (item) == METAL_PICKAXE || \
	   (item) == METAL_SCYTHE)

#define IS_USABLE_ON(item, tile) ((((item) == DIRT || (item) == STONE || (item) == SEED || (item)) && ((tile) == EMPTY)) || (((item == WOOD_SWORD) || (item) == METAL_SWORD) && ((tile) == PLAYER)) || (((item == WOOD_AXE) || (item) == METAL_AXE) && ((tile) == TREE)) || (((item == WOOD_SHOVEL) || (item) == METAL_SHOVEL) && ((tile) == DIRT)) || (((item == WOOD_SWORD) || ((item) == METAL_SWORD)) && (((tile) == STONE) || ((tile) == ORE))) || (((item == WOOD_SCYTHE) || (item) == METAL_SCYTHE) && ((tile) == PLANT)))

#define IS_BREAKER(item) (((item) == WOOD_AXE) || ((item) == METAL_AXE) || ((item) == WOOD_SHOVEL) || ((item) == METAL_SHOVEL) || ((item) == WOOD_PICKAXE) || ((item) == METAL_PICKAXE) || ((item) == WOOD_SCYTHE) || ((item) == METAL_SCTYTHE))

#define CONSUMES_ACTION(a) ((a) == ACTION_MOVE) || ((a) == ACTION_USE) || ((a) == ACTION_EAT) || ((a) == ACTION_BUY) || ((a) == ACTION_SELL) || ((a) == ACTION_LOOT) || ((a) == ACTION_SEARCH) || ((a) == ACTION_REFINE)

struct action {
	uint8_t action;
	union {
		struct {
			uint8_t direction;
		} move_data;
		struct {
			uint8_t direction;
			uint8_t inven_slot;
		} use_data;
	} data;
};

struct player {
	uint64_t id;
	uint16_t x;
	uint16_t y;
	struct action queued_action;
	uint16_t money;
	uint8_t health;
	uint8_t hunger;
	uint8_t last_action_status;
	struct player *next;
	struct inventory{
		uint8_t item;
		uint8_t amount;//or health
	} inventory[8];
};

struct realm {
	struct domain *dirty_domains;
	uint8_t y: 4;
	uint8_t x: 4;
	uint8_t ndirty_domains;
};

struct domain {
	struct chunk *dirty_chunks;
	uint8_t y: 4;
	uint8_t x: 4;
	uint8_t ndirty_chunks;
};

struct chunk {
	struct tile *dirty_tiles;
	uint8_t y: 4;
	uint8_t x: 4;
	uint8_t ndirty_tiles;
};

struct tile {
	uint8_t y: 4;
	uint8_t x: 4;
	uint8_t type;
	union {
		struct player *player;
		time_t placed_tick;
	} data;
	struct player *influencing_players[4];
};

extern void lltob(unsigned long long n, uint8_t *b);
extern void ltob(unsigned long n, uint8_t *b);
extern void stob(unsigned short n, uint8_t *b);
extern unsigned long long btoll(uint8_t *b);
extern unsigned long btol(uint8_t *b);
extern unsigned short btos(uint8_t *b);

extern time_t next_tick;
extern pthread_mutex_t ticklock;

extern void *worldmanager_routine(void * arg);
extern void *reqhandler_routine(void *pfd_p);

extern int setTile(uint16_t x, uint16_t y, struct tile *t);
extern void getTile(uint16_t x, uint16_t y, struct tile *res);
extern struct player *players;

#endif//_GAME_H_
