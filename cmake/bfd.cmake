if (NOT HAVE_BFD)
	include (CheckIncludeFile)
	include (CheckCSourceCompiles)

	check_include_file (bfd.h HAVE_BFD)

	if (HAVE_BFD)
		find_library (LIBBFD_TMP bfd)

		if (NOT LIBBFD_TMP)
			message (STATUS "No useable bfd-lib found, disabling support")
		else()
			foreach (CMAKE_REQUIRED_LIBRARIES
				"" "${LIBBFD_TMP}" "${LIBBFD_TMP};iberty" "${LIBBFD_TMP};dl" "${LIBBFD};iberty;dl" "${LIBBFD_TMP};${LIBINTL}" "${LIBBFD_TMP};iberty;${LIBINTL_TMP}" "${LIBBFD_TMP};iberty;dl;${LIBINBTL}"
			)
				unset (BFD_COMPILE_TEST CACHE)
				check_c_source_compiles ("#include <ansidecl.h>
					#include <bfd.h>

					int main()
					{
						const char *dummy = bfd_errmsg(bfd_get_error());
					}"
					BFD_COMPILE_TEST
				)

				if (BFD_COMPILE_TEST)
					set (BFD_LIBRARY ${CMAKE_REQUIRED_LIBRARIES} CACHE STRING "Lib to use when linking to bfd")
					unset (${CMAKE_REQUIRED_LIBRARIES})
					break()
				endif()	
			endforeach()
		endif()

		if (NOT BFD_LIBRARY)
			set (HAVE_BFD FALSE)
			message (STATUS "bfd.h found but can't link against it, disabling support")
		endif()
	else()
		message (STATUS "bfd.h not found, disabling support")
	endif()

	unset (LIBBFD_TMP CACHE)
endif (NOT HAVE_BFD)
