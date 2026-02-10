# MaxMind DB implementation for IP2Country
find_package(maxminddb QUIET)
if (NOT maxminddb_FOUND)
    # Try alternative spelling
    find_package(MaxMindDB QUIET)
endif()

if (maxminddb_FOUND)
    message(STATUS "MaxMind DB found: ${maxminddb_VERSION}")
else()
    # Try to find manually
    find_path(MAXMINDDB_INCLUDE_DIR maxminddb.h)
    find_library(MAXMINDDB_LIBRARY NAMES maxminddb)

    if (MAXMINDDB_INCLUDE_DIR AND MAXMINDDB_LIBRARY)
        set(maxminddb_FOUND TRUE)
        set(maxminddb_INCLUDE_DIRS ${MAXMINDDB_INCLUDE_DIR})
        set(maxminddb_LIBRARIES ${MAXMINDDB_LIBRARY})
        message(STATUS "MaxMind DB found manually")
    else()
        if (ENABLE_IP2COUNTRY)
            message(FATAL_ERROR "**************************************************")
            message(FATAL_ERROR "libmaxminddb not found but ENABLE_IP2COUNTRY is enabled")
            message(FATAL_ERROR "Please install: sudo apt install libmaxminddb-dev")
            message(FATAL_ERROR "**************************************************")
        else()
            message(STATUS "MaxMind DB not found - GeoIP/country flags will be disabled")
            message(STATUS "To enable, install: sudo apt install libmaxminddb-dev")
            set(maxminddb_FOUND FALSE)
        endif()
    endif()
endif()

if (ENABLE_IP2COUNTRY)
    if (maxminddb_FOUND)
        # MaxMind DB implementation
        add_library(maxminddb::maxminddb UNKNOWN IMPORTED)

        set_target_properties(maxminddb::maxminddb PROPERTIES
            INTERFACE_COMPILE_DEFINITIONS "ENABLE_IP2COUNTRY"
            INTERFACE_INCLUDE_DIRECTORIES "${maxminddb_INCLUDE_DIRS}"
            IMPORTED_LOCATION "${maxminddb_LIBRARIES}"
        )
        message(STATUS "Using MaxMind DB implementation")
    else()
        # No MaxMind DB library found, disable support
        set(ENABLE_IP2COUNTRY FALSE)
        message(STATUS "MaxMind DB not found, disabling IP2Country support")
    endif()
endif()
