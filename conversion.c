#include "game.h"
void lltob(unsigned long long n, uint8_t *b){
	b[0] = n;
	b[1] = n >> 8;
	b[2] = n >> 16;
	b[3] = n >> 24;
	b[4] = n >> 32;
	b[5] = n >> 40;
	b[6] = n >> 48;
	b[7] = n >> 56;
}
void ltob(unsigned long n, uint8_t *b){
	b[0] = n;
	b[1] = n >> 8;
	b[2] = n >> 16;
	b[3] = n >> 24;
}

void stob(unsigned short n, uint8_t *b){
	b[0] = n;
	b[1] = n >> 8;
}

unsigned long long btoll(uint8_t *b){
	unsigned long long r;
	unsigned i;
	r = 0;
	for(i = 0; i < 8; i++){
		r |= ((unsigned long long) b[i]) << (8 * i);
	}
	return r;
}

unsigned long btol(uint8_t *b){
	return (unsigned long)b[0] | (((unsigned long)b[1]) << 8) | (((unsigned long)b[2]) << 16) \
		| (((unsigned long)b[3]) << 24);
}

unsigned short btos(uint8_t *b){
	return (unsigned short)b[0] | (((unsigned short)b[1]) << 8);
}
