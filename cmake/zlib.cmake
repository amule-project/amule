INCLUDE (FindZLIB)

#At least on Win32 the includepath isn't set correctly sometimes
IF (NOT EXISTS ${ZLIB_INCLUDE_DIR}/zlib.h)
	IF (EXISTS ${ZLIB_INCLUDE_DIR}/include/zlib.h)
		SET (${ZLIB_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR}/include)
		MESSAGE (STATUS "zlib.h found in ${ZLIB_INCLUDE_DIR}")
	ELSE (EXISTS ${ZLIB_INCLUDE_DIR}/include/zlib.h)
		MESSAGE (FATAL_ERROR "No useable zlib.h found")
	ENDIF (EXISTS ${ZLIB_INCLUDE_DIR}/include/zlib.h)
ENDIF (NOT EXISTS ${ZLIB_INCLUDE_DIR}/zlib.h)

