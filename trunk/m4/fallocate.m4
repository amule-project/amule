#							-*- Autoconf -*-
# This file is part of the aMule Project.
#
# Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

AC_DEFUN([MULE_CHECK_FALLOCATE],
[
	AC_MSG_CHECKING([for fallocate])
	MULE_BACKUP([CPPFLAGS])
	MULE_APPEND([CPPFLAGS], [$WX_CPPFLAGS])
	AC_LINK_IFELSE([
		AC_LANG_PROGRAM([[
			#define _GNU_SOURCE
			#ifdef HAVE_FCNTL_H
			#  include <fcntl.h>
			#endif
			#include <linux/falloc.h>
		]], [[
			fallocate(0, 0, 0, 0);
		]])
	], [
		AH_TEMPLATE([HAVE_FALLOCATE], [Define to 1 if you have the fallocate() function.])
		AC_DEFINE([HAVE_FALLOCATE])
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([no])
	])
	MULE_RESTORE([CPPFLAGS])


	AC_MSG_CHECKING([for SYS_fallocate])
	AC_LINK_IFELSE([
		AC_LANG_PROGRAM([[
			#include <sys/syscall.h>
			#include <sys/types.h>
			#include <unistd.h>
		]], [[
			syscall(SYS_fallocate, 0, (loff_t)0, (loff_t)0);
		]])
	], [
		AH_TEMPLATE([HAVE_SYS_FALLOCATE], [Define to 1 if you have the SYS_fallocate syscall number.])
		AC_DEFINE([HAVE_SYS_FALLOCATE])
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([no])
	])

	AC_MSG_CHECKING([for posix_fallocate])
	AC_LINK_IFELSE([
		AC_LANG_PROGRAM([[
			#define _XOPEN_SOURCE 600
			#include <stdlib.h>
			#ifdef HAVE_FCNTL_H
			#  include <fcntl.h>
			#endif
		]], [[
			posix_fallocate(0, 0, 0);
		]])
	], [
		AH_TEMPLATE([HAVE_POSIX_FALLOCATE], [Define to 1 if you have posix_fallocate() and it should be used.])
		AC_DEFINE([HAVE_POSIX_FALLOCATE])
		AC_MSG_RESULT([yes])
	], [
		AC_MSG_RESULT([no])
	])
])
