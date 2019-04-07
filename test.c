#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

static void ltob(unsigned long n, uint8_t *buf){
	int i;
	for(i = 0; i < 4; i++){
		buf[i] = n;
		n >>= 8;
	}
}

int main(int argc, char *argv[]){
	uint8_t userid[8];
	uint8_t buffer[64];
	struct addrinfo *res, *rp, hints;
	int s, sfd;
	ssize_t nbytes;

	if(argc != 3){
		fprintf(stderr, "usage: %s host port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_NUMERICSERV;
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	s = getaddrinfo(argv[1], argv[2], &hints, &res);
	if(s){
		fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	for(rp = res; rp; rp = rp->ai_next){
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(sfd < 0)
			continue;
		if(connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;
		sfd = -1;
	}
	if(sfd == -1){
		fprintf(stderr, "failed to connect\n");
		exit(EXIT_FAILURE);
	}
	printf("connected\n");

	memset(buffer, 0, 13);
	if(send(sfd, buffer, 13, 0) < 13){
		perror("send()");
		exit(EXIT_FAILURE);
	}

	nbytes = recv(sfd, buffer, 64, 0);
	if(nbytes < 0){
		perror("recv()");
		exit(EXIT_FAILURE);
	}
	if(nbytes == 0){
		fprintf(stderr, "server unexpectedly closed\n");
		exit(EXIT_FAILURE);
	}
	
	memcpy(userid, &buffer[5], 8);
	printf("userid %llx\n", *((uint64_t *)userid));

	sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);

	if(connect(sfd, rp->ai_addr, rp->ai_addrlen)){
		perror("connect()");
		exit(EXIT_FAILURE);
	}

	memcpy(&buffer[0], userid, 8);
	ltob(time(NULL), &buffer[8]);
	buffer[12] = 3;

	if(send(sfd, buffer, 13, 0) < 13){
		perror("send()");
		exit(EXIT_FAILURE);
	}

	nbytes = recv(sfd, buffer, 64, 0);
	if(nbytes < 0){
		perror("recv()");
		exit(EXIT_FAILURE);
	}
	if(nbytes == 0){
		fprintf(stderr, "server unexpectedly closed\n");
		exit(EXIT_FAILURE);
	}

	while(nbytes--){
		printf("%x ", *buffer++);
	}

	exit(EXIT_SUCCESS);
}
