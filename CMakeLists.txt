cmake_minimum_required (VERSION 3.11)
project (amule)
set (MIN_BOOST_VERSION 1.47)
set (MIN_CRYPTOPP_VERSION 5.6)
set (MIN_GDLIB_VERSION 2.0.0)
set (MIN_WX_VERSION 2.8.12)
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

if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
	SET (DEFAULT_BUILD_TYPE "Debug")
endif()

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

if (BUILD_PLASMAMULE)
	find_package (Qt4 COMPONENTS QtCore QtGUI REQUIRED)
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

if (NOT SVNDATE)
	find_package (Git)

	if (GIT_FOUND)
		execute_process (
			COMMAND ${GIT_EXECUTABLE} describe
			OUTPUT_VARIABLE GIT_INFO_WC_REVISION
			OUTPUT_STRIP_TRAILING_WHITESPACE
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
	endif (GIT_FOUND)

	set (SVNDATE "rev. ${GIT_INFO_WC_REVISION}" CACHE STRING "Revision to be written to version string" FORCE)
	message (STATUS "git revision ${SVNDATE} found")
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
endif()

if (BUILD_MONOLITHIC)
	install (FILES amule.desktop
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
	)
endif()

if (BUILD_REMOTEGUI)
	install (FILES amulegui.desktop
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
	)
endif()

add_subdirectory (docs)
add_subdirectory (src)

if (BUILD_TESTING)
	enable_testing()
	add_subdirectory (unittests)
endif()
