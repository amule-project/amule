if (GEOIP_INCLUDE_DIR)
	set (CMAKE_REQUIRED_INCLUDES ${GEOIP_INCLUDE_DIR})
endif()

if (NOT GEOIP_LIB)
	include (CheckIncludeFile)


	check_include_file (GeoIP.h GEOIP_H)

	if (GEOIP_H)
		find_library (GEOIP_LIB GeoIP)

		if (NOT GEOIP_LIB AND GEOIP_INCLUDE_DIR)
			find_library (GEOIP_LIB GeoIP
				PATHS ${GEOIP_INCLUDE_DIR}
			)
		endif()

		if (NOT GEOIP_LIB)
			set (ENABLE_IP2COUNTRY FALSE)
			message (STATUS "GeoIP lib not found, disabling support")
		else()
			message (STATUS "GeoIP found useable")
		endif()
	else()
		set (ENABLE_IP2COUNTRY FALSE)
		message (STATUS "GeoIP headers not found, disabling support")
	endif()
endif()

if (ENABLE_IP2COUNTRY)
	add_library (GeoIP::Shared UNKNOWN IMPORTED)

	set_target_properties (GeoIP::Shared PROPERTIES
		INTERFACE_COMPILE_DEFINITIONS "ENABLE_IP2COUNTRY"
		INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_REQUIRED_INCLUDES}"
		IMPORTED_LOCATION "${GEOIP_LIB}"
	)
endif()

unset (CMAKE_REQUIRED_INCLUDES)
