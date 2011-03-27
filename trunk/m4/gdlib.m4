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

dnl ---------------------------------------------------------------------------
dnl MULE_CHECK_GDLIB([VERSION = 2.0.0], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl
dnl adds support for --with-gdlib-prefix and --with-gdlib-config
dnl command line options
dnl
dnl Test for gdlib, and define GDLIB_CFLAGS, GDLIB_LIBS and GDLIB_CONFIG_NAME
dnl environment variable to override the default name of the gdlib-config script
dnl to use. Set GDLIB_CONFIG_PATH to specify the full path to gdlib-config -
dnl in this case the macro won't even waste time on tests for its existence.
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_GDLIB],
[dnl
m4_define([REQUIRED_VERSION], [m4_ifval([$1], [$1], [2.0.0])])dnl
m4_define([REQUIRED_VERSION_MAJOR], [m4_bregexp(REQUIRED_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\1])])dnl
m4_define([REQUIRED_VERSION_MINOR], [m4_bregexp(REQUIRED_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\2])])dnl
m4_define([REQUIRED_VERSION_MICRO], [m4_bregexp(REQUIRED_VERSION, [\([0-9]+\)\.\([0-9]+\)\.\([0-9]+\)], [\3])])dnl

	AC_ARG_WITH([gdlib-prefix], [AS_HELP_STRING([--with-gdlib-prefix=PREFIX], [prefix where gdlib is installed (optional)])])
	AC_ARG_WITH([gdlib-config], [AS_HELP_STRING([--with-gdlib-config=CONFIG], [gdlib-config script to use (optional)])])

	GDLIB_VERSION=

	# do we have gdlib-config name: it can be gdlib-config or gd-config or ...
	AS_IF([test x${GDLIB_CONFIG_NAME+set} != xset], [GDLIB_CONFIG_NAME=gdlib-config])
	AS_IF([test -n "$with_gdlib_config"], [GDLIB_CONFIG_NAME="$with_gdlib_config"])

	# deal with optional prefix
	AS_IF([test -n "$with_gdlib_prefix"], [GDLIB_LOOKUP_PATH="$with_gdlib_prefix/bin"])

	# don't search the PATH if GDLIB_CONFIG_NAME is absolute filename
	AS_IF([test -x "$GDLIB_CONFIG_NAME"], [
		AC_MSG_CHECKING([for gdlib-config])
		GDLIB_CONFIG_PATH="$GDLIB_CONFIG_NAME"
		AC_MSG_RESULT([$GDLIB_CONFIG_PATH])
	], [AC_PATH_PROG([GDLIB_CONFIG_PATH], [$GDLIB_CONFIG_NAME], [no], [$GDLIB_LOOKUP_PATH:$PATH])])

	AS_IF([test ${GDLIB_CONFIG_PATH:-no} != no],
	[
		AC_MSG_CHECKING([for gdlib version >= REQUIRED_VERSION])
		GDLIB_CONFIG_WITH_ARGS="$GDLIB_CONFIG_PATH $gdlib_config_args"

		GDLIB_VERSION=`$GDLIB_CONFIG_WITH_ARGS --version`
		gdlib_config_major_version=`echo $GDLIB_VERSION | sed ['s/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\1/']`
		gdlib_config_minor_version=`echo $GDLIB_VERSION | sed ['s/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\2/']`
		gdlib_config_micro_version=`echo $GDLIB_VERSION | sed ['s/\([0-9]*\)\.\([0-9]*\)\.\([0-9]*\).*/\3/']`

		gdlib_ver_ok=
		MULE_IF([test $gdlib_config_major_version -gt REQUIRED_VERSION_MAJOR], [gdlib_ver_ok=yes],
			[test $gdlib_config_major_version -eq REQUIRED_VERSION_MAJOR], [
				MULE_IF([test $gdlib_config_minor_version -gt REQUIRED_VERSION_MINOR], [gdlib_ver_ok=yes],
					[test $gdlib_config_minor_version -eq REQUIRED_VERSION_MINOR],
						[MULE_IF([test $gdlib_config_micro_version -ge REQUIRED_VERSION_MICRO], [gdlib_ver_ok=yes])])
			])

		AS_IF([test -z "$gdlib_ver_ok"], [
			AS_IF([test -z "$GDLIB_VERSION"], [AC_MSG_RESULT([no])], [
				AC_MSG_RESULT([no (version $GDLIB_VERSION is not new enough)])
				GDLIB_VERSION=
			])
		], [
			AC_MSG_RESULT([yes (version $GDLIB_VERSION)])
			GDLIB_CFLAGS="`$GDLIB_CONFIG_WITH_ARGS --cflags`"
			GDLIB_LDFLAGS="`$GDLIB_CONFIG_WITH_ARGS --ldflags`"
			GDLIB_LIBS="`$GDLIB_CONFIG_WITH_ARGS --libs`"
			MULE_BACKUP([CFLAGS])
			MULE_APPEND([CFLAGS], [$GDLIB_CFLAGS])
			AC_CHECK_HEADER([gd.h],, [GDLIB_VERSION=])
			MULE_RESTORE([CFLAGS])
		])
	])

	AS_IF([test -n "$GDLIB_VERSION"], [$2], [$3])

AC_SUBST([GDLIB_CFLAGS])dnl
AC_SUBST([GDLIB_LDFLAGS])dnl
AC_SUBST([GDLIB_LIBS])dnl

m4_undefine([REQUIRED_VERSION])dnl
m4_undefine([REQUIRED_VERSION_MAJOR])dnl
m4_undefine([REQUIRED_VERSION_MINOR])dnl
m4_undefine([REQUIRED_VERSION_MICRO])dnl
])
