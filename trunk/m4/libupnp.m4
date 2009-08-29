#							-*- Autoconf -*-
# This file is part of the aMule project.
# This file is part of the libupnp library project.
#
# Copyright (c) 2003-2008 aMule Team ( admin@amule.org / http://www.amule.org )
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
dnl - LIBUPNP_VERSION
dnl 	Something like "1.6.7"
dnl - LIBUPNP_CPPFLAGS
dnl 	Flags to be added to CPPFLAGS
dnl - LIBUPNP_CFLAGS
dnl 	Flags to be added to CFLAGS
dnl - LIBUPNP_LDFLAGS
dnl 	Flags to be added to LDFLAGS
dnl - LIBUPNP_LIBS
dnl 	Library to be added to LIBS
dnl
dnl The LIBUPNP_CPPFLAGS, LIBUPNP_CFLAGS, LIBUPNP_LDFLAGS and LIBUPNP_LIBS variables are also substituted.
dnl
AC_DEFUN([LIBUPNP_CHECK],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
m4_define([MIN_LIBUPNP_VERSION], [m4_ifval([$1], [$1], [1.6.6])])dnl

dnl	Test for --with-libupnp-prefix
	AC_ARG_WITH(
		[libupnp-prefix],
		[AS_HELP_STRING(
			[--with-libupnp-prefix=PREFIX],
			[UPnP library location])],
		[export PKG_CONFIG_PATH=$withval/lib/pkgconfig])

dnl	Check for libupnp >= MIN_LIBUPNP_VERSION
	AS_IF([test $cross_compiling = no], [
		AC_MSG_CHECKING([for libupnp version >= MIN_LIBUPNP_VERSION])
		AS_IF([test -n "$PKG_CONFIG"], [
			AS_IF([$PKG_CONFIG libupnp --exists], [
				LIBUPNP_VERSION=`$PKG_CONFIG libupnp --modversion`
				AS_IF([$PKG_CONFIG libupnp --atleast-version=MIN_LIBUPNP_VERSION], [
					result=yes
					resultstr=" (version $LIBUPNP_VERSION)"
					LIBUPNP_CPPFLAGS=`$PKG_CONFIG libupnp --cflags-only-I`
					LIBUPNP_CFLAGS=`$PKG_CONFIG libupnp --cflags-only-other`
					LIBUPNP_LDFLAGS=`$PKG_CONFIG libupnp --libs-only-L`
					LIBUPNP_LIBS=`$PKG_CONFIG libupnp --libs-only-other`
					LIBUPNP_LIBS="$LIBUPNP_LIBS `$PKG_CONFIG libupnp --libs-only-l`"
				], [
					result=no
					resultstr=" (version $LIBUPNP_VERSION is not new enough)"
				])
			], [
				result=no
				resultstr=" (try to use --with-libupnp-prefix=PREFIX)"
			])
		], [
			result=no
			resultstr=" (pkg-config not found)"
		])
		AC_MSG_RESULT([$result$resultstr])
		libupnp_error="libupnp >= MIN_LIBUPNP_VERSION not found$resultstr"
	], [
dnl Currently cross-compilation with libupnp is not supported.
		result=no
		libupnp_error="cross compiling"
	])

dnl	Execute the right action.
	AS_IF([test ${result:-no} = yes], [$2], [$3])

dnl Exported symbols
AC_SUBST([LIBUPNP_CPPFLAGS])dnl
AC_SUBST([LIBUPNP_CFLAGS])dnl
AC_SUBST([LIBUPNP_LDFLAGS])dnl
AC_SUBST([LIBUPNP_LIBS])dnl
m4_undefine([MIN_LIBUPNP_VERSION])dnl
])
