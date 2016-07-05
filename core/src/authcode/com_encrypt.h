/*
 * com_encrypt.h
 *
 *  Created on: 2013年7月24日
 *      Author: Administrator
 */

#ifndef COM_ENCRYPT_H_
#define COM_ENCRYPT_H_

#ifdef __cplusplus
extern "C"
{
#endif
#include <lua.h>
#include <lauxlib.h>
	/**
	* @param1 string source 输入的原文或者密文
	* @param2 string $is_encode 操作(true is encode | false is DECODE), 默认为 true
	* @param3 string $key 密钥
	* @param4 int $expiry 密文有效期, 加密时候有效， 单位 秒，0 为永久有效
	* @return 字符串,返回到lua,解密的原文或者 加密的密文
	*/
int _com_encrypt(lua_State *L);
int _base64decode(lua_State *L);
int _base64encode(lua_State *L);

#ifdef __cplusplus
};
#endif

#endif /* COM_ENCRYPT_H_ */
