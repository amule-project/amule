find_package(Boost CONFIG REQUIRED)

if (NOT ASIO_SOCKETS)
	include (CheckIncludeFiles)

	if (WIN32)
		set (Boost_USE_STATIC_LIBS TRUE)
	endif()

	if (Boost_FOUND)
		set (CMAKE_REQUIRED_INCLUDES ${Boost_INCLUDE_DIRS})

		if (MINGW)
			# check_include_files on MinGW does a link step, and boost::asio
			# pulls in WSAStartup/WSACleanup from ws2_32 — passing the lib
			# via CMAKE_REQUIRED_LIBRARIES isn't reliably honoured with this
			# CMake + CheckIncludeFiles combination, so the link fails and
			# ASIO_SOCKETS gets silently disabled.  find_package(Boost CONFIG
			# REQUIRED) already verified boost is usable — skip the redundant
			# check on MinGW only.  MSVC keeps the original check.
			set (ASIO_SOCKETS TRUE)
		else()
			if (WIN32)
				set (CMAKE_REQUIRED_FLAGS " -DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB -DBOOST_SYSTEM_NO_LIB -DBOOST_ERROR_CODE_HEADER_ONLY")
			else()
				set (CMAKE_REQUIRED_FLAGS "-DBOOST_ERROR_CODE_HEADER_ONLY")
			endif()
			check_include_files ("boost/system/error_code.hpp;boost/asio.hpp" ASIO_SOCKETS LANGUAGE CXX)
		endif (MINGW)

		if (ASIO_SOCKETS)
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
		else()
			if (NOT DOWNLOAD_AND_BUILD_DEPS)
				# This file is only included from the top-level
				# CMakeLists.txt when ENABLE_BOOST is true — i.e. the user
				# explicitly asked for the feature and is not opting into
				# the download-and-build fallback. Honour the user's
				# intent: fail loudly instead of silently downgrading to
				# ENABLE_BOOST=FALSE, which would mask the missing headers
				# behind a green build with the asio sockets path
				# mysteriously absent.
				message (FATAL_ERROR "ENABLE_BOOST=YES but boost::asio "
					"headers were not usable. find_package(Boost) found "
					"a Boost install but check_include_files for "
					"boost/system/error_code.hpp + boost/asio.hpp "
					"failed to compile. Install full Boost development "
					"headers (Debian/Ubuntu: libboost-dev, Fedora: "
					"boost-devel, macOS Homebrew: boost, MSYS2: "
					"mingw-w64-x86_64-boost), or pass -DENABLE_BOOST=NO "
					"to fall back to wxWidgets sockets, or pass "
					"-DDOWNLOAD_AND_BUILD_DEPS=YES to have CMake build "
					"Boost from source.")
			endif()
		endif()
	else()
		# Defensive branch: find_package(Boost CONFIG REQUIRED) at the
		# top of this file already errors out when Boost is absent, so
		# this else() is practically unreachable. Keep a FATAL_ERROR
		# rather than a silent disable so that any future edit which
		# drops the REQUIRED keyword surfaces the bug instead of
		# producing a green build with no boost support.
		message (FATAL_ERROR "ENABLE_BOOST=YES but Boost_FOUND is false. "
			"This shouldn't happen — find_package(Boost CONFIG REQUIRED) "
			"at the top of cmake/boost.cmake should have errored out "
			"already. Please report this configuration as a bug.")
	endif()
endif()
