#define _XOPEN_SOURCE 600
#include <stdlib.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

int main ()
{
	posix_fallocate(0, 0, 0);
	;
	return 0;
}
