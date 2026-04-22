if (SEARCH_DIR_UPNP)
	set (PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
	set (CMAKE_PREFIX_PATH ${SEARCH_DIR_UPNP})
endif()

# Some distro packages ship a broken UPNP.cmake that lists non-existent paths
# (e.g. /usr/COMPONENT, /mingw64/COMPONENT) in INTERFACE_INCLUDE_DIRECTORIES.
# Observed on Ubuntu 25.10 libupnp-dev 1.14.x and MSYS2 mingw-w64-pupnp 1.14.x.
# Detect this before loading the broken config by inspecting the targets file,
# and fall through to pkg-config if affected.
find_file (_upnp_cmake_targets "UPNP.cmake"
	PATH_SUFFIXES lib/cmake/UPNP lib64/cmake/UPNP
	              lib/x86_64-linux-gnu/cmake/UPNP
	              lib/aarch64-linux-gnu/cmake/UPNP)
if (_upnp_cmake_targets)
	file (READ "${_upnp_cmake_targets}" _upnp_cmake_content)
	if (_upnp_cmake_content MATCHES "INTERFACE_INCLUDE_DIRECTORIES.*COMPONENT")
		message (STATUS "Broken UPNP CMake config detected (bad include paths) — using pkg-config instead")
		set (_upnp_skip_config TRUE)
	endif()
	unset (_upnp_cmake_content)
endif()
unset (_upnp_cmake_targets CACHE)

if (NOT _upnp_skip_config)
	# MSYS2's pupnp UPNPConfig.cmake references Threads::Threads in its link
	# interface without calling find_package(Threads) itself — load it first
	# so the UPNP::Shared target resolves correctly.
	find_package (Threads REQUIRED)
	find_package (UPNP CONFIG)
endif()
unset (_upnp_skip_config)

if (UPNP_CONFIG)
	set (LIBUPNP_VERSION ${UPNP_VERSION})
endif()

if (NOT UPNP_CONFIG)
	include (FindPkgConfig)
	pkg_check_modules (LIBUPNP libupnp)
	unset (CMAKE_PREFIX_PATH)

	if (LIBUPNP_FOUND)
		if (MINGW)
			# On MinGW, SHARED IMPORTED without IMPORTED_IMPLIB makes
			# Debug-config lookups resolve to UPNP::Shared-NOTFOUND.
			add_library (UPNP::Shared UNKNOWN IMPORTED)
		else()
			add_library (UPNP::Shared SHARED IMPORTED)
		endif()

		set_target_properties (UPNP::Shared PROPERTIES
			IMPORTED_LOCATION "${pkgcfg_lib_LIBUPNP_upnp}"
			INTERFACE_INCLUDE_DIRECTORIES "${LIBUPNP_INCLUDE_DIRS}"
			INTERFACE_LINK_LIBRARIES "${LIBUPNP_LIBRARIES}"
		)
	elseif (NOT LIBUPNP_FOUND AND NOT DOWNLOAD_AND_BUILD_DEPS)
		set (ENABLE_UPNP FALSE)
		message (STATUS "lib-upnp not, disabling upnp")
	elseif (NOT LIBUPNP_FOUND AND DOWNLOAD_AND_BUILD_DEPS)
		CmDaB_install ("pupnp")
	endif()
endif()
