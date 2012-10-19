#pragma warning(disable:4996) 
#define ssize_t int
#include <stdio.h>
#include <io.h>
char PACKAGE_VERSION[] = "";
ssize_t pread(int fildes, void *buf, size_t nbyte, size_t offset)
{
	lseek(fildes, offset, SEEK_SET);
	return read(fildes, buf, nbyte);
}
#include "GeoIP.c" 
