cmake_minimum_required(VERSION 3.20)
project(amule)

# Modern C++ configuration
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optional flag to disable 'register' keyword warnings
option(DISABLE_REGISTER_WARNING "Disable warnings about deprecated 'register' keyword" OFF)
if(DISABLE_REGISTER_WARNING AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
    add_compile_options(-Wno-register)
endif()

# Coroutine support check
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag(-fcoroutines HAS_COROUTINES)
    if(HAS_COROUTINES)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fcoroutines>)
        add_compile_definitions(USE_CPP20 HAS_COROUTINES)
    else()
        message(WARNING "Compiler does not support coroutines")
    endif()
endif()

# Suppress 'register' keyword warnings in generated C++ lexer files
# if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
#    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wno-register>")
# endif()

# Package configuration
set(MIN_BOOST_VERSION 1.47)
set(MIN_CRYPTOPP_VERSION 5.6)
set(MIN_GDLIB_VERSION 2.0.0)
set(MIN_WX_VERSION 3.1.0)
set(PACKAGE "amule")
set(PACKAGE_BUGREPORT "admin@amule.org")
set(PACKAGE_NAME "aMule")
set(PACKAGE_STRING "aMule SVN")
set(PACKAGE_TARNAME "amule")
set(PACKAGE_URL "")
set(PACKAGE_VERSION "SVN")
set(VERSION "GIT")
set(DEFAULT_BUILD_TYPE "Release")
set(RECONF_COMMAND ${CMAKE_COMMAND})

if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    set(DEFAULT_BUILD_TYPE "Debug")
endif()

# Set the possible values of build type for cmake-gui
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE
        STRING "Semicolon separated list of supported configuration types, only supports debug and release, anything else will be ignored" FORCE
    )

    set_property(CACHE CMAKE_CONFIGURATION_TYPES PROPERTY STRINGS
        "Debug" "Release"
    )
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
        STRING "Choose the type of build." FORCE
    )
endif()

include (cmake/CmDaB.cmake)
include (cmake/manpage_install.cmake)
include (cmake/options.cmake)
include (cmake/search-dirs.cmake)

# ICU for better encoding detection
option(ENABLE_ICU "Enable ICU for better encoding detection" ON)
if(ENABLE_ICU)
    include(cmake/icu.cmake)
endif()

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

# Search window debug logging
if(ENABLE_SEARCH_WINDOW_DEBUG)
	add_compile_definitions(ENABLE_SEARCH_WINDOW_DEBUG=1)
else()
	add_compile_definitions(ENABLE_SEARCH_WINDOW_DEBUG=0)
endif()

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

if (UNIX)
	message ("	Should xas XChat2 plugin be installed?				${BUILD_XAS}")
endif()

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
	if (maxminddb_FOUND)
		message ("			libmaxminddb		${maxminddb_VERSION}")
	endif()
endif()

if (ICU_FOUND)
	message ("			libicu				${ICU_VERSION}")
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
