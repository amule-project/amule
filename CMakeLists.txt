cmake_minimum_required (VERSION 3.10...3.31)
project (amule)

set (MIN_BOOST_VERSION 1.70)
set (MIN_CRYPTOPP_VERSION 5.6)
set (MIN_GDLIB_VERSION 2.0.0)
set (MIN_WX_VERSION 3.2.0)
set (PACKAGE "amule")
set (PACKAGE_BUGREPORT "admin@amule.org")
set (PACKAGE_NAME "aMule")
set (PACKAGE_STRING "aMule SVN")
set (PACKAGE_TARNAME "amule")
set (PACKAGE_URL \"\")
set (PACKAGE_VERSION "SVN")
set (VERSION "GIT")
set (DEFAULT_BUILD_TYPE "Release")
set (RECONF_COMMAND ${CMAKE_COMMAND})
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the possible values of build type for cmake-gui
if (CMAKE_CONFIGURATION_TYPES)
	set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE
		STRING "Semicolon separated list of supported configuration types, only supports debug and release, anything else will be ignored" FORCE
	)

	set_property (CACHE CMAKE_CONFIGURATION_TYPES PROPERTY STRINGS
		"Debug" "Release"
	)
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message (STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
	set (CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
		STRING "Choose the type of build." FORCE
	)
endif()

# Try to use ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "ccache found: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    # Add other languages if needed, e.g., CMAKE_CUDA_COMPILER_LAUNCHER
else()
    message(STATUS "ccache not found.")
endif()

include (cmake/CmDaB.cmake)
include (cmake/manpage_install.cmake)
include (cmake/options.cmake)
include (cmake/search-dirs.cmake)

if (BUILD_AMULECMD OR BUILD_WEBSERVER)
	include (cmake/FindReadline.cmake)
endif()

if (BUILD_CAS)
	include (cmake/gdlib.cmake)
	include (cmake/getopt_long.cmake)
endif()

if (BUILD_WEBSERVER OR NEED_ZLIB)
	include (cmake/zlib.cmake)
endif()

if (BUILD_WEBSERVER)
	include (cmake/png.cmake)
endif()

if (ENABLE_BOOST)
	include (cmake/boost.cmake)
endif()

if (ENABLE_IP2COUNTRY)
	include (cmake/ip2country.cmake)
endif()

if (ENABLE_NLS)
	include (cmake/nls.cmake)
endif()

if (ENABLE_UPNP)
	include (cmake/upnp.cmake)
endif()

if (NEED_GLIB_CHECK)
	include (cmake/glib21.cmake)
endif()

if (NEED_LIB_CRYPTO)
	include (cmake/cryptopp.cmake)
endif()

if (wx_NEEDED)
	include (cmake/wx.cmake)
endif()

#
# SVNDATE is consumed by config.h.cm (#define SVNDATE "...") and
# version.rc.in (Windows version resource), and ends up in the binary's
# `--version` output and the amuled startup banner.
#
# Two modes:
#   * Distro packagers building from a tarball (no .git) pass
#     -DSVNDATE="rev. foo" on the cmake command line.  That value lands
#     in the cache and is respected here unconditionally.
#   * Source builds derive the value from `git describe` on EVERY
#     configure so subsequent commits show up in the banner without a
#     full reconfigure.  The previous code wrapped this in
#     `if (NOT SVNDATE)` and stored the result with `CACHE STRING FORCE`,
#     which froze the value at the first configure forever — every
#     incremental rebuild thereafter embedded the stale revision string.
#
# CMAKE_CONFIGURE_DEPENDS on .git/HEAD makes the build system re-run
# cmake when HEAD changes (commit, checkout, pull), so `cmake --build`
# alone is enough to pick up new commits in the banner.
#
if (NOT DEFINED CACHE{SVNDATE})
	find_package (Git)
	if (GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
		execute_process (
			COMMAND ${GIT_EXECUTABLE} describe
			OUTPUT_VARIABLE GIT_INFO_WC_REVISION
			OUTPUT_STRIP_TRAILING_WHITESPACE
			RESULT_VARIABLE GIT_INFO_RESULT
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		if (GIT_INFO_RESULT EQUAL 0 AND GIT_INFO_WC_REVISION)
			set (SVNDATE "rev. ${GIT_INFO_WC_REVISION}")
			message (STATUS "git revision ${SVNDATE} found")
		endif()
	endif()
	set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
		"${CMAKE_SOURCE_DIR}/.git/HEAD")
endif()

include (cmake/bfd.cmake)

configure_file (
	config.h.cm
	config.h
)

if (WIN32)
	configure_file (
		version.rc.in
		version.rc
	)

	# Silences 80+ CI spam lines from MSYS2 `winsock2.h:15` which prints
	# `#warning Please include winsock2.h before windows.h` whenever
	# <windows.h> pulls in the legacy WinSock API first. Defining
	# WIN32_LEAN_AND_MEAN tells windows.h to skip that legacy include,
	# leaving <winsock2.h> to be pulled in cleanly by wx headers.
	# Upstream wx 3.3 does this inside wrapwin.h; wx 3.2 does not.
	add_compile_definitions(WIN32_LEAN_AND_MEAN)
endif()

if (BUILD_MONOLITHIC)
	install (FILES amule.desktop
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
	)
	# Ship the PNG application icon in the XDG hicolor theme so
	# desktop environments (GTK, KDE, Wayland shells, file managers)
	# can resolve `Icon=amule` from amule.desktop.
	install (FILES amule.png
		DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps"
	)
endif()

if (BUILD_REMOTEGUI)
	install (FILES amulegui.desktop
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
	)
endif()

install (FILES org.amule.amule.metainfo.xml
	DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
)

if (ENABLE_NLS)
	include (FindGettext)
	add_subdirectory (po)
endif()

add_subdirectory (docs)
add_subdirectory (src)

if (BUILD_TESTING)
	enable_testing()
	add_subdirectory (unittests)
endif()

message (STATUS "

	Configured aMule ${PACKAGE_VERSION}${SVN_REVISION} for '${CMAKE_SYSTEM}' on '${CMAKE_SYSTEM_PROCESSOR}'.

	aMule enabled options:

	**** aMule Core ****
	Prefix where aMule should be installed?				${CMAKE_PREFIX_PATH}
	Should aMule be compiled with i18n support?			${ENABLE_NLS}
	Which mode should aMule be compiled in?				${CMAKE_BUILD_TYPE}
	Should aMule be compiled with UPnP support?			${ENABLE_UPNP}
	Should aMule be compiled with IP2country support?		${ENABLE_IP2COUNTRY}
	Should aMule monolithic application be built?			${BUILD_MONOLITHIC}
	Should aMule daemon version be built?				${BUILD_DAEMON}
	Should aMule remote gui be built?				${BUILD_REMOTEGUI}

	**** aMule TextClient ****
	Should aMule Command Line Client be built?			${BUILD_AMULECMD}

	**** aMule WebServer ****
	Should aMule WebServer be built?				${BUILD_WEBSERVER}

	**** aMule ED2K Links Handler ****
	Should aMule ED2K Links Handler be built?			${BUILD_ED2K}

	**** aMuleLinkCreator ****
	Should aMuleLinkCreator GUI version (alc) be built?		${BUILD_ALC}
	Should aMuleLinkCreator for console (alcc) be built?		${BUILD_ALCC}

	**** aMule Statistics ****
	Should C aMule Statistics (CAS) be built?			${BUILD_CAS}
	Should aMule GUI Statistics (wxCas) be built?			${BUILD_WXCAS}"
)

message ("
	**** General Libraries and Tools ****
	Should aMule file viewer for console be built?			${BUILD_FILEVIEW}

	Libraries aMule will use to build:"
)

if (NEED_WX)
	message (STATUS "			wxWidgets				${WX_VERSION}")
endif()

if (ASIO_SOCKETS)
	message ("			boost				${Boost_VERSION}")
endif()

if (NEED_LIB_CRYPTO)
	message ("			crypto++			${CRYPTOPP_VERSION} in ${CRYPTOPP_INCLUDE_PREFIX}")
endif()

if (ENABLE_UPNP)
	message ("			libupnp				${LIBUPNP_VERSION}")
endif()

message ("			libintl				${ENABLE_NLS}")

if (ENABLE_IP2COUNTRY)
	message ("			libGeoIP			${GEOIP_LIB}")
endif()

if (BUILD_WEBSERVER)
	message ("			libpng				${PNG_VERSION_STRING}")
endif()

if (BUILD_CAS)
	message ("			libgd				${gdlib_VERSION}")
endif()

if (NEED_ZLIB)
	message ("			zlib				${ZLIB_VERSION_STRING}")
endif()
