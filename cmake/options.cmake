#
# This file is part of the aMule Project.
#
# Copyright (c) 2011 Werner Mahr (Vollstrecker) <amule@vollstreckernet.de>
#
# Any parts of this program contributed by third-party developers are copyrighted
# by their respective authors.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
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
# This file contains the options for en- or disabling parts of aMule, and
# sets the needed variables for them to compile
#

option (BUILD_ALC "compile aLinkCreator GUI version")
option (BUILD_ALCC "compile aLinkCreator for console")
option (BUILD_AMULECMD "compile aMule command line client")

if (UNIX)
	option (BUILD_CAS "compile C aMule Statistics")
endif()

option (BUILD_DAEMON "compile aMule daemon version")
option (BUILD_ED2K "compile aMule ed2k links handler" ON)
option (BUILD_EVERYTHING "compile all parts of aMule")
option (BUILD_FILEVIEW "compile aMule file viewer for console (EXPERIMENTAL)")
option (BUILD_MONOLITHIC "enable building of the monolithic aMule app" ON)

if (UNIX)
	option (BUILD_PLASMAMULE "compile aMule plasma applet and engine")
endif()

option (BUILD_REMOTEGUI "compile aMule remote GUI")
option (BUILD_WEBSERVER "compile aMule WebServer")
option (BUILD_WXCAS "compile aMule GUI Statistics")
option (BUILD_XAS "install xas XChat2 plugin")
option (BUILD_TESTING "Run Tests after compile" ON)

if (PREFIX)
	set (CMAKE_INSTALL_PREFIX "${PREFIX}")
endif()

include (GNUInstallDirs)

set (PKGDATADIR "${CMAKE_INSTALL_DATADIR}/${PACKAGE}")

if (BUILD_EVERYTHING)
	set (BUILD_ALC ON CACHE BOOL "compile aLinkCreator GUI version" FORCE)
	set (BUILD_ALCC ON CACHE BOOL "compile aLinkCreator for console" FORCE)
	set (BUILD_AMULECMD ON CACHE BOOL "compile aMule command line client" FORCE)

	if (UNIX)
		set (BUILD_CAS ON CACHE BOOL "compile C aMule Statistics" FORCE)
	endif()

	set (BUILD_DAEMON ON CACHE BOOL "compile aMule daemon version" FORCE)
	set (BUILD_FILEVIEW ON CACHE BOOL "compile aMule file viewer for console (EXPERIMENTAL)" FORCE)

#	if (UNIX)
#		set (BUILD_PLASMAMULE ON CACHE BOOL )
#	endif()

	set (BUILD_REMOTEGUI ON CACHE BOOL "compile aMule remote GUI" FORCE)
	set (BUILD_WEBSERVER ON CACHE BOOL "compile aMule WebServer" FORCE)
	set (BUILD_WXCAS ON CACHE BOOL "compile aMule GUI Statistics" FORCE)
	set (BUILD_XAS ON CACHE BOOL "install xas XChat2 plugin" FORCE)
endif()

if (BUILD_AMULECMD)
	set (NEED_LIB_EC TRUE)
	set (NEED_LIB_MULECOMMON TRUE)
	set (NEED_LIB_MULESOCKET TRUE)
	set (wx_NEED_NET TRUE)
	set (NEED_ZLIB TRUE)
endif()

if (BUILD_CAS)
	set (BUILD_UTIL TRUE)
endif()

if (BUILD_ALCC)
	set (BUILD_UTIL TRUE)
	set (wx_NEED_BASE TRUE)
endif()

if (BUILD_ALC)
	set (BUILD_UTIL TRUE)
	set (wx_NEED_GUI TRUE)
endif()

if (BUILD_XAS)
	set (BUILD_UTIL TRUE)
endif()

if (BUILD_DAEMON)
	set (NEED_LIB_EC TRUE)
	set (NEED_LIB_MULEAPPCOMMON TRUE)
	set (NEED_LIB_MULECOMMON TRUE)
	set (NEED_LIB_MULESOCKET TRUE)
	set (NEED_ZLIB TRUE)
	set (wx_NEED_NET TRUE)
endif()

if (BUILD_ED2K)
	set (wx_NEED_BASE TRUE)
endif()

if (BUILD_FILEVIEW)
	set (BUILD_UTIL TRUE)
	set (NEED_LIB_CRYPTO TRUE)
	set (NEED_LIB_MULECOMMON TRUE)
	set (wx_NEED_NET TRUE)
endif()

if (BUILD_MONOLITHIC)
	set (NEED_LIB_EC TRUE)
	set (NEED_LIB_MULEAPPGUI TRUE)
	set (NEED_LIB_MULEAPPCOMMON TRUE)
	set (NEED_LIB_MULECOMMON TRUE)
	set (NEED_LIB_MULESOCKET TRUE)
	set (NEED_ZLIB TRUE)
	set (wx_NEED_ADV TRUE)
	set (wx_NEED_NET TRUE)
endif()

if (BUILD_MONOLITHIC OR BUILD_REMOTEGUI)
	set (INSTALL_SKINS TRUE)
endif()

if (BUILD_PLASMAMULE)
	set (BUILD_UTIL TRUE)
endif()

if (BUILD_REMOTEGUI)
	set (NEED_GLIB_CHECK TRUE)
	set (NEED_LIB_EC TRUE)
	set (NEED_LIB_MULEAPPCOMMON TRUE)
	set (NEED_LIB_MULEAPPGUI TRUE)
	set (NEED_LIB_MULECOMMON TRUE)
	set (NEED_LIB_MULESOCKET TRUE)
	set (NEED_ZLIB TRUE)
	set (wx_NEED_ADV TRUE)
	set (wx_NEED_NET TRUE)
endif()

if (BUILD_WEBSERVER)
	set (NEED_LIB_EC TRUE)
	set (NEED_LIB_MULECOMMON TRUE)
	set (NEED_LIB_MULESOCKET TRUE)
	set (NEED_ZLIB TRUE)
	set (WEBSERVERDIR "${PKGDATADIR}/webserver/")
	set (wx_NEED_NET TRUE)
endif()

if (BUILD_WXCAS)
	set (BUILD_UTIL TRUE)
	set (wx_NEED_GUI TRUE)
	set (wx_NEED_NET TRUE)
endif()

if (NEED_LIB_EC)
	set (NEED_LIB_CRYPTO TRUE)
endif()

if (NEED_LIB_MULECOMMON OR NEED_LIB_EC)
	set (NEED_LIB TRUE)
	set (wx_NEED_BASE TRUE)
endif()

if (NEED_LIB_MULECOMMON)
	set (NEED_GLIB_CHECK TRUE)
endif()

if (NEED_LIB_MULEAPPCOMMON)
	option (ENABLE_BOOST "compile with Boost.ASIO Sockets" ON)
	option (ENABLE_IP2COUNTRY "compile with GeoIP IP2Country library")
	option (ENABLE_MMAP "enable using mapped memory if supported")
	option (ENABLE_NLS "enable national language support" ON)
	set (NEED_LIB_MULEAPPCORE TRUE)
	set (wx_NEED_BASE TRUE)
else()
	set (ENABLE_BOOST FALSE)
	set (ENABLE_IP2COUNTRY FALSE)
	set (ENABLE_MMAP FALSE)
	set (ENABLE_NLS FALSE)
endif()

if (NEED_LIB_MULEAPPGUI)
	set (wx_NEED_GUI TRUE)
endif()

if (NEED_LIB_MULESOCKET)
	set (wx_NEED_BASE TRUE)
endif()

if (ENABLE_BOOST AND NOT (BUILD_DAEMON OR BUILD_MONOLITHIC OR BUILD_REMOTEGUI OR BUILD_WXCAS))
	set (wx_NEED_NET FALSE)
endif()

if (wx_NEED_ADV OR wx_NEED_BASE OR wx_NEED_GUI OR wx_NEED_NET)
	set (wx_NEEDED TRUE)

	if (WIN32 AND NOT wx_NEED_BASE)
		set (wx_NEED_BASE TRUE)
	endif()
endif()

ADD_COMPILE_DEFINITIONS ($<$<CONFIG:DEBUG>:__DEBUG__>)

IF (WIN32)
	ADD_COMPILE_DEFINITIONS ($<$<CONFIG:DEBUG>:wxDEBUG_LEVEL=0>)
ENDIF (WIN32)

if (NEED_LIB_MULEAPPCOMMON OR BUILD_WEBSERVER)
	option (ENABLE_UPNP "enable UPnP support in aMule" ON)
endif()
