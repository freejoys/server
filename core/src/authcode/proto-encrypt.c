/*
 * proto-encrypt.c
 *
 *  Created on: 2013Äê8ÔÂ1ÈÕ
 *      Author: Administrator
 */
#include "proto-encrypt.h"
#include <time.h>
void
proto_encrypt(char *msg, int sz, unsigned char *box){
	int i;
	for(i = 0; i < sz; i++)
	{
		msg[i] = (char )(msg[i] ^ box[i % B_SIZE]);
	}
}

void
random_key(char *key, size_t key_len){
	static const char chars[63] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	srand(time(0));
	int i;
	for(i = 0; i < key_len; ++i)
	{
		key[i] = chars[rand() % 62];
	}
}

void
init_box(unsigned char *box,const char *key, size_t key_len){
	int i;
	for (i = 0; i < B_SIZE; ++i)
	{
		box[i] = i;
	}

	unsigned char rndkey[B_SIZE] = {0};
	for (i = 0; i < B_SIZE; ++i)
	{
		rndkey[i] = key[i % key_len];
	}
	int j;
	for(i = 0,j = 0; i < B_SIZE; i++)
	{
		j = (j + box[i] + rndkey[i]) % B_SIZE;
		unsigned char tmp = box[i];
		box[i] = box[j];
		box[j] = tmp;
	}
}
