add_library (CRYPTOPP::CRYPTOPP
	UNKNOWN
	IMPORTED
)

set (CRYPTOPP_SEARCH_PREFIXES "cryptopp" "crypto++")

if (NOT CRYPTOPP_INCLUDE_PREFIX)
	unset (CRYPT_SEARCH CACHE)
	check_include_file_cxx (cryptlib.h CRYPT_SEARCH)

	if (CRYPT_SEARCH)
		set (CRYPTOPP_INCLUDE_PREFIX "" CACHE STRING "cryptopp include prefix" FORCE)
	else()
		foreach (PREFIX ${CRYPTOPP_SEARCH_PREFIXES})
			unset (CRYPT_SEARCH CACHE)
			check_include_file_cxx (${PREFIX}/cryptlib.h CRYPT_SEARCH)

			if (CRYPT_SEARCH)
				message (STATUS "cryptopp prefix found: ${PREFIX}")
				set (CRYPTOPP_INCLUDE_PREFIX ${PREFIX} CACHE STRING "cryptopp include prefix" FORCE)
				break()
			endif()
		endforeach()
	endif()
endif()

if (NOT CRYPTOPP_INCLUDE_PREFIX)
	MESSAGE (FATAL_ERROR "cryptlib.h not found")
endif()

if (WIN32)
	if (NOT CRYPTOPP_LIBRARY_DEBUG)
		unset (CRYPTOPP_LIBRARY_DEBUG CACHE)

		find_library (CRYPTOPP_LIBRARY_DEBUG
			NAMES crypto++d cryptlibd cryptoppd
			PATHS ${CRYPTOPP_LIB_SEARCH_PATH}
		)

		if (CRYPTOPP_LIBRARY_DEBUG)
			message (STATUS "Found debug-libcrypto++ in ${CRYPTOPP_LIBRARY_DEBUG}")
		endif()
	endif()

	if (CRYPTOPP_LIBRARY_DEBUG)
		set_property (TARGET CRYPTOPP::CRYPTOPP
			PROPERTY IMPORTED_LOCATION_DEBUG ${CRYPTOPP_LIBRARY_DEBUG}
		)
	else()
		set (CRYPTO_COMPLETE FALSE)
	endif()

	if (NOT CRYPTOPP_LIBRARY_RELEASE)
		unset (CRYPTOPP_LIBRARY_RELEASE CACHE)

		find_library (CRYPTOPP_LIBRARY_RELEASE
			NAMES crypto++ cryptlib cryptopp
			PATHS ${CRYPTOPP_LIB_SEARCH_PATH}
		)

		if (CRYPTOPP_LIBRARY_RELEASE)
			message (STATUS "Found release-libcrypto++ in ${CRYPTOPP_LIBRARY_RELEASE}")
		endif()
	endif (NOT CRYPTOPP_LIBRARY_RELEASE)

	if (CRYPTOPP_LIBRARY_RELEASE)
		set_property (TARGET CRYPTOPP::CRYPTOPP
			PROPERTY IMPORTED_LOCATION_RELEASE ${CRYPTOPP_LIBRARY_RELEASE}
		)
	else()
		set (CRYPTO_COMPLETE FALSE)
	endif()
else()
	if (NOT CRYPTOPP_LIBRARY)
		unset (CRYPTOPP_LIBRARY CACHE)

		find_library (CRYPTOPP_LIBRARY
			NAMES crypto++ cryptlib cryptopp
			PATHS ${CRYPTOPP_LIB_SEARCH_PATH}
		)

		if (CRYPTOPP_LIBRARY)
			message (STATUS "Found libcrypto++ in ${CRYPTOPP_LIBRARY}")
		endif()
	endif()

	if (CRYPTOPP_LIBRARY)
		set_property (TARGET CRYPTOPP::CRYPTOPP
			PROPERTY IMPORTED_LOCATION ${CRYPTOPP_LIBRARY}
		)
	else()
		set (CRYPTO_COMPLETE FALSE)
	endif()
endif()

if (NOT CRYPTOPP_CONFIG_SEARCH)
	unset (CRYPTOPP_CONFIG_SEARCH CACHE)

	check_include_file_cxx (${CRYPTOPP_INCLUDE_PREFIX}/config.h
		CRYPTOPP_CONFIG_SEARCH
	)
endif()

if (NOT CRYPTOPP_CONFIG_FILE)
	if (CRYPTOPP_CONFIG_SEARCH)
		if (CRYPTOPP_INCLUDE_DIR)
			set (CRYPTOPP_CONFIG_FILE ${CRYPTOPP_INCLUDE_DIR}/${CRYPTOPP_INCLUDE_PREFIX}/config.h
				CACHE FILEPATH "cryptopp config.h" FORCE
			)

			set_target_properties (CRYPTOPP::CRYPTOPP PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${CRYPTOPP_INCLUDE_DIR}"
			)
		else()
			set (CRYPTOPP_CONFIG_FILE ${CRYPTOPP_INCLUDE_PREFIX}/config.h
				CACHE FILEPATH "cryptopp config.h" FORCE
			)
		endif()
	else()
		unset(CRYPTOPP_CONFIG_SEARCH)
	endif()

	unset (CMAKE_REQUIRED_INCLUDES)
endif()

if (NOT CRYPTOPP_CONFIG_FILE)
		MESSAGE (FATAL_ERROR "crypto++ config.h not found")
endif()

if (NOT CRYPTOPP_VERSION)# AND CRYPTO_COMPLETE)
	set (CMAKE_CONFIGURABLE_FILE_CONTENT
		"#include <${CRYPTOPP_CONFIG_FILE}>\n
		#include <stdio.h>\n
		int main(){\n
			printf (\"%d\", CRYPTOPP_VERSION);\n
		}\n"
	)

	configure_file ("${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in"
		"${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckCryptoppVersion.cxx" @ONLY IMMEDIATE
	)

	try_run (RUNRESULT
		COMPILERESULT
		${CMAKE_BINARY_DIR}
		${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckCryptoppVersion.cxx
		RUN_OUTPUT_VARIABLE CRYPTOPP_VERSION
	)

	string (REGEX REPLACE "([0-9])([0-9])([0-9])" "\\1.\\2.\\3" CRYPTOPP_VERSION "${CRYPTOPP_VERSION}")

	if (${CRYPTOPP_VERSION} VERSION_LESS ${MIN_CRYPTOPP_VERSION})
		message (FATAL_ERROR "crypto++ version ${CRYPTOPP_VERSION} is too old")
	else()
		MESSAGE (STATUS "crypto++ version ${CRYPTOPP_VERSION} -- OK")
		set (CRYPTOPP_CONFIG_FILE ${CRYPTOPP_CONFIG_FILE} CACHE STRING "Path to config.h of crypto-lib" FORCE)
		set (CRYPTOPP_VERSION ${CRYPTOPP_VERSION} CACHE STRING "Version of cryptopp" FORCE)
	endif()
endif()
