#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>//only for mutex

#include "game.h"

#define SERVICE ("12345")
#define LISTEN_BACKLOG (5)

int sfd;

void sendservererror(int pfd);

int main(){
	struct addrinfo *res, *rp, hints;
	int s, pfd;
	struct sockaddr paddr;
	socklen_t paddrlen;
	pthread_t worldmanager, reqhandler;

	if(pthread_mutex_init(&ticklock, NULL)){
		perror("pthread_mutex_init()");
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_protocol = 0;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

	if((s = getaddrinfo(NULL, SERVICE, &hints, &res))){
		fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}
	s = 1;//for setsockopt
	for(rp = res; rp != NULL; rp = rp->ai_next){
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sfd < 0)
			continue;

		if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &s, sizeof(s))){
			perror("setsockopt()");
			exit(EXIT_FAILURE);
		}
			
		
		if(bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;
		close(sfd);
		sfd = -1;
	}
	if(sfd < 0){
		fprintf(stderr, "could not bind\n");
		exit(EXIT_FAILURE);
	}

	if(listen(sfd, LISTEN_BACKLOG)){
		perror("listen()");
		exit(EXIT_FAILURE);
	}
	printf("listening...\n");

	if(pthread_create(&worldmanager, NULL, worldmanager_routine, NULL)){
		perror("worldmanager: pthread_create()");
		exit(EXIT_FAILURE);
	}

	next_tick = 0;
	for(;;){
		pfd = accept(sfd, &paddr, &paddrlen);
		if(pfd < 0){
			perror("accept()");
			continue;
		}
		printf("accepted\n");
		
		if(pthread_create(&reqhandler, NULL, reqhandler_routine, 
					(void *) (uintptr_t) pfd)){
			perror("reqhandler: pthread_create()");
			sendservererror(pfd);
			close(pfd);
			continue;
		}

		if(pthread_detach(reqhandler)){
			perror("reqhandler: pthread_detach()");
			sendservererror(pfd);
			close(pfd);
			continue;
		}
	}

	close(sfd);

	exit(EXIT_SUCCESS);
}

void sendservererror(int pfd){
	uint8_t buf[5];
	ltob(next_tick, &buf[0]);
	buf[4] = RESPONSE_SERVER_ERROR;
	send(pfd, buf, 5, 0);
}
