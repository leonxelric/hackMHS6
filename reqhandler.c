#include "game.h"
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

static struct player *getplayer(uint64_t id){
	struct player *p;
	for(p = players; p; p = p->next)
		if(p->id == id)
			break;
	return p;
}

void *reqhandler_routine(void *pfd_p){
	int pfd, x, y;
	char intime;
	uint8_t buffer[64], error_code, item;
	ssize_t nbytes;
	uint64_t userid;
	struct action requested_action;
	time_t request_time;
	struct player *p;
	struct tile t;

	printf("reqhandler\n");
	pfd = (int) (uintptr_t) pfd_p;
	nbytes = recv(pfd, buffer, 64, 0);

	if(nbytes < 0){
			perror("recv()");
			return NULL;
		} else if(nbytes == 0){ //connection closed without data being sent
			close(pfd);
			return NULL;
		} else if(nbytes < 13){
			error_code = RESPONSE_BAD_REQUEST;
			goto send_error;
		}

		userid = btoll(&buffer[0]);
		request_time = btol(&buffer[8]);
		requested_action.action = buffer[12];
		printf("action: %d\n", buffer[12]);
		printf("userid: %llx\n", userid);
		p = getplayer(userid);
		if(requested_action.action == ACTION_REGISTER){
			p = calloc(1, sizeof(*p));
			if(!p){
				error_code = RESPONSE_SERVER_ERROR;
				goto send_error;
			}
			pthread_mutex_lock(&ticklock);
			do {
				arc4random_buf(&p->id, 8);
			} while(getplayer(p->id));
			do {
				p->x = arc4random();
				p->y = arc4random();
				getTile(p->x, p->y, &t);
			} while(t.type != EMPTY);
			p->next = players;
			players = p;
			p->health = START_HEALTH;
			p->hunger = START_HUNGER;
			p->money = 0;
			p->inventory[0].item = WOOD_AXE;
			p->inventory[0].amount = 50;
			t.type = PLAYER;
			if(setTile(p->x, p->y, &t)){
				fprintf(stderr, "setTile(): OOM\n");
				error_code = RESPONSE_SERVER_ERROR;
				goto send_error;
			}
			pthread_mutex_unlock(&ticklock);
			
			ltob(next_tick, &buffer[0]);
			buffer[4] = RESPONSE_ACK;
			lltob(p->id, &buffer[5]);
			buffer[13] = VERSION;
			stob(TICKLEN, &buffer[14]);
			printf("user registered id = %llx\n", p->id);
			if(send(pfd, buffer, 16, 0) < 0){
				perror("send()");
			}
		} else if(!p){
			error_code = RESPONSE_UNKNOWN_USER;
			goto send_error;
		} else if(p->queued_action.action != ACTION_NONE && \
				CONSUMES_ACTION(requested_action.action)){
			error_code = RESPONSE_ALREADY_SUBMITTED;
			goto send_error;
		} else if(requested_action.action == ACTION_MOVE){
			requested_action.action = ACTION_MOVE;
			requested_action.data.move_data.direction = buffer[13];
			pthread_mutex_lock(&ticklock);
			intime = request_time >= (next_tick - TICKLEN);
			pthread_mutex_unlock(&ticklock);
			if(!intime){
				error_code = RESPONSE_MISSED_TICK;
				goto send_error;
			}
			p->queued_action = requested_action;
			ltob(next_tick, &buffer[0]);
			buffer[4] = RESPONSE_ACK;
			if(send(pfd, buffer, 5, 0) < 0){
				perror("send()");
			}
		} else if(requested_action.action == ACTION_INFO){
			ltob(next_tick, &buffer[0]);
			buffer[4] = RESPONSE_ACK;
			for(x = -2; x < 3; x++){
				for(y = -2; y < 3; y++){
					getTile(p->x + x, p->y + y, &t);
					buffer[(y + 2) * 5 + x + 2 + 5] = t.type;
				}
			}
			buffer[30] = p->last_action_status;
			buffer[31] = p->health;
			buffer[32] = p->hunger;
			stob(p->money, &buffer[33]);
			ssize_t nbytes;
			if((nbytes = send(pfd, buffer, 35, 0)) < 0){
				perror("send()");
			}
			printf("%d bytes sent\n", (int) nbytes);
		} else if(requested_action.action == ACTION_USE){
			requested_action.data.use_data.inven_slot = buffer[13];
			requested_action.data.use_data.direction = buffer[14];
			item = p->inventory[buffer[13]].item;
			pthread_mutex_lock(&ticklock);
			p->queued_action = requested_action;
			intime = request_time >= (next_tick - TICKLEN);
			pthread_mutex_unlock(&ticklock);
			if(!intime){
				error_code = RESPONSE_MISSED_TICK;
				goto send_error;
			}
			p->queued_action = requested_action;
			ltob(next_tick, &buffer[0]);
			buffer[4] = RESPONSE_ACK;
		} else if(requested_action.action == ACTION_EAT){
			pthread_mutex_lock(&ticklock);
			p->queued_action.action = ACTION_EAT;
			if(p->inventory[buffer[13]].item == PLANT){
				if(!(--p->inventory[buffer[13]].amount)){
					p->inventory[buffer[13]].item = EMPTY;
				}
				p->hunger = MIN(START_HUNGER, p->hunger + 50);
				p->last_action_status = STATUS_SUCCESS;
			} else {
				p->last_action_status = STATUS_INVALID;
			}
			pthread_mutex_unlock(&ticklock);
		} else if(requested_action.action == ACTION_QUERY_INVENTORY){
			ltob(next_tick, &buffer[0]);
			buffer[4] = RESPONSE_ACK;
			for(x = 0; x < 8; x++){
				buffer[5 + 2 * x] = p->inventory[x].item;
				buffer[6 + 2 * x] = p->inventory[x].amount;
			}
			if(send(pfd, buffer, 21, 0) < 0){
				perror("send()");
			}
		} else {
			error_code = RESPONSE_BAD_REQUEST;
			goto send_error;
		}

		close(pfd);
		return NULL;	
	send_error:
		ltob(next_tick, &buffer[0]);
		buffer[4] = error_code;
		send(pfd, buffer, 5, 0);
		close(pfd);
		for(x = 0; x < 5; x++){
			printf("%x ", buffer[x]);
		}
	putchar('\n');
	return NULL;	
}
