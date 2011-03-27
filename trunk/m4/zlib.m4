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

dnl ----------------------------------------------------
dnl MULE_CHECK_ZLIB([MIN_ZLIB_VERSION = 1.1.4], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl
dnl check if zlib is on the system and is at least MIN_ZLIB_VERSION
dnl
dnl Add support for --with-zlib command-line parameter.
dnl PREFIX may be a directory prefix where zlib is installed, e.g. /usr/local
dnl or may be one of the following special keywords:
dnl    sys - use system zlib
dnl
dnl Will set the output variables:
dnl	 ZLIB_CPPFLAGS
dnl	 ZLIB_LDFLAGS
dnl	 ZLIB_LIBS
dnl ----------------------------------------------------
AC_DEFUN([MULE_CHECK_ZLIB],
[dnl
m4_define([MIN_ZLIB_VERSION], [m4_ifval([$1], [$1], [1.1.4])])dnl
m4_define([zver_max], [m4_bregexp(MIN_ZLIB_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\1])])dnl
m4_define([zver_mid], [m4_bregexp(MIN_ZLIB_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\2])])dnl
m4_define([zver_min], [m4_bregexp(MIN_ZLIB_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\3])])dnl

	AC_ARG_WITH([zlib], AS_HELP_STRING([--with-zlib=PREFIX], [use zlib in PREFIX]))

	case "${with_zlib:-yes}" in
	no)
		$3
		;;
	yes | sys)
		;;
	*)
		zlib_prefix="$with_zlib"
	esac

	MULE_BACKUP([CPPFLAGS])
	MULE_BACKUP([LDFLAGS])
	MULE_BACKUP([LIBS])
	AS_IF([test -n "$zlib_prefix"],
	[
		ZLIB_CPPFLAGS="-I$zlib_prefix/include"
		ZLIB_LDFLAGS="-L$zlib_prefix/lib"
		MULE_APPEND([CPPFLAGS], [$ZLIB_CPPFLAGS])
		MULE_APPEND([LDFLAGS], [$ZLIB_LDFLAGS])
	], [
		ZLIB_CPPFLAGS=
		ZLIB_LDFLAGS=
	])
	ZLIB_LIBS="-lz"
	MULE_PREPEND([LIBS], [$ZLIB_LIBS])

	AC_MSG_CHECKING([for zlib >= $1])
	AC_RUN_IFELSE([
		AC_LANG_PROGRAM([[
			#include <zlib.h>
			#include <stdio.h>
		]], [dnl Do not use double-quoting here!
			char *zver = zlibVersion();
			FILE *f=fopen("conftestval", "w");
			if (!f) return 1;
			fprintf(f, "%s",
				zver[[0]] > 'zver_max' ||
				(zver[[0]] == 'zver_max' &&
				(zver[[2]] > 'zver_mid' ||
				(zver[[2]] == 'zver_mid' &&
				zver[[4]] >= 'zver_min'))) ? "yes" : "no");
			fclose(f);
			f=fopen("conftestver", "w");
			if (f) {
				fprintf(f, "%s", ZLIB_VERSION);
				fclose(f);
			}
		])
	], [
		AS_IF([test -f conftestval], [result=`cat conftestval`], [result=no])
		AS_IF([test ${result:-no} = yes],
		[
			AS_IF([test -f conftestver],
			[
				ZLIB_VERSION=`cat conftestver`
				z_version=" (version $ZLIB_VERSION)"
			], [z_version=])
		])
		AC_MSG_RESULT([$result$z_version])
	], [
		result=no
		AC_MSG_RESULT([$result])
	], [
		result=no
		z_version=
		AC_LINK_IFELSE([
			AC_LANG_PROGRAM([[
				#include <zlib.h>
				#include <stdio.h>
			]], [[
				printf("\nZLIB_VERSION_START" ZLIB_VERSION "ZLIB_VERSION_END\n\n");
				zlibVersion();
			]])
		], [
			ZLIB_VERSION=`grep -a '^ZLIB_VERSION_START.*ZLIB_VERSION_END$' conftest$ac_exeext | sed 's/^ZLIB_VERSION_START\(.*\)ZLIB_VERSION_END$/\1/'`
			cross_zver_max="`echo $ZLIB_VERSION | cut -d. -f1`"
			cross_zver_mid="`echo $ZLIB_VERSION | cut -d. -f2`"
			cross_zver_min="`echo $ZLIB_VERSION | cut -d. -f3`"
			MULE_IF([test "$cross_zver_max" -gt "zver_max"], [result=yes],
				[test "$cross_zver_max" -eq "zver_max"], [
					MULE_IF([test "$cross_zver_mid" -gt "zver_mid"], [result=yes],
						[test "$cross_zver_mid" -eq "zver_mid"],
							[MULE_IF([test "$cross_zver_min" -ge "zver_min"], [result=yes])])
				])
			AS_IF([test ${result:-no} = yes], [z_version=" (version $ZLIB_VERSION)"])
		])
		AC_MSG_RESULT([$result$z_version])
	])

	MULE_RESTORE([CPPFLAGS])
	MULE_RESTORE([LDFLAGS])
	MULE_RESTORE([LIBS])

	AS_IF([test ${result:-no} = no], [
		ZLIB_CPPFLAGS=
		ZLIB_LDFLAGS=
		ZLIB_LIBS=
		$3
	], [$2])

AC_SUBST([ZLIB_CPPFLAGS])dnl
AC_SUBST([ZLIB_LDFLAGS])dnl
AC_SUBST([ZLIB_LIBS])dnl
m4_undefine([zver_max])dnl
m4_undefine([zver_mid])dnl
m4_undefine([zver_min])dnl
m4_undefine([MIN_ZLIB_VERSION])dnl
])
