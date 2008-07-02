#
# This file is part of the libupnp library project.
#
# Copyright (c) 2008 Marcelo Roberto Jimenez (mroberto@users.sf.net)
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

dnl --------------------------------------------------------------------------
dnl LIBUPNP_CHECK([VERSION = 1.6.6], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl
dnl Check for the libupnp library
dnl --------------------------------------------------------------------------
dnl
dnl This macro sets these variables:
dnl - LIBUPNP_PREFIX
dnl 	This is the user or system directory where libupnp is installed.
dnl - LIBUPNP_VERSION_STRING
dnl 	Something like "1.6.7"
dnl - LIBUPNP_VERSION_NUMBER
dnl 	Something like 1006007
dnl - LIBUPNP_STYLE
dnl 	"prefix" or "system"
dnl - LIBUPNP_CPPFLAGS
dnl 	Flags to be added to CPPFLAGS
dnl - LIBUPNP_CFLAGS
dnl 	Flags to be added to CFLAGS
dnl - LIBUPNP_LDFLAGS
dnl 	Flags to be added to LDFLAGS
dnl - LIBUPNP_LDADD
dnl	Library to be added to LDADD
dnl
dnl The LIBUPNP_CPPFLAGS, LIBUPNP_LDFLAGS and LIBUPNP_LIBS variables are also substituted.
dnl
AC_DEFUN([LIBUPNP_CHECK],
[dnl
m4_define([MIN_LIBUPNP_VERSION], [m4_ifval([$1], [$1], [1.6.6])])dnl

dnl	Test for --with
	AC_ARG_WITH(
		[libupnp-prefix],
		[AS_HELP_STRING(
			[--with-libupnp-prefix=PREFIX],
			[UPnP library location])],
		[LIBUPNP_PREFIX="$withval"])

dnl	Check for the presence
	AC_MSG_CHECKING([for libupnp presence])
	AS_IF([test -n "$LIBUPNP_PREFIX"],[dnl
		LIBUPNP_STYLE=prefix
		LIBUPNP_VERSION_STRING=`PKG_CONFIG_PATH=$LIBUPNP_PREFIX/lib/pkgconfig pkg-config libupnp --modversion`
		AS_IF([test -n "$LIBUPNP_VERSION_STRING"], [dnl
			result=yes
			resultstr=" (prefix specified in --with-libupnp-prefix=PREFIX)"dnl
		], [dnl
			result=no
			resultstr=" (maybe an invalid prefix was specified in --with-libupnp-prefix=PREFIX)"dnl
		])dnl
	], [dnl
		AS_IF([pkg-config libupnp --exists], [dnl
			LIBUPNP_PREFIX=`pkg-config libupnp --prefix`
			LIBUPNP_STYLE=system
			LIBUPNP_VERSION_STRING=`pkg-config libupnp --modversion`
		])dnl
		AS_IF([test -n "$LIBUPNP_VERSION_STRING"], [dnl
			result=yes
			resultstr=" (installed in the system)"dnl
		], [dnl
			result=no
			resultstr=" (try to use --with-libupnp-prefix=PREFIX)"dnl
		])dnl
	])
	AC_MSG_RESULT([$result$resultstr])

	AS_IF([test x$result = xyes],[dnl
dnl		Ok, we know that libupnp is in the system, check for the mininum library version required.
		AC_MSG_CHECKING([for libupnp version >= MIN_LIBUPNP_VERSION])
		# Find out the libupnp version and check against the minimum required
		LIBUPNP_VERSION_NUMBER=`echo $LIBUPNP_VERSION_STRING | $AWK 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`
		minvers=`echo MIN_LIBUPNP_VERSION | $AWK 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`

		AS_IF([test -n "$LIBUPNP_VERSION_NUMBER" && test "$LIBUPNP_VERSION_NUMBER" -ge $minvers], [dnl
			result=yes
			resultstr=" (version $LIBUPNP_VERSION_STRING)"
			LIBUPNP_CPPFLAGS=`PKG_CONFIG_PATH=$LIBUPNP_PREFIX/lib/pkgconfig pkg-config libupnp --cflags-only-I`
			LIBUPNP_CFLAGS=`PKG_CONFIG_PATH=$LIBUPNP_PREFIX/lib/pkgconfig pkg-config libupnp --cflags-only-other`
			LIBUPNP_LDFLAGS=`PKG_CONFIG_PATH=$LIBUPNP_PREFIX/lib/pkgconfig pkg-config libupnp --libs-only-other`
			LIBUPNP_LDADD=`PKG_CONFIG_PATH=$LIBUPNP_PREFIX/lib/pkgconfig pkg-config libupnp --libs-only-L`
			LIBUPNP_LDADD="$LIBUPNP_LDADD `PKG_CONFIG_PATH=$LIBUPNP_PREFIX/lib/pkgconfig pkg-config libupnp --libs-only-l`"
			AH_TEMPLATE([LIBUPNP_INCLUDE_PREFIX], [Define this to the include prefix of libupnp])
			AC_DEFINE_UNQUOTED([LIBUPNP_INCLUDE_PREFIX], $LIBUPNP_INCLUDE_PREFIX)dnl
		], [dnl
			result=no
			resultstr=" (version $LIBUPNP_VERSION_STRING is not new enough)"dnl
		])
		AC_MSG_RESULT([$result$resultstr])
	])

dnl	Execute the right action.
	AS_IF([test ${result:-no} = yes], [$2], [$3])

dnl Exported symbols
AC_SUBST([LIBUPNP_CPPFLAGS])dnl
AC_SUBST([LIBUPNP_CFLAGS])dnl
AC_SUBST([LIBUPNP_LDFLAGS])dnl
AC_SUBST([LIBUPNP_LDADD])dnl
m4_undefine([MIN_LIBUPNP_VERSION])dnl
])

