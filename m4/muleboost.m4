#							-*- Autoconf -*-
# This file is part of the aMule Project.
#
# Copyright (c) 2015 aMule Team ( admin@amule.org / http://www.amule.org )
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

# -----------------------------------------------------------------------------
# Fake libtool initialization just for Boost
#
# We do not really want to use libtool, but Boost requires it. Let's just
# pretend we did initialize libtool for that single variable ($libext) Boost
# needs.
# -----------------------------------------------------------------------------
m4_defun([_FAKE_LT_INIT],
[# Calculate cc_basename. Skip known compiler wrappers and cross-prefix.
for cc_temp in $compiler""; do
  case $cc_temp in
    compile | *[[\\/]]compile | ccache | *[[\\/]]ccache ) ;;
    distcc | *[[\\/]]distcc | purify | *[[\\/]]purify ) ;;
    \-*) ;;
    *) break;;
 esac
done
cc_basename=`echo "$cc_temp" | $SED "s%.*/%%; s%^$host_alias-%%"`
# All known linkers require a `.a' archive for static linking (except MSVC,
# which needs '.lib').
libext=a
case $cc_basename in
  cl*) libext=lib ;;
esac
])

# -----------------------------------------------------------------------------
# MULE_CHECK_BOOST(MINIMUM-BOOST-VERSION)
#
# Adds a configure flag --with-boost[=DIR]. You may optionally specify the
# location of boost headers (or sources), if they are in a non-standard
# location. If --with-boost if not given, nothing is done. Otherwise it
# checks for the required minumum Boost version, and Boost.Asio. If everything
# is fine, defines ASIO_SOCKETS. If the Boost sources are found, also defines
# HAVE_BOOST_SOURCES. Other flags defined for using Boost:
#   - BOOST_CPPFLAGS
#   - BOOST_SYSTEM_LDFLAGS
#   - BOOST_SYSTEM_LIBS
# -----------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_BOOST],
[
	AC_ARG_WITH([boost],
		[AS_HELP_STRING([--with-boost@<:@=DIR@:>@], [use Boost.Asio for networking])],
		,
		[with_boost=no]
	)
	AS_IF([test ${with_boost:-no} != no], [
		BOOST_REQUIRE([$1], [with_boost=disabled])
		AS_IF([test ${with_boost:-no} != disabled], [
dnl Boost requires libtool. We don't.
			_FAKE_LT_INIT
dnl Expand required macros here, otherwise autoconf may decide to expand them at
dnl the top level, which is highly unwanted and would mess up Boost detection.
			_BOOST_FIND_COMPILER_TAG
			BOOST_STATIC
			_BOOST_GUESS_WHETHER_TO_USE_MT
			AC_MSG_CHECKING([for Boost sources])
			MULE_BACKUP([CPPFLAGS])
			MULE_APPEND([CPPFLAGS], [$BOOST_CPPFLAGS])
			AC_COMPILE_IFELSE([
				AC_LANG_PROGRAM([[#include <boost/../libs/system/src/error_code.cpp>]])
			], [
				AC_DEFINE([HAVE_BOOST_SOURCES], [1], [Define to 1 if you have the Boost sources])
				AC_MSG_RESULT([yes])
			], [
				AC_MSG_RESULT([no])
				BOOST_SYSTEM([mt])
				AS_IF([test ${boost_cv_lib_system:-no} != yes], [
					MULE_WARNING([Boost support has been disabled because Boost.System not found])
					with_boost=disabled
				])
			])
			MULE_RESTORE([CPPFLAGS])
dnl Not using BOOST_ASIO here, because it doesn't have ACTION-IF[-NOT]-FOUND parameters.
			BOOST_FIND_HEADER([boost/asio.hpp], [
				MULE_WARNING([Boost support has been disabled because Boost.Asio not found])
				with_boost=disabled
			], [AC_DEFINE([ASIO_SOCKETS], [1], [Define to 1 if you have <boost/asio.hpp> and are using Boost.Asio for networking.])])
		], [MULE_WARNING([Boost support has been disabled because of insufficient Boost version.])])
	])
])
