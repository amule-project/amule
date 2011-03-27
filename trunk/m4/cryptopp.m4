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

dnl --------------------------------------------------------------------------
dnl MULE_CHECK_CRYPTOPP([VERSION = 5.1], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl
dnl Check for cryptopp library
dnl --------------------------------------------------------------------------
dnl
dnl This macro sets these variables:
dnl - CRYPTOPP_PREFIX
dnl 	This is the user or system directory where crypto++ is installed or sources
dnl - CRYPTOPP_VERSION_STRING
dnl 	Something like "5.5.2"
dnl - CRYPTOPP_VERSION_NUMBER
dnl 	Something like 5005002
dnl - CRYPTOPP_STYLE
dnl 	"sources", "installed" or "gentoo_debian"
dnl - CRYPTOPP_LIB_NAME
dnl 	"cryptopp" or "crypto++"
dnl - CRYPTOPP_INCLUDE_PREFIX
dnl 	The string that goes here: #include <@CRYPTOPP_INCLUDE_PREFIX@/rsa.h>
dnl - CRYPTOPP_CPPFLAGS
dnl 	Flags to be added to CPPFLAGS
dnl - CRYPTOPP_LDFLAGS
dnl 	Flags to be added to LDFLAGS
dnl - CRYPTOPP_LIBS
dnl	Library to be added to LIBS
dnl
dnl The CRYPTOPP_CPPFLAGS, CRYPTOPP_LDFLAGS and CRYPTOPP_LIBS variables are also substituted.
dnl
dnl Worth notice:
dnl - cryptopp_includedir
dnl 	The string that goes in -I on CPPFLAGS
dnl - cryptopp_libdir
dnl 	The string that goes in -L on LDFLAGS
dnl - cryptopp_header_path
dnl 	The file we use to discover the version of cryptopp
dnl
AC_DEFUN([MULE_CHECK_CRYPTOPP],
[dnl
AC_REQUIRE([AC_PROG_EGREP])dnl
m4_define([MIN_CRYPTO_VERSION], [m4_ifval([$1], [$1], [5.1])])dnl

	AC_ARG_WITH([crypto-prefix],
		[AS_HELP_STRING([--with-crypto-prefix=PREFIX], [prefix where crypto++ is installed])])

	AC_MSG_CHECKING([for crypto++ version >= MIN_CRYPTO_VERSION])

	cryptopp_file_with_version="cryptlib.h"

	CRYPTOPP_STYLE="unknown"
	CRYPTOPP_LIB_NAME="unknown"
	cryptopp_includedir="unknown"
	CRYPTOPP_INCLUDE_PREFIX="unknown"
	cryptopp_libdir="unknown"

	AS_IF([test -n "$with_crypto_prefix"], [
		CRYPTOPP_PREFIX="$with_crypto_prefix"
		# Find the Cryptopp header in the user-provided location
		MULE_IF([test -f $CRYPTOPP_PREFIX/$cryptopp_file_with_version], [
			CRYPTOPP_STYLE="sources"
			CRYPTOPP_LIB_NAME="cryptopp"
			cryptopp_includedir=
			CRYPTOPP_INCLUDE_PREFIX="$CRYPTOPP_PREFIX"
			cryptopp_libdir=
		], [test -f $CRYPTOPP_PREFIX/include/cryptopp/$cryptopp_file_with_version], [
			CRYPTOPP_STYLE="installed"
			CRYPTOPP_LIB_NAME="cryptopp"
			cryptopp_includedir="$CRYPTOPP_PREFIX/include"
			CRYPTOPP_INCLUDE_PREFIX="$CRYPTOPP_LIB_NAME"
			cryptopp_libdir="$CRYPTOPP_PREFIX/lib"
		], [test -f $CRYPTOPP_PREFIX/include/crypto++/$cryptopp_file_with_version], [
			# Debian uses libcrypto++5.1 - it's not my fault, please soda patch the package
			CRYPTOPP_STYLE="gentoo_debian"
			CRYPTOPP_LIB_NAME="crypto++"
			cryptopp_includedir="$CRYPTOPP_PREFIX/include"
			CRYPTOPP_INCLUDE_PREFIX="$CRYPTOPP_LIB_NAME"
			cryptopp_libdir="$CRYPTOPP_PREFIX/lib"
		])
	], [
		for CRYPTOPP_PREFIX in /usr /usr/local /opt /opt/local /usr/pkg /mingw ; do
			# Find the Cryptopp header in system locations
			MULE_IF([test -f $CRYPTOPP_PREFIX/$cryptopp_file_with_version], [
				CRYPTOPP_STYLE="sources"
				CRYPTOPP_LIB_NAME="cryptopp"
				cryptopp_includedir=
				CRYPTOPP_INCLUDE_PREFIX="$CRYPTOPP_PREFIX"
				cryptopp_libdir=
				break
			], [test -f $CRYPTOPP_PREFIX/include/cryptopp/$cryptopp_file_with_version], [
				CRYPTOPP_STYLE="installed"
				CRYPTOPP_LIB_NAME="cryptopp"
				cryptopp_includedir="$CRYPTOPP_PREFIX/include"
				CRYPTOPP_INCLUDE_PREFIX="$CRYPTOPP_LIB_NAME"
				cryptopp_libdir="$CRYPTOPP_PREFIX/lib"
				break
			], [test -f $CRYPTOPP_PREFIX/include/crypto++/$cryptopp_file_with_version], [
				# Debian uses libcrypto++5.1 - it's not my fault, please soda patch the package
				CRYPTOPP_STYLE="gentoo_debian"
				CRYPTOPP_LIB_NAME="crypto++"
				cryptopp_includedir="$CRYPTOPP_PREFIX/include"
				CRYPTOPP_INCLUDE_PREFIX="$CRYPTOPP_LIB_NAME"
				cryptopp_libdir="$CRYPTOPP_PREFIX/lib"
				break
			])
		done
	])

	AS_IF([test $CRYPTOPP_STYLE = "unknown"], [result=no; resultstr=""], [
		# Find out the crypto++ version and check against the minimum required
		cryptopp_header_path="${cryptopp_includedir+$cryptopp_includedir/}$CRYPTOPP_INCLUDE_PREFIX/$cryptopp_file_with_version"
		CRYPTOPP_VERSION_STRING=`$EGREP "Reference Manual|API Reference" $cryptopp_header_path | sed -e ['s/[^0-9]*\([0-9.]*\).*/\1/']`
		CRYPTOPP_VERSION_NUMBER=`echo $CRYPTOPP_VERSION_STRING | $AWK 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`
		minvers=`echo MIN_CRYPTO_VERSION | $AWK 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`

		AS_IF([test -n "$CRYPTOPP_VERSION_NUMBER" && test "$CRYPTOPP_VERSION_NUMBER" -ge $minvers], [
			result=yes
			resultstr=" (version $CRYPTOPP_VERSION_STRING, $CRYPTOPP_STYLE)"
			AS_IF([test -n "$cryptopp_includedir"], [CRYPTOPP_CPPFLAGS="-I$cryptopp_includedir"], [CRYPTOPP_CPPFLAGS=])
			AS_IF([test -n "$cryptopp_libdir"], [
				CRYPTOPP_LDFLAGS="-L$cryptopp_libdir"
				CRYPTOPP_LIBS="-l$CRYPTOPP_LIB_NAME"
			], [
				CRYPTOPP_LDFLAGS=
				CRYPTOPP_LIBS="${CRYPTOPP_INCLUDE_PREFIX}/lib${CRYPTOPP_LIB_NAME}.a"
			])
			AH_TEMPLATE([CRYPTOPP_INCLUDE_PREFIX], [Define this to the include prefix of crypto++])
			AC_DEFINE_UNQUOTED([CRYPTOPP_INCLUDE_PREFIX], $CRYPTOPP_INCLUDE_PREFIX)
		], [
			result=no
			resultstr=" (version $CRYPTOPP_VERSION_STRING is not new enough)"
		])
	])

	AC_MSG_RESULT([$result$resultstr])

	AS_IF([test ${result:-no} = yes], [$2], [$3])

dnl Exported symbols
AC_SUBST([CRYPTOPP_CPPFLAGS])dnl
AC_SUBST([CRYPTOPP_LDFLAGS])dnl
AC_SUBST([CRYPTOPP_LIBS])dnl
m4_undefine([MIN_CRYPTO_VERSION])dnl
])
