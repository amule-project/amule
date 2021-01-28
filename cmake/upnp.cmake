if (SEARCH_DIR_UPNP)
	set (PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
	set (CMAKE_PREFIX_PATH ${SEARCH_DIR_UPNP})
endif()

find_package (UPNP CONFIG)

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
