if (SEARCH_DIR_UPNP)
	set (PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
	set (CMAKE_PREFIX_PATH ${SEARCH_DIR_UPNP})
endif()

# Skip find_package(UPNP CONFIG) because the system CMake config has broken include paths
# find_package (UPNP CONFIG)

# Always use pkg-config method to avoid the broken system CMake configuration
include (FindPkgConfig)
pkg_check_modules (LIBUPNP libupnp)
unset (CMAKE_PREFIX_PATH)

if (LIBUPNP_FOUND)
	# Validate and filter LIBUPNP_INCLUDE_DIRS to remove non-existent paths
	set(VALID_INCLUDE_DIRS "")
	foreach(INCLUDE_DIR ${LIBUPNP_INCLUDE_DIRS})
		if(EXISTS "${INCLUDE_DIR}")
			list(APPEND VALID_INCLUDE_DIRS "${INCLUDE_DIR}")
		endif()
	endforeach()

	# Only create the imported target if we have valid include directories
	if(VALID_INCLUDE_DIRS)
		add_library (UPNP::Shared SHARED IMPORTED)

		set_target_properties (UPNP::Shared PROPERTIES
			IMPORTED_LOCATION "${pkgcfg_lib_LIBUPNP_upnp}"
			INTERFACE_INCLUDE_DIRECTORIES "${VALID_INCLUDE_DIRS}"
			INTERFACE_LINK_LIBRARIES "${LIBUPNP_LIBRARIES}"
		)
	else()
		set (ENABLE_UPNP FALSE)
		message (STATUS "No valid UPnP include directories found, disabling UPnP support")
	endif()
elseif (NOT LIBUPNP_FOUND AND NOT DOWNLOAD_AND_BUILD_DEPS)
	set (ENABLE_UPNP FALSE)
	message (STATUS "libupnp not found, disabling UPnP support")
elseif (NOT LIBUPNP_FOUND AND DOWNLOAD_AND_BUILD_DEPS)
	CmDaB_install ("pupnp")
endif()