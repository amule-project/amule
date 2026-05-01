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
	# This file is only included from the top-level CMakeLists.txt when
	# ENABLE_IP2COUNTRY is already true — i.e., the user explicitly asked
	# for the feature. Honour the user's intent: fail loudly instead of
	# silently downgrading to ENABLE_IP2COUNTRY=FALSE, which would mask
	# the missing dep behind a green build with the feature mysteriously
	# absent at runtime.
	if (NOT MAXMINDDB_INCLUDE_DIR)
		message (FATAL_ERROR "ENABLE_IP2COUNTRY=YES but maxminddb.h was not found. "
			"Install libmaxminddb headers (Debian/Ubuntu: libmaxminddb-dev, "
			"Fedora: libmaxminddb-devel, macOS Homebrew: libmaxminddb, "
			"MSYS2: mingw-w64-x86_64-libmaxminddb), or pass -DENABLE_IP2COUNTRY=NO "
			"to disable the feature.")
	else()
		message (FATAL_ERROR "ENABLE_IP2COUNTRY=YES but the libmaxminddb shared "
			"library was not found. Install the runtime package alongside the "
			"headers, or pass -DENABLE_IP2COUNTRY=NO to disable the feature.")
	endif()
endif()
