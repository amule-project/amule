if (NOT GEOIP_LIBRARY)
	include (CheckIncludeFile)

	if (GEOIP_INCLUDE_DIR)
		set (CMAKE_REQUIRED_INCLUDES ${GEOIP_INCLUDE_DIR})
	endif()

	check_include_file (GeoIP.h GEOIP_H)

	if (GEOIP_H)
		find_library (GEOIP_LIBRARY GeoIP)

		if (NOT GEOIP_LIBRARY AND GEOIP_INCLUDE_DIR)
			find_library (GEOIP_LIBRARY GeoIP
				PATHS ${GEOIP_INCLUDE_DIR}
			)
		endif()

		if (NOT GEOIP_LIBRARY)
			set (ENABLE_IP2COUNTRY FALSE)
			message (STATUS "GeoIP lib not found, disabling support")
		else()
			message (STATUS "GeoIP found useable")
		endif()
	else()
		set (ENABLE_IP2COUNTRY FALSE)
		message (STATUS "GeoIP headers not found, disabling support")
	endif()

	unset (CMAKE_REQUIRED_INCLUDES)

	if (GEOIP_INCLUDE_DIR)
		set (CMAKE_REQUIRED_INCLUDES)
	endif()
endif()
