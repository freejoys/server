/*
 * com_encrypt.cpp
 *
 *  Created on: 2013Äê7ÔÂ24ÈÕ
 *      Author: Administrator
 */
#include "com_encrypt.h"
#include <string>
#include <algorithm>
#include <stdio.h>
#include <sys/time.h>
#include "base64.h"
#include "md5cpp.h"


#define C_SIZE 256

int com_md5(const std::string &source_str, std::string &target_str);

void com_microtime(std::string &time_str)
{
	time_t micro_time;
	time_t second_time;

	timeval tv;
	gettimeofday(&tv, NULL);
	micro_time = tv.tv_usec;
	second_time = tv.tv_sec;

	char s[64];
	sprintf(s, "0.%010ld %010ld", micro_time, second_time);
	time_str.append(s);
	return;
}

/*
	* @param1 string source ÊäÈëµÄÔ­ÎÄ»òÕßÃÜÎÄ
	* @param2 string $key ÃÜÔ¿
	* @param3 bool $is_encode ²Ù×÷(true is encode | false is DECODE), Ä¬ÈÏÎª true
	* @param4 int $expiry ÃÜÎÄÓÐÐ§ÆÚ, ¼ÓÃÜÊ±ºòÓÐÐ§£¬ µ¥Î» Ãë£¬0 ÎªÓÀ¾ÃÓÐÐ§
	*/
int
_com_encrypt(lua_State *L)
{
	// Ëæ»úÃÜÔ¿³¤¶È È¡Öµ 0-32;
	// ¼ÓÈëËæ»úÃÜÔ¿£¬¿ÉÒÔÁîÃÜÎÄÎÞÈÎºÎ¹æÂÉ£¬¼´±ãÊÇÔ­ÎÄºÍÃÜÔ¿ÍêÈ«ÏàÍ¬£¬¼ÓÃÜ½á¹ûÒ²»áÃ¿´Î²»Í¬£¬Ôö´óÆÆ½âÄÑ¶È¡£
	// µ±´ËÖµÎª 0 Ê±£¬Ôò²»²úÉúËæ»úÃÜÔ¿
	static unsigned int ckey_length = 4;

	const char *source = lua_tostring(L,1);
	const char *key = lua_tostring(L,2);
	int is_encode = lua_toboolean(L,3);
	int expiry = lua_tonumber(L,4);
	std::string str_s(source);
	std::string key_s(key);
	std::string keya, keyb, keyc;

	//com_md5(key_s == "" ? "key" : key_s, key_s);
	key_s.size() == 0 ? ((keya = "d41d8cd98f00b204e9800998ecf8427e") != "") : com_md5(key_s.substr(0, 16), keya);
	key_s.size() <= 16 ? ((keyb = "d41d8cd98f00b204e9800998ecf8427e") != "") : com_md5(key_s.substr(16, 16), keyb);

	std::string time_key;
	if (ckey_length != 0 && is_encode)
	{
		com_microtime(time_key);
		com_md5(time_key, time_key);
		time_key = time_key.substr(time_key.size() - ckey_length);
	}

	keyc = ckey_length != 0 ? (!is_encode ? str_s.substr(0, ckey_length) : time_key) : "";

	std::string keyac_md5;
	com_md5(keya + keyc, keyac_md5);
	std::string cryptkey = keya + keyac_md5;
	int key_length = cryptkey.size();

	static unsigned int md5_head_len = 16;
	static unsigned int time_head_len = 10;
	static unsigned int head_len = time_head_len + md5_head_len;

	std::string form_source_str;
	if (is_encode)
	{
		//»ñÈ¡time head ³¤¶ÈÎª10
		char t[32];
		sprintf(t, "%010d", (int)(expiry != 0 ? (expiry + time(0)) : 0));
		form_source_str.append(t);

		//»ñÈ¡ÓÉ£¨Ô´´®+keyb£©µÄÇ°16Î»µÄmd5Âë
		std::string str_keyb_md5;
		com_md5(str_s + keyb,str_keyb_md5);

		form_source_str.append(str_keyb_md5.substr(0, md5_head_len));
		form_source_str.append(str_s);
	}
	else
	{
		//½âÂëµÄ´®³¤¶È²»ÄÜÐ¡ÓÚckey_length
		if(str_s.size() < ckey_length)
			return 0;//com_encrypt decode error!
	}
	
	form_source_str = !is_encode ? base64_decode(str_s.substr(ckey_length)) : form_source_str;
	int string_length = form_source_str.size();

	static unsigned char s_box[C_SIZE] = {0};
	unsigned char box[C_SIZE] = {0};
	if (s_box[C_SIZE - 1] == 0)
	{
		for (int i = 0; i < C_SIZE; ++i)
		{
			s_box[i] = i;
		}
	}
	memcpy(box,s_box,C_SIZE);

	unsigned char rndkey[C_SIZE] = {0};
	for (int i = 0; i < C_SIZE; ++i)
	{
		rndkey[i] = cryptkey[i % key_length];
	}

	for(int i = 0,j = 0; i < C_SIZE; i++)
	{
		j = (j + box[i] + rndkey[i]) % C_SIZE;
		unsigned char tmp = box[i];
		box[i] = box[j];
		box[j] = tmp;
	}

	std::string xor_str;
	for(int a = 0,j = 0, i = 0; i < string_length; i++)
	{
		a = (a + 1) % C_SIZE;
		j = (j + box[a]) % C_SIZE;
		unsigned char tmp = box[a];
		box[j] = tmp;
		xor_str += (char )(form_source_str[i] ^ (box[(box[a] + box[j]) % C_SIZE]));
	}

	std::string target_str;
	if(!is_encode)
	{
		if (xor_str.size() < head_len)
		{
			return 0;//com_encrypt decode error!
		}

		target_str = xor_str.substr(head_len);

		std::string check_md5_str;
		com_md5(target_str + keyb,check_md5_str);
		long long head_time = atol(xor_str.substr(0, time_head_len).c_str());

		if((head_time != 0 && head_time - time(0) <= 0)
			|| xor_str.substr(time_head_len, md5_head_len) != check_md5_str.substr(0, md5_head_len))
		{
			return 0;//com_encrypt decode error!
		}
	}
	else
	{
		target_str = keyc + base64_encode((const unsigned char *)xor_str.c_str(), xor_str.size());
		//È¥³ýbase64ºóÃæµÄ²¹È«·ûºÅ"=",×î¶à2¸ö
		for(int i = 0; i < 2 && target_str.size() != 0 && target_str.at(target_str.size() - 1) == '='; ++i)
		{
			target_str.erase(target_str.size() - 1);
		}
	}
	lua_pushstring(L,target_str.c_str());
	return 1;
}

int
_base64decode(lua_State *L) {
	size_t sz;
	const char * str = lua_tolstring(L,1,&sz);
	std::string str_s(str);
	std::string target = base64_decode(str_s);
	lua_pushlstring(L,target.c_str(),target.length());
	return 1;
}

int
_base64encode(lua_State *L) {
	size_t sz;
	const char * str = lua_tolstring(L,1,&sz);
	std::string src = base64_encode((const unsigned char*)str,sz);
	lua_pushlstring(L,src.c_str(),src.length());
	return 1;
}

int com_md5(const std::string &source_str, std::string &target_str)
{
	MD5 md5;
	MD5_buffer(&md5, source_str.c_str(), (unsigned long)source_str.size());
	MD52String(&target_str, md5);
	transform(target_str.begin(),target_str.end(),target_str.begin(),tolower);
	return 0;
}
