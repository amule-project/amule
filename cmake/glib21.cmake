include (CheckFunctionExists)
include (CheckIncludeFile)
include (CheckIncludeFileCXX)

if (BUILD_MONOLITHIC OR BUILD_DAEMON)
	check_function_exists (fallocate HAVE_FALLOCATE)
	check_function_exists (getrlimit HAVE_GETRLIMIT)
	check_function_exists (setrlimit HAVE_SETRLIMIT)
	check_include_file (fcntl.h HAVE_FCNTL_H)
	check_include_file (sys/resource.h HAVE_SYS_RESOURCE_H)
	check_include_file (sys/statvfs.h HAVE_SYS_STATVFS_H)

	set (TEST_APP "#include <features.h>
		#ifdef __GNU_LIBRARY__
			#if (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 1) || (__GLIBC__ > 2)
				Lucky GNU user
			#endif
		#endif"
	)

	execute_process (COMMAND echo ${TEST_APP}
		COMMAND ${CMAKE_C_COMPILER} -E -xc -
		OUTPUT_VARIABLE GLIB_TEST_OUTPUT
	)

	string (REGEX MATCH "Lucky GNU user" MATCH "${GLIB_TEST_OUTPUT}")

	if (${MATCH})
		set (__GLIBC__ TRUE)
		message (STATUS "glibc -- found")
	endif()

	check_function_exists (posix_fallocate HAVE_POSIX_FALLOCATE)
endif()

if (BUILD_DAEMON)
	check_include_file (sys/select.h HAVE_SYS_SELECT_H)
	check_include_file (sys/time.h HAVE_SYS_TIME_H)
	check_include_file (sys/wait.h HAVE_SYS_WAIT_H)
	check_include_file (unistd.h HAVE_UNISTD_H)
endif()

if (BUILD_DAEMON OR BUILD_WEBSERVER)
	check_include_file (sys/types.h HAVE_SYS_TYPES_H)
endif()

if (BUILD_DAEMON OR BUILD_WEBSERVER OR NEED_LIB_MULECOMMON)
	include (CheckTypeSize) #Sets also HAVE_SYS_TYPES_H, HAVE_STDINT_H, and HAVE_STDDEF_H
	check_type_size (int INTSIZE)
endif()

if (NEED_LIB_MULEAPPCORE)
	check_include_file (errno.h HAVE_ERRNO_H)
	check_include_file (float.h HAVE_FLOAT_H)
	check_include_file (signal.h HAVE_SIGNAL_H)
	check_include_file (stdarg.h HAVE_STDARG_H)
	check_include_file (stdlib.h HAVE_STDLIB_H)
	check_include_file (string.h HAVE_STRING_H)

	if (HAVE_STDLIB_H)
		set (CMAKE_REQUIRED_INCLUDES stdlib.h)
		check_function_exists (free HAVE_FREE)
		unset (CMAKE_REQUIRED_INCLUDES)
	endif()

	if (HAVE_FREE AND HAVE_FLOAT_H AND HAVE_STDARG_H AND HAVE_STRING_H)
		set (STDC_HEADERS TRUE)
	endif()

	if (ENABLE_MMAP)
		check_include_file (sys/mman.h HAVE_SYS_MMAN_H)

		if (HAVE_SYS_MMAN_H)
			check_function_exists (munmap HAVE_MUNMAP)

			if (HAVE_MUNMAP)
				check_function_exists (sysconf HAVE_SYSCONF)

				if (HAVE_SYSCONF AND STDC_HEADERS)
					try_run (PS_RUN_RESULT PS_COMPILE_RESULT
						${CMAKE_BINARY_DIR}
						${amule_SOURCE_DIR}/cmake/mmap-test.cpp
						RUN_OUTPUT_VARIABLE PS_OUTPUT
					)

					if (PS_RUN_RESULT EQUAL 0)
						message (STATUS "_SC_PAGESIZE found")
						set (HAVE__SC_PAGESIZE TRUE)
					else()
						message (STATUS "_SC_PAGESIZE not defined, mmap support is disabled")
					endif()
				else()
					message (STATUS "sysconf function not found, mmap support is disabled")
				endif()
			else()
				message (STATUS "munmap function not found, mmap support is disabled")
			endif()
		else()
			message (STATUS "sys/mman.h wasn't found, mmap support is disabled")
		endif()
	endif()
endif()

if (NEED_LIB_MULECOMMON)
	check_include_file_cxx (cxxabi.h HAVE_CXXABI)
	check_include_file (execinfo.h HAVE_EXECINFO)
	check_include_file (inttypes.h HAVE_INTTYPES_H)

	if (HAVE_INTTYPES_H AND HAVE_SYS_TYPES_H)
		set (TEST_APP "#include <sys/types.h>
			#include <inttypes.h>"
		)

		EXECUTE_PROCESS (COMMAND echo ${TEST_APP}
			COMMAND ${CMAKE_C_COMPILER} -c -xc -
			ERROR_VARIABLE INTTYPES_SYSTYPES_TEST_ERRORS
		)

		if (INTTYPES_SYSTYPES_TEST_ERRORS)
			set (HAVE_INTTYPES_H FALSE)
		else()
			set (TEST_APP "#include <sys/types.h>
				#include <inttypes.h>
				uintmax_t i = (uintmax_t) -1\;"
			)

			EXECUTE_PROCESS (COMMAND echo ${TEST_APP}
				COMMAND ${CMAKE_C_COMPILER} -c -xc -
				ERROR_VARIABLE INTTYPES_SYSTYPES_UINTMAX_TEST_ERRORS
			)

			if (NOT INTTYPES_SYSTYPES_UINTMAX_TEST_ERRORS)
				set (HAVE_INTTYPES_H_WITH_UINTMAX TRUE)
			endif()
		endif()
	endif()

	if (HAVE_INTTYPES_H)
		set (TEST_APP "#include <inttypes.h>
			#ifdef PRId32
			char *p = PRId32\;
			#endif"
		)

		EXECUTE_PROCESS (COMMAND echo ${TEST_APP}
			COMMAND ${CMAKE_C_COMPILER} -c -xc -
			ERROR_VARIABLE INTTYPES_BROKEN_PRI_TEST_ERRORS
		)

		if (INTTYPES_BROKEN_PRI_TEST_ERRORS)
			set (PRI_MACROS_BROKEN TRUE)
		endif()
	endif()

	check_function_exists (strerror_r HAVE_STRERROR_R)

	if (HAVE_STRERROR_R)
		set (TEST_APP "int main ()
			{
				char buf[100]\;
				char x = *strerror_r (0, buf, sizeof buf)\;
			}"
		)

		EXECUTE_PROCESS (COMMAND echo ${TEST_APP}
			COMMAND ${CMAKE_C_COMPILER} -E -xc -
			OUTPUT_VARIABLE STR_ERROR_CHAR_P_OUTPUT
			ERROR_VARIABLE STR_ERROR_CHAR_P_TEST
		)

		if (STR_ERROR_CHAR_P_TEST)
			set (STRERROR_R_CHAR_P TRUE)
			message (STATUS "strerror_r returns char*")
		endif()
	endif()
endif()
