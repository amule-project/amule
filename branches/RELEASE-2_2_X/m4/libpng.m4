#							-*- Autoconf -*-
# This file is part of the aMule Project.
#
# Copyright (c) 2003-2009 aMule Team ( admin@amule.org / http://www.amule.org )
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
dnl MULE_CHECK_LIBPNG([VERSION = 1.2.0], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl
dnl adds support for --with-libpng-prefix and --with-libpng-config
dnl command line options
dnl
dnl Test for libpng, and define LIBPNG_CFLAGS, LIBPNG_LIBS and LIBPNG_CONFIG_NAME
dnl environment variable to override the default name of the libpng-config script
dnl to use. Set LIBPNG_CONFIG_PATH to specify the full path to libpng-config -
dnl in this case the macro won't even waste time on tests for its existence.
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_LIBPNG],
[dnl
m4_define([REQUIRED_VERSION], [m4_ifval([$1], [$1], [1.2.0])])dnl
m4_define([REQUIRED_VERSION_MAJOR], [m4_bregexp(REQUIRED_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\1])])dnl
m4_define([REQUIRED_VERSION_MINOR], [m4_bregexp(REQUIRED_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\2])])dnl
m4_define([REQUIRED_VERSION_MICRO], [m4_bregexp(REQUIRED_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\3])])dnl

	AC_ARG_WITH([libpng-prefix], [AS_HELP_STRING([--with-libpng-prefix=PREFIX], [prefix where libpng is installed (optional)])])
	AC_ARG_WITH([libpng-config], [AS_HELP_STRING([--with-libpng-config=CONFIG], [libpng-config script to use (optional)])])

	LIBPNG_VERSION=

	# do we have libpng-config name: it can be libpng-config or libpng12-config or ...
	AS_IF([test x${LIBPNG_CONFIG_NAME+set} != xset], [LIBPNG_CONFIG_NAME=libpng-config])
	AS_IF([test -n "$with_libpng_config"], [LIBPNG_CONFIG_NAME="$with_libpng_config"])

	# deal with optional prefix
	AS_IF([test -n "$with_libpng_prefix"], [LIBPNG_LOOKUP_PATH="$with_libpng_prefix/bin"])

	# don't search the PATH if LIBPNG_CONFIG_NAME is absolute filename
	AS_IF([test -x "$LIBPNG_CONFIG_NAME"], [
		AC_MSG_CHECKING([for libpng-config])
		LIBPNG_CONFIG_PATH="$LIBPNG_CONFIG_NAME"
		AC_MSG_RESULT($LIBPNG_CONFIG_PATH)
	], [AC_PATH_PROG([LIBPNG_CONFIG_PATH], [$LIBPNG_CONFIG_NAME], [no], [$LIBPNG_LOOKUP_PATH:$PATH])])

	AS_IF([test ${LIBPNG_CONFIG_PATH:-no} != no],
	[
		AC_MSG_CHECKING([for libpng version >= REQUIRED_VERSION])

		LIBPNG_CONFIG_WITH_ARGS="$LIBPNG_CONFIG_PATH $libpng_config_args"

		LIBPNG_VERSION=`$LIBPNG_CONFIG_WITH_ARGS --version`
		libpng_config_major_version=`echo $LIBPNG_VERSION | sed ['s/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\1/']`
		libpng_config_minor_version=`echo $LIBPNG_VERSION | sed ['s/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\2/']`
		libpng_config_micro_version=`echo $LIBPNG_VERSION | sed ['s/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\3/']`

		libpng_ver_ok=
		MULE_IF([test $libpng_config_major_version -gt REQUIRED_VERSION_MAJOR], [libpng_ver_ok=yes],
			[test $libpng_config_major_version -eq REQUIRED_VERSION_MAJOR], [
				MULE_IF([test $libpng_config_minor_version -gt REQUIRED_VERSION_MINOR], [libpng_ver_ok=yes],
					[test $libpng_config_minor_version -eq REQUIRED_VERSION_MINOR],
						[MULE_IF([test $libpng_config_micro_version -ge REQUIRED_VERSION_MICRO], [libpng_ver_ok=yes])])
			])

		AS_IF([test -z "$libpng_ver_ok"], [
			AS_IF([test -z "$LIBPNG_VERSION"], [AC_MSG_RESULT([no])], [
				AC_MSG_RESULT([no (version $LIBPNG_VERSION is not new enough)])
				LIBPNG_VERSION=
			])
		], [
			LIBPNG_LIBS=`$LIBPNG_CONFIG_WITH_ARGS --libs`
			LIBPNG_LDFLAGS=`$LIBPNG_CONFIG_WITH_ARGS --ldflags | sed -e "s/ *${LIBPNG_LIBS}$//"`
			LIBPNG_CFLAGS=`$LIBPNG_CONFIG_WITH_ARGS --cflags`
			AC_MSG_RESULT([yes (version $LIBPNG_VERSION)])
		])
	], [
		# Try hard to find a usable libpng
		# Some RedHat RPMs miss libpng-config, so test for
		# the usability with default options.
		AC_MSG_CHECKING([for libpng >= REQUIRED_VERSION])
		MULE_BACKUP([LIBS])
		MULE_PREPEND([LIBS], [-lpng -lz -lm])
		AC_RUN_IFELSE([
			AC_LANG_PROGRAM([[
				#include <png.h>
				#include <stdio.h>

				/* Check linking to png library */
				void dummy() {
					png_check_sig(NULL, 0);
				}
			]], [dnl Don't use double-quoting here!
				/* png.h defines PNG_LIBPNG_VER=xyyzz */
				FILE *f=fopen("conftestval", "w");
				if (!f) exit(1);
				fprintf(f, "%s", (PNG_LIBPNG_VER >= REQUIRED_VERSION_MAJOR * 10000 + REQUIRED_VERSION_MINOR * 100 + REQUIRED_VERSION_MICRO) ? "yes" : "no");
				fclose(f);
				f=fopen("conftestver", "w");
				if (!f) exit(0);
				fprintf(f, "%s", PNG_LIBPNG_VER_STRING);
				fclose(f);
				exit(0);
			])
		], [
			AS_IF([test -f conftestval], [result=`cat conftestval`], [result=no])
			AS_IF([test ${result:-no} = yes], [
				AS_IF([test -f conftestver], [
					LIBPNG_VERSION=`cat conftestver`
					lib_version=" (version $LIBPNG_VERSION)"
				], [
					lib_version=
					LIBPNG_VERSION="detected"
				])
			])
			AC_MSG_RESULT([$result$lib_version])
			LIBPNG_LIBS="-lpng -lz -lm"
		], [
			result=no
			AC_MSG_RESULT([$result])
		], [
			AC_MSG_RESULT([cross-compilation detected, checking only the header])
			AC_CHECK_HEADER([png.h], [
				LIBPNG_VERSION="detected"
				LIBPNG_LIBS="-lpng -lz -lm"
			])
		])
		MULE_RESTORE([LIBS])
	])

	AS_IF([test -n "$LIBPNG_VERSION"], [$2], [$3])

AC_SUBST([LIBPNG_CFLAGS])dnl
AC_SUBST([LIBPNG_LDFLAGS])dnl
AC_SUBST([LIBPNG_LIBS])dnl

m4_undefine([REQUIRED_VERSION])dnl
m4_undefine([REQUIRED_VERSION_MAJOR])dnl
m4_undefine([REQUIRED_VERSION_MINOR])dnl
m4_undefine([REQUIRED_VERSION_MICRO])dnl
])
