# This file is part of the aMule project.                      -*- Autoconf -*-
#
AC_DEFUN([CHECK_FALLOCATE],
[
	AC_MSG_CHECKING([for fallocate])
	AC_LINK_IFELSE([
		AC_LANG_PROGRAM([[
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


	AC_MSG_CHECKING([for SYS_fallocate])
	AC_LINK_IFELSE([
		AC_LANG_PROGRAM([[
			#include <sys/sycall.h>
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
