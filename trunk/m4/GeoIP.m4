#                                               -*- Autoconf -*-
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
dnl MULE_CHECK_GEOIP
dnl
dnl Checks if the GeoIP library is requested, exists, and whether it should and
dnl could be linked statically.
dnl ---------------------------------------------------------------------------
AC_DEFUN([MULE_CHECK_GEOIP],
[
	MULE_ARG_ENABLE([geoip], [no], [compile with GeoIP IP2Country library])

	MULE_IF_ENABLED([geoip], [
		AC_ARG_WITH([geoip-headers],
			AS_HELP_STRING([--with-geoip-headers=DIR], [GeoIP include files location]),
			[GEOIP_CPPFLAGS="-I$withval"])
		AC_ARG_WITH([geoip-lib],
			AS_HELP_STRING([--with-geoip-lib=DIR], [GeoIP library location]),
			[GEOIP_LDFLAGS="-L$withval"])

		MULE_BACKUP([CPPFLAGS])
		MULE_APPEND([CPPFLAGS], [$GEOIP_CPPFLAGS])
		MULE_BACKUP([LDFLAGS])
		MULE_APPEND([LDFLAGS], [$GEOIP_LDFLAGS])

		AC_CHECK_HEADER([GeoIP.h], [
			AS_IF([test x$SYS = xwin32], [
				AC_MSG_CHECKING([for WinSock library needed by GeoIP])
				# Actually, we might not need any if GeoIP is linked as a .dll
				# - but that's even harder to check for
				AC_COMPILE_IFELSE([
					AC_LANG_PROGRAM([[
						#include <GeoIP.h>
						#ifdef _WINSOCK2_H
							I do know it's not the best approach, but at least works with MinGW stock headers.
							(tested with w32api-3.12)
						#endif
					]])
				], [
					GEOIP_WINSOCK_LIB="-lwsock32"
				], [
					GEOIP_WINSOCK_LIB="-lws2_32"
				])
				AC_MSG_RESULT([$GEOIP_WINSOCK_LIB])
			])
			AC_CHECK_LIB([GeoIP], [GeoIP_open], [
				AC_DEFINE([SUPPORT_GEOIP], [1], [Define if you want GeoIP support.])
				GEOIP_LIBS="-lGeoIP"
				AS_IF([test x$SYS = xwin32], [MULE_APPEND([GEOIP_LIBS], [$GEOIP_WINSOCK_LIB])])
				MULE_APPEND([GEOIP_CPPFLAGS], [-DENABLE_IP2COUNTRY=1])
				AC_ARG_WITH([geoip-static], AS_HELP_STRING([--with-geoip-static], [Explicitly link GeoIP statically (default=no)]),
				[
					AS_IF([test "$withval" != "no" -a ${enable_static:-no} = no], [
						MULE_BACKUP([LIBS])
						MULE_PREPEND([LIBS], [-Wl,-Bstatic $GEOIP_LIBS -Wl,-Bdynamic])
						AC_LINK_IFELSE([
							AC_LANG_PROGRAM([[
								#include <GeoIP.h>
							]], [[
								GeoIP *g = GeoIP_new(GEOIP_STANDARD);
							]])
						], [
							GEOIP_LIBS="-Wl,-Bstatic $GEOIP_LIBS -Wl,-Bdynamic"
						], [
							MULE_WARNING([Cannot link GeoIP statically, because your linker ($LD) does not support it.])
						])
						MULE_RESTORE([LIBS])
					])
				])
			], [
				MULE_ENABLEVAR([geoip])=disabled
				MULE_WARNING([GeoIP support has been disabled because the GeoIP libraries were not found])
			], [${GEOIP_WINSOCK_LIB:-}])
		], [
			MULE_ENABLEVAR([geoip])=disabled
			MULE_WARNING([GeoIP support has been disabled because the GeoIP header files were not found])
		])

		MULE_RESTORE([CPPFLAGS])
		MULE_RESTORE([LDFLAGS])
	])
])
AC_SUBST([GEOIP_CPPFLAGS])dnl
AC_SUBST([GEOIP_LDFLAGS])dnl
AC_SUBST([GEOIP_LIBS])dnl
