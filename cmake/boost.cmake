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
				message (STATUS "No useable boost headers found. Disabling support")
				set (ENABLE_BOOST FALSE)
			endif()
		endif()
	else()
		message (STATUS "No useable boost headers found. Disabling support")
		set (ENABLE_BOOST FALSE)
	endif()
endif()
