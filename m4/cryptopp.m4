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
dnl	This is the directory where crypto++ is located, if found. The format
dnl	is suitable to be passed to --with-crypto-prefix.
dnl - CRYPTOPP_VERSION_STRING
dnl	Something like "5.5.2"
dnl - CRYPTOPP_VERSION_NUMBER
dnl	Something like 5005002
dnl - CRYPTOPP_LIB_NAME
dnl	"cryptopp" or "crypto++"
dnl
dnl Exported variables (to amule-config.h, to be used in C++ source files):
dnl - CRYPTOPP_INCLUDE_PREFIX
dnl	The string that goes here: #include <CRYPTOPP_INCLUDE_PREFIX/rsa.h>
dnl
dnl Substituted variables (to be used in Makefiles):
dnl - CRYPTOPP_CPPFLAGS
dnl	Flags to be added to CPPFLAGS
dnl - CRYPTOPP_LDFLAGS
dnl	Flags to be added to LDFLAGS
dnl - CRYPTOPP_LIBS
dnl	Library to be added to LIBS
dnl
AC_DEFUN([MULE_CHECK_CRYPTOPP],
[dnl
AC_REQUIRE([AC_PROG_EGREP])dnl
AC_LANG_ASSERT([C++])dnl
m4_define([MIN_CRYPTO_VERSION], [m4_ifval([$1], [$1], [5.1])])dnl
m4_define([cryptopp_file_with_version], [cryptlib.h])dnl

	AC_ARG_WITH([crypto-prefix],
		[AS_HELP_STRING([--with-crypto-prefix=PREFIX], [prefix where crypto++ is installed])])

	AC_MSG_CHECKING([for crypto++ version >= MIN_CRYPTO_VERSION])

	cryptopp_found=false

	AS_IF([test -n "$with_crypto_prefix"], [
		CRYPTOPP_PREFIX="$with_crypto_prefix"
		# Find the Cryptopp header in the user-provided location
		MULE_IF([test -f $CRYPTOPP_PREFIX/cryptopp_file_with_version], [
			cryptopp_found=true
			CRYPTOPP_LIB_NAME="cryptopp"
			CRYPTOPP_INCLUDE_PREFIX="$CRYPTOPP_PREFIX"
			CRYPTOPP_CPPFLAGS=
			CRYPTOPP_LDFLAGS=
			CRYPTOPP_LIBS="${CRYPTOPP_PREFIX}/lib${CRYPTOPP_LIB_NAME}.a"
		], [
			for CRYPTOPP_LIB_NAME in "cryptopp" "crypto++"; do
				AS_IF([test -f $CRYPTOPP_PREFIX/include/$CRYPTOPP_LIB_NAME/cryptopp_file_with_version], [
					cryptopp_found=true
					CRYPTOPP_INCLUDE_PREFIX="$CRYPTOPP_LIB_NAME"
					CRYPTOPP_CPPFLAGS="-I$CRYPTOPP_PREFIX/include"
					CRYPTOPP_LDFLAGS="-L$CRYPTOPP_PREFIX/lib"
					CRYPTOPP_LIBS="-l$CRYPTOPP_LIB_NAME"
					break
				])
			done
		])
	], [
		# Check whether the compiler can find it
		for CRYPTOPP_LIB_NAME in "cryptopp" "crypto++"; do
			AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
					#include <$CRYPTOPP_LIB_NAME/]cryptopp_file_with_version[>
				])
			], [
				cryptopp_found=true
				CRYPTOPP_INCLUDE_PREFIX="$CRYPTOPP_LIB_NAME"
				CRYPTOPP_CPPFLAGS=
				CRYPTOPP_LDFLAGS=
				CRYPTOPP_LIBS="-l$CRYPTOPP_LIB_NAME"
				break
			])
		done
	])

	AS_IF([$cryptopp_found], [
		# Find out the crypto++ version and check against the minimum required
		# Ask the compiler where are the crypto++ files
		MULE_BACKUP([CPPFLAGS])
		MULE_APPEND([CPPFLAGS], [$CRYPTOPP_CPPFLAGS])
dnl Hack begin!
dnl To be able to preprocess the file but scan the preprocessed results
dnl ourselves, we need to use undocumented features of autoconf.
		AC_LANG_CONFTEST([AC_LANG_SOURCE([
			#include <$CRYPTOPP_INCLUDE_PREFIX/]cryptopp_file_with_version[>
		])])
		[cryptopp_header_path=`(eval "$ac_cpp conftest.$ac_ext") 2>&]AS_MESSAGE_LOG_FD[ | sed -e '/^#.*]cryptopp_file_with_version[/{s/.*"\(.*\)".*/\1/;q;};d'`]
		rm -f conftest*
dnl Hack end!
		MULE_RESTORE([CPPFLAGS])
		# Set a prefix suitable for --with-crypto-prefix
		CRYPTOPP_PREFIX="${cryptopp_header_path%/*}"
		CRYPTOPP_PREFIX="${CRYPTOPP_PREFIX%/include/${CRYPTOPP_LIB_NAME}}"
		# Now check the version
		CRYPTOPP_VERSION_STRING=`$EGREP "Reference Manual|API Reference" $cryptopp_header_path | sed -e ['s/[^0-9]*\([0-9.]*\).*/\1/']`
		CRYPTOPP_VERSION_NUMBER=`echo $CRYPTOPP_VERSION_STRING | $AWK 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`
		minvers=`echo MIN_CRYPTO_VERSION | $AWK 'BEGIN { FS = "."; } { printf "%d", ([$]1 * 1000 + [$]2) * 1000 + [$]3;}'`

		AS_IF([test -n "$CRYPTOPP_VERSION_NUMBER" && test "$CRYPTOPP_VERSION_NUMBER" -ge $minvers], [
			result=yes
			resultstr=" (version $CRYPTOPP_VERSION_STRING)"
			AH_TEMPLATE([CRYPTOPP_INCLUDE_PREFIX], [Define this to the include prefix of crypto++])
			AC_DEFINE_UNQUOTED([CRYPTOPP_INCLUDE_PREFIX], $CRYPTOPP_INCLUDE_PREFIX)
		], [
			result=no
			resultstr=" (version $CRYPTOPP_VERSION_STRING is not new enough)"
			CRYPTOPP_CPPFLAGS=
			CRYPTOPP_LDFLAGS=
			CRYPTOPP_LIBS=
		])
	], [result="no"; resultstr=])

	AC_MSG_RESULT([$result$resultstr])

	AS_IF([test ${result:-no} = yes], [$2], [$3])

dnl Exported symbols
AC_SUBST([CRYPTOPP_CPPFLAGS])dnl
AC_SUBST([CRYPTOPP_LDFLAGS])dnl
AC_SUBST([CRYPTOPP_LIBS])dnl
m4_undefine([cryptopp_file_with_version])dnl
m4_undefine([MIN_CRYPTO_VERSION])dnl
])
