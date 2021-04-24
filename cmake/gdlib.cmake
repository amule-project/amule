if (NOT WIN32)
	find_package (PkgConfig REQUIRED)

	pkg_search_module (gdlib REQUIRED
		IMPORTED_TARGET GLOBAL
		gdlib
	)

	set_property (TARGET PkgConfig::gdlib PROPERTY
		INTERFACE_COMPILE_DEFINITIONS __GD__
	)

	message (STATUS "gdlib version: ${gdlib_VERSION} -- OK")
endif()
