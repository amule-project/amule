INCLUDE (CheckIncludeFile)
INCLUDE (CheckCSourceCompiles)

CHECK_INCLUDE_FILE (bfd.h HAVE_BFD)

IF (HAVE_BFD)
	FIND_LIBRARY (LIBBFD bfd)

	IF (NOT LIBBFD)
		MESSAGE (STATUS "No useable bfd-lib found, disabling support")
	ENDIF (NOT LIBBFD)		
ELSE (HAVE_BFD)
	MESSAGE (STATUS "bfd.h not found, disabling support")
ENDIF (HAVE_BFD)

FOREACH (CMAKE_REQUIRED_LIBRARIES "" "${LIBBFD}" "${LIBBFD};iberty" "${LIBBFD};dl" "${LIBBFD};iberty;dl" "${LIBBFD};${LIBINTL}" "${LIBBFD};iberty;${LIBINTL}" "${LIBBFD};iberty;dl;${LIBINBTL}")
	UNSET (BFD_COMPILE_TEST CACHE)
	CHECK_C_SOURCE_COMPILES ("#include <ansidecl.h>
		#include <bfd.h>

		int main()
		{
			char *dummy = bfd_errmsg(bfd_get_error());
		}"
		BFD_COMPILE_TEST
	)

	IF (BFD_COMPILE_TEST)
		SET (LIBBFD ${CMAKE_REQUIRED_LIBRARIES})
		BREAK()
	ENDIF (BFD_COMPILE_TEST)	
ENDFOREACH (CMAKE_REQUIRED_LIBRARIES "" "${LIBBFD}" "${LIBBFD};iberty" "${LIBBFD};dl" "${LIBBFD};iberty;dl" "${LIBBFD};${LIBINTL}" "${LIBBFD};iberty;${LIBINTL}" "${LIBBFD};iberty;dl;${LIBINBTL}")

