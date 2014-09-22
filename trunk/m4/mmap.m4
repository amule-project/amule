#							-*- Autoconf -*-
# This file is part of the aMule Project.
#
# Copyright (c) 2014 aMule Team ( admin@amule.org / http://www.amule.org )
#
# Any parts of this program derived from the xMule, lMule or eMule project,
# or contributed by third-party developers are copyrighted by their
# respective authors.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
#


dnl ---------------------------------------------------------------------------
dnl MULE_FUNC_MMAP
dnl
dnl This function is copied over from autoconf sources, but fixed to work with
dnl C++.
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_FUNC_MMAP],
[AC_CHECK_HEADERS_ONCE([stdlib.h unistd.h sys/param.h])
AC_CHECK_FUNCS([getpagesize])
AC_CACHE_CHECK([for working mmap], [ac_cv_func_mmap_fixed_mapped],
[AC_RUN_IFELSE([AC_LANG_SOURCE([AC_INCLUDES_DEFAULT]
[[/* malloc might have been renamed as rpl_malloc. */
#undef malloc

/* Thanks to Mike Haertel and Jim Avera for this test.
   Here is a matrix of mmap possibilities:
	mmap private not fixed
	mmap private fixed at somewhere currently unmapped
	mmap private fixed at somewhere already mapped
	mmap shared not fixed
	mmap shared fixed at somewhere currently unmapped
	mmap shared fixed at somewhere already mapped
   For private mappings, we should verify that changes cannot be read()
   back from the file, nor mmap's back from the file at a different
   address.  (There have been systems where private was not correctly
   implemented like the infamous i386 svr4.0, and systems where the
   VM page cache was not coherent with the file system buffer cache
   like early versions of FreeBSD and possibly contemporary NetBSD.)
   For shared mappings, we should conversely verify that changes get
   propagated back to all the places they're supposed to be.

   Grep wants private fixed already mapped.
   The main things grep needs to know about mmap are:
   * does it exist and is it safe to write into the mmap'd area
   * how to use it (BSD variants)  */

#include <fcntl.h>
#include <sys/mman.h>

#if !defined STDC_HEADERS && !defined HAVE_STDLIB_H
char *malloc ();
#endif

/* This mess was copied from the GNU getpagesize.h.  */
#ifndef HAVE_GETPAGESIZE
# ifdef _SC_PAGESIZE
#  define getpagesize() sysconf(_SC_PAGESIZE)
# else /* no _SC_PAGESIZE */
#  ifdef HAVE_SYS_PARAM_H
#   include <sys/param.h>
#   ifdef EXEC_PAGESIZE
#    define getpagesize() EXEC_PAGESIZE
#   else /* no EXEC_PAGESIZE */
#    ifdef NBPG
#     define getpagesize() NBPG * CLSIZE
#     ifndef CLSIZE
#      define CLSIZE 1
#     endif /* no CLSIZE */
#    else /* no NBPG */
#     ifdef NBPC
#      define getpagesize() NBPC
#     else /* no NBPC */
#      ifdef PAGESIZE
#       define getpagesize() PAGESIZE
#      endif /* PAGESIZE */
#     endif /* no NBPC */
#    endif /* no NBPG */
#   endif /* no EXEC_PAGESIZE */
#  else /* no HAVE_SYS_PARAM_H */
#   define getpagesize() 8192	/* punt totally */
#  endif /* no HAVE_SYS_PARAM_H */
# endif /* no _SC_PAGESIZE */

#endif /* no HAVE_GETPAGESIZE */

int
main ()
{
  char *data, *data2, *data3;
  int i, pagesize;
  int fd, fd2;

  pagesize = getpagesize ();

  /* First, make a file with some known garbage in it. */
  data = (char *) malloc (pagesize);
  if (!data)
    return 1;
  for (i = 0; i < pagesize; ++i)
    *(data + i) = rand ();
  umask (0);
  fd = creat ("conftest.mmap", 0600);
  if (fd < 0)
    return 2;
  if (write (fd, data, pagesize) != pagesize)
    return 3;
  close (fd);

  /* Next, check that the tail of a page is zero-filled.  File must have
     non-zero length, otherwise we risk SIGBUS for entire page.  */
  fd2 = open ("conftest.txt", O_RDWR | O_CREAT | O_TRUNC, 0600);
  if (fd2 < 0)
    return 4;
  data2 = (char *) "";
  if (write (fd2, data2, 1) != 1)
    return 5;
  data2 = (char *) mmap (0, pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, fd2, 0L);
  if (data2 == MAP_FAILED)
    return 6;
  for (i = 0; i < pagesize; ++i)
    if (*(data2 + i))
      return 7;
  close (fd2);
  if (munmap (data2, pagesize))
    return 8;

  /* Next, try to mmap the file at a fixed address which already has
     something else allocated at it.  If we can, also make sure that
     we see the same garbage.  */
  fd = open ("conftest.mmap", O_RDWR);
  if (fd < 0)
    return 9;
  if (data2 != mmap (data2, pagesize, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_FIXED, fd, 0L))
    return 10;
  for (i = 0; i < pagesize; ++i)
    if (*(data + i) != *(data2 + i))
      return 11;

  /* Finally, make sure that changes to the mapped area do not
     percolate back to the file as seen by read().  (This is a bug on
     some variants of i386 svr4.0.)  */
  for (i = 0; i < pagesize; ++i)
    *(data2 + i) = *(data2 + i) + 1;
  data3 = (char *) malloc (pagesize);
  if (!data3)
    return 12;
  if (read (fd, data3, pagesize) != pagesize)
    return 13;
  for (i = 0; i < pagesize; ++i)
    if (*(data + i) != *(data3 + i))
      return 14;
  close (fd);
  return 0;
}]])],
	       [ac_cv_func_mmap_fixed_mapped=yes],
	       [ac_cv_func_mmap_fixed_mapped=no],
	       [ac_cv_func_mmap_fixed_mapped=no])])
if test $ac_cv_func_mmap_fixed_mapped = yes; then
  AC_DEFINE([HAVE_MMAP], [1],
	    [Define to 1 if you have a working `mmap' system call.])
fi
rm -f conftest.mmap conftest.txt
])


dnl ---------------------------------------------------------------------------
dnl MULE_CHECK_MMAP
dnl
dnl Checks for mmap() and makes use of it when found.
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_MMAP],
[
	MULE_ARG_ENABLE([mmap], [no], [enable using mapped memory if supported])

	AH_TEMPLATE([ENABLE_MMAP], [Define this variable to 1 if using mapped memory was requested. Note that defining it will alone not allow usage of mmap(), but unsetting it will completely disable its usage.])

	MULE_IF_ENABLED([mmap], [
		AC_DEFINE([ENABLE_MMAP], [1])
		MULE_FUNC_MMAP
		AC_CHECK_FUNCS([munmap sysconf])
		AS_IF([test $ac_cv_func_sysconf = yes], [
			AC_MSG_CHECKING([for pagesize constant for sysconf])
			AC_LINK_IFELSE([
				AC_LANG_PROGRAM([[
					#include <unistd.h>
				]], [[
					return sysconf(_SC_PAGESIZE);
				]])
			], [
				AC_MSG_RESULT([_SC_PAGESIZE])
				AC_DEFINE([HAVE__SC_PAGESIZE], [1], [Define to 1 if you have the _SC_PAGESIZE constant in <unistd.h>])
			], [
				AC_LINK_IFELSE([
					AC_LANG_PROGRAM([[
						#include <unistd.h>
					]], [[
						return sysconf(_SC_PAGE_SIZE);
					]])
				], [
					AC_MSG_RESULT([_SC_PAGE_SIZE])
					AC_DEFINE([HAVE__SC_PAGE_SIZE], [1], [Define to 1 if you have the _SC_PAGE_SIZE constant in <unistd.h>, but not _SC_PAGESIZE])
				], [
					AC_MSG_RESULT([none])
				])
			])
		])
	])
])
