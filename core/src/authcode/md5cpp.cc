
#include "md5cpp.h"
//#include "Unicode.h"
#include <iomanip>
#include <sstream>
#include <fcntl.h>

#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

//#ifdef _FEATURES_H
//#error Please make sure this file included before features.h
//#endif
#define _FILE_OFFSET_BITS 64

#include <sys/stat.h>
#include <unistd.h>


#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_RANDOM
#define O_RANDOM 0
#endif

#define fstat64(f, s) fstat(f, s)
typedef struct stat Stat64;
//using namespace std;

#undef min

std::ostream& operator<<(std::ostream& stream, const MD5& md5)
{
	for(unsigned int i=0;i<sizeof(md5);++i)
	{
		stream<<std::setfill('0')<<std::setw(2)<<std::setiosflags(std::ios::uppercase)<<std::hex<<(unsigned short)md5[i]<<std::dec;
	}
	return stream;
}

std::istream& operator>>(std::istream& stream, MD5& md5)
{
	std::string str;
	stream>>str;
	str.resize(sizeof(MD5)*2);
	String2MD5(&md5, str);
	return stream;
}

void MD52String(std::string* out, const MD5& in)
{
	std::ostringstream stream;
	stream<<in;
	*out = stream.str();
}


int hex_char_value(char c)   
{   
	if(c >= '0' && c <= '9')
		return c - '0';
	else if(c >= 'a' && c <= 'f')   
		return (c - 'a' + 10);
	else if(c >= 'A' && c <= 'F')   
		return (c - 'A' + 10);
	else
		return -1;   
}   

void String2MD5(MD5* out, const std::string& in)
{
	if ( in.length() > sizeof(MD5)*2 ) return;
	std::istringstream stream;
	stream.str(in);

	
	int index = 0;

	while( index < 16)
	{
		char c;
		stream>>c;
		unsigned char v = hex_char_value(c) * 16;
		stream>>c;
		v += hex_char_value(c);
		(*out)[index++] = v;
	}
}

size_t MD5_stream(MD5* out, std::istringstream& file, unsigned long start, unsigned long length)
{
	memset(*out, 0, sizeof(MD5));
	if ( length == 0) return 0;
	MD5_CTX ctx;
	MD5Init(&ctx);
	unsigned long total = 0;
	unsigned long size = 1024;
	file.seekg(start, std::ios::beg);
	while( size == 1024 )
	{
		char buffer[1024];
		file.read(buffer, std::min((unsigned long)1024, length - total));
		size = file.gcount();
		total += size;
		MD5Update(&ctx, (unsigned char*)buffer, (unsigned int)size);
	}
	MD5Final(*out, &ctx);
	file.clear();

	return total;
}

size_t MD5_buffer(MD5* out, const char* buffer, unsigned long length)
{
	memset(*out, 0, sizeof(MD5));
	if ( length == 0) return 0;
	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, (unsigned char*)buffer, (unsigned int)length);
	MD5Final(*out, &ctx);

	return length;
}

unsigned long long MD5_file64(MD5* out, int file, unsigned long long start, unsigned long long length)
{
	memset(*out, 0, sizeof(MD5));
	if ( length == 0) return 0;
	MD5_CTX ctx;
	MD5Init(&ctx);
	unsigned long long total = 0;
	unsigned long long size = 1024;

	if ( -1 == lseek64(file, start, SEEK_SET) )
	{
		return 0;
	}

	while( size == 1024 )
	{
		char buffer[1024];
		size = read(file, buffer, (unsigned long)std::min((unsigned long long)1024, length - total));
		total += size;
		MD5Update(&ctx, (unsigned char*)buffer, (unsigned int)size);
	}
	MD5Final(*out, &ctx);

	return total;
}
unsigned long long MD5_file64(MD5* out, const char* filename, unsigned long long start, unsigned long long length)
{
	int file = open(filename, O_BINARY | O_RDONLY | O_RANDOM);
	if ( file == -1 )
	{
		return 0;
	}
	unsigned long long size =  MD5_file64(out, file, start, length);
	close(file);
	return size;
}

