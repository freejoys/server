/*
 * proto-encrypt.h
 *
 *  Created on: 2013Äê8ÔÂ1ÈÕ
 *      Author: Administrator
 */

#ifndef PROTO_ENCRYPT_H_
#define PROTO_ENCRYPT_H_

#include <stdlib.h>

#define B_SIZE 255
#define KEY_SIZE 64

void proto_encrypt(char *msg, int sz, unsigned char *box);

void random_key(char *key, size_t key_len);

void init_box(unsigned char *box,const char *key, size_t key_len);

#endif /* PROTO_ENCRYPT_H_ */
