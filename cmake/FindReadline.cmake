include (CheckIncludeFile)
include (CheckFunctionExists)
include (FindPackageHandleStandardArgs)

macro (_ADD_LIBRARY_IF_EXISTS _LST _LIB)
	string (TOUPPER "${_LIB}" _LIBVAR)
	find_library (${_LIBVAR}_LIB "${_LIB}")

	if (${_LIBVAR}_LIB)
		list (APPEND ${_LST} ${${_LIBVAR}_LIB})
	endif (${_LIBVAR}_LIB)

	unset (${_LIBVAR}_LIB CACHE)
endmacro()

# Modified version of the library CHECK_FUNCTION_EXISTS
# This version will not produce any output, and the result variable is only
# set when the function is found
macro (_CHECK_FUNCTION_EXISTS FUNCTION VARIABLE)
	unset (_RESULT_VAR)
	try_compile (_RESULT_VAR
		${CMAKE_BINARY_DIR}
		${CMAKE_ROOT}/Modules/CheckFunctionExists.c
		CMAKE_FLAGS -DCOMPILE_DEFINITIONS:STRING="-DCHECK_FUNCTION_EXISTS=${FUNCTION}"
			"-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}"
		OUTPUT_VARIABLE OUTPUT)

	if (_RESULT_VAR)
		set (${VARIABLE} 1 CACHE INTERNAL "Have function ${FUNCTION}")
		file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
			"Determining if the function ${FUNCTION} exists passed with the following output:\n${OUTPUT}\n\n")
	else()
		file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
			"Determining if the function ${FUNCTION} exists failed with the following output:\n${OUTPUT}\n\n")
	endif()
endmacro()

if (HAVE_LIBREADLINE MATCHES ^HAVE_LIBREADLINE$)
	foreach (_maybe_readline_lib "readline" "edit" "editline")
		_ADD_LIBRARY_IF_EXISTS (_readline_libs ${_maybe_readline_lib})
	endforeach()

	foreach (_maybe_termcap_lib "termcap" "curses" "ncurses")
		_ADD_LIBRARY_IF_EXISTS (_termcap_libs ${_maybe_termcap_lib})
	endforeach()

	message (STATUS "Looking for readline")

	foreach (_readline_lib IN LISTS _readline_libs)
		set (CMAKE_REQUIRED_LIBRARIES "${_readline_lib}")
		_CHECK_FUNCTION_EXISTS (readline HAVE_LIBREADLINE)

		if (HAVE_LIBREADLINE)
			break()
		endif()

		foreach (_termcap_lib IN LISTS _termcap_libs)
			set (CMAKE_REQUIRED_LIBRARIES "${_readline_lib}" "${_termcap_lib}")
			_CHECK_FUNCTION_EXISTS (readline HAVE_LIBREADLINE)

			if (HAVE_LIBREADLINE)
				break()
			endif()
		endforeach()

		if (HAVE_LIBREADLINE)
			break()
		endif()
	endforeach()

	if (HAVE_LIBREADLINE)
		message (STATUS "Looking for readline - found")
	else (HAVE_LIBREADLINE)
		message (STATUS "Looking for readline - not found")
	endif (HAVE_LIBREADLINE)
endif()

if (HAVE_LIBREADLINE)
	set (READLINE_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}"
		CACHE STRING "Required link libraries for the readline library"
	)

	check_function_exists (add_history HAVE_READLINE_HISTORY)
	set (CMAKE_REQUIRED_LIBRARIES "")
endif()

check_include_file (stdio.h HAVE_STDIO_H)

# Stripped down and output modified version of the library CHECK_INCLUDE_FILES
macro (_CHECK_INCLUDE_FILES INCLUDE VARIABLE)
	if (${VARIABLE} MATCHES ^${VARIABLE}$)
		set (CMAKE_CONFIGURABLE_FILE_CONTENT "/* */\n")

		foreach (FILE ${INCLUDE})
			set (CMAKE_CONFIGURABLE_FILE_CONTENT "${CMAKE_CONFIGURABLE_FILE_CONTENT}#include <${FILE}>\n")
			set (_LAST_INCLUDE "${FILE}")
		endforeach()

		set (CMAKE_CONFIGURABLE_FILE_CONTENT "${CMAKE_CONFIGURABLE_FILE_CONTENT}\n\nint main(){return 0;}\n")
		configure_file ("${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in"
			"${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckIncludeFiles.c" @ONLY IMMEDIATE
		)

		message (STATUS "Looking for ${_LAST_INCLUDE}")
		try_compile (${VARIABLE}
			${CMAKE_BINARY_DIR}
			${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/CheckIncludeFiles.c
			OUTPUT_VARIABLE OUTPUT
		)

		if (${VARIABLE})
			message (STATUS "Looking for ${_LAST_INCLUDE} - found")
			set (${VARIABLE} 1 CACHE INTERNAL "Have ${_LAST_INCLUDE}")
			file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
				"Determining if ${_LAST_INCLUDE} exist passed with the following output:\n${OUTPUT}\n\n"
			)
		else()
			message (STATUS "Looking for ${_LAST_INCLUDE} - not found.")
			set (${VARIABLE} 0 CACHE INTERNAL "Have ${_LAST_INCLUDE}")
			file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
				"Determining if ${_LAST_INCLUDE} exist failed with the following output:\n${OUTPUT}\n"
				"Source:\n${CMAKE_CONFIGURABLE_FILE_CONTENT}\n"
			)
		endif()
	endif()
endmacro()

if (HAVE_STDIO_H)
	_check_include_files ("stdio.h;readline.h" HAVE_READLINE_H)

	if (HAVE_READLINE_H)
		_check_include_files ("stdio.h;readline.h;history.h" HAVE_HISTORY_H)
	else()
		_check_include_files ("stdio.h;readline/readline.h" HAVE_READLINE_READLINE_H)

		if (HAVE_READLINE_READLINE_H)
			_check_include_files ("stdio.h;readline/readline.h;readline/history.h" HAVE_READLINE_HISTORY_H)

			if (NOT HAVE_READLINE_HISTORY_H)
				_check_include_files ("stdio.h;readline/readline.h;history.h" HAVE_HISTORY_H)
			endif(NOT HAVE_READLINE_HISTORY_H)
		endif()
	endif()
endif()

find_package_handle_standard_args (readline DEFAULT_MSG READLINE_LIBRARIES)
