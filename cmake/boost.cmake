find_package(Boost CONFIG REQUIRED)

include (CheckIncludeFiles)

if (WIN32)
	set (Boost_USE_STATIC_LIBS TRUE)
endif()

set (CMAKE_REQUIRED_INCLUDES ${Boost_INCLUDE_DIRS})

if (MINGW)
	# check_include_files on MinGW does a link step, and boost::asio
	# pulls in WSAStartup/WSACleanup from ws2_32 — passing the lib via
	# CMAKE_REQUIRED_LIBRARIES isn't reliably honoured with this
	# CMake + CheckIncludeFiles combination, so the link fails even
	# when the headers are perfectly usable. find_package(Boost CONFIG
	# REQUIRED) above already verified boost is installed — skip the
	# redundant compile-link check on MinGW only.  MSVC keeps it.
	set (ASIO_SOCKETS TRUE)
else()
	# Force a re-check every configure: without this an earlier
	# missing-headers result lives forever in the CMakeCache, even
	# after the user installs libboost-dev properly. Cheap (one
	# compile of a 3-line test program) and avoids the cache-poison
	# trap the previous version of this file had.
	unset (ASIO_SOCKETS CACHE)
	if (WIN32)
		set (CMAKE_REQUIRED_FLAGS " -DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB -DBOOST_SYSTEM_NO_LIB -DBOOST_ERROR_CODE_HEADER_ONLY")
	else()
		set (CMAKE_REQUIRED_FLAGS "-DBOOST_ERROR_CODE_HEADER_ONLY")
	endif()
	check_include_files ("boost/system/error_code.hpp;boost/asio.hpp" ASIO_SOCKETS LANGUAGE CXX)
endif (MINGW)

if (NOT ASIO_SOCKETS)
	# boost::asio is mandatory. The previous version of this file
	# silently downgraded to wxWidgets sockets when DOWNLOAD_AND_BUILD_DEPS
	# was set, which produced a build that compiled the long-deprecated
	# wx-sockets fallback path (issue #600). That path is being removed;
	# there is no fallback to fall back to.
	message (FATAL_ERROR "boost::asio headers were not usable. "
		"find_package(Boost) found a Boost install but "
		"check_include_files for boost/system/error_code.hpp + "
		"boost/asio.hpp failed to compile. Install full Boost "
		"development headers (Debian/Ubuntu: libboost-dev, Fedora: "
		"boost-devel, macOS Homebrew: boost, MSYS2: "
		"mingw-w64-x86_64-boost) and re-configure. If the headers "
		"are clearly present, inspect build/CMakeFiles/CMakeError.log "
		"to see why the compile probe failed (typically a missing "
		"transitive dependency or a C++ standard mismatch).")
endif()

if (MINGW)
	# Carry the WinSock libs over to the real targets that link boost::asio.
	set (Boost_LIBRARIES "ws2_32;mswsock" CACHE STRING "Libraries needed for linking with boost" FORCE)
else()
	set (Boost_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES} CACHE STRING "Libraries needed for linking with boost" FORCE)
endif()
# find_package(Boost CONFIG) above sets a local non-cache Boost_LIBRARIES
# that masks the cache value we just wrote — clear it so the cache shows through.
unset (Boost_LIBRARIES)
set (BOOST_ERROR_CODE_HEADER_ONLY TRUE CACHE INTERNAL "When true, boost_system lib is not needed for linking" FORCE)
unset (CMAKE_REQUIRED_INCLUDES)
unset (CMAKE_REQUIRED_FLAGS)
unset (CMAKE_REQUIRED_LIBRARIES)
