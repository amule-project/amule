find_path (MAXMINDDB_INCLUDE_DIR maxminddb.h)
find_library (MAXMINDDB_LIB maxminddb)

if (MAXMINDDB_INCLUDE_DIR AND MAXMINDDB_LIB)
	message (STATUS "libmaxminddb found, IP2Country support enabled")

	add_library (MaxMindDB::Shared UNKNOWN IMPORTED)
	set_target_properties (MaxMindDB::Shared PROPERTIES
		INTERFACE_COMPILE_DEFINITIONS "ENABLE_IP2COUNTRY"
		INTERFACE_INCLUDE_DIRECTORIES "${MAXMINDDB_INCLUDE_DIR}"
		IMPORTED_LOCATION "${MAXMINDDB_LIB}"
	)
else()
	set (ENABLE_IP2COUNTRY FALSE)
	if (NOT MAXMINDDB_INCLUDE_DIR)
		message (STATUS "maxminddb.h not found, IP2Country support disabled")
	else()
		message (STATUS "libmaxminddb not found, IP2Country support disabled")
	endif()
endif()
