include (FindZLIB)

#At least on Win32 the includepath isn't set correctly sometimes
if (NOT EXISTS ${ZLIB_INCLUDE_DIR}/zlib.h)
	if (EXISTS ${ZLIB_INCLUDE_DIR}/include/zlib.h)
		set (${ZLIB_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR}/include)
		message (STATUS "zlib.h found in ${ZLIB_INCLUDE_DIR}")
	else()
		message (FATAL_ERROR "No useable zlib.h found")
	endif()
endif()
