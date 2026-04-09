if (SEARCH_DIR_UPNP)
	set (PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
	set (CMAKE_PREFIX_PATH ${SEARCH_DIR_UPNP})
endif()

# Some distro packages (e.g. Ubuntu 25.10 libupnp-dev 1.14.x) ship a broken
# UPNP.cmake that lists non-existent paths (e.g. /usr/COMPONENT) in
# INTERFACE_INCLUDE_DIRECTORIES. Detect this before loading the broken config
# by inspecting the targets file, and fall through to pkg-config if affected.
find_file (_upnp_cmake_targets "UPNP.cmake"
	PATH_SUFFIXES cmake/UPNP
	NO_DEFAULT_PATH
	PATHS /usr/lib /usr/lib64 /usr/local/lib
	      /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu)
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
	find_package (UPNP CONFIG)
endif()
unset (_upnp_skip_config)

if (NOT UPNP_CONFIG)
	include (FindPkgConfig)
	pkg_check_modules (LIBUPNP libupnp)
	unset (CMAKE_PREFIX_PATH)

	if (LIBUPNP_FOUND)
		add_library (UPNP::Shared SHARED IMPORTED)

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
