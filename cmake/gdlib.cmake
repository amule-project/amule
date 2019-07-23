#
# This file is part of the aMule Project.
#
# Copyright (c) 2011 Werner Mahr (Vollstrecker) <amule@vollstreckernet.de>
#
# Any parts of this program contributed by third-party developers are copyrighted
# by their respective authors.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
#
#
# This file uses the following variables:
#
#     MIN_GDLIB_VERSION -- to check if gdlib is available in a recent
#         version for usage in your project. If MIN_GDLIB_VERSION was
#         not set, no version-check is done.
#
# This file defines the folling variables
#
#    gdlib_FOUND -- TRUE if gdlib was found, and if version-check is done
#        if it is new enough, otherwise FALSE
#
#    gdlib_CONFIG_EXECUTABLE -- Contains the complete path to gdlib-config
#        executable
#
#    gdlib_VERSION -- If version-check is done, the version goes here
#
#    gdlib_LIB_DIR -- libdir reportet by gdlib-config
#
#    gdlib_INCLUDE_DIR -- include-dir reportet by gdlib-config
#
#    gdlib_LD_FLAGS -- ldflags reportet by gdlib-config
#
#    gdlib_LIBS -- libs reportet by gdlib-config
#
#    gdlib_CFLAGS -- cflags reportet by gdlib-config
#

IF (NOT gdlib_FOUND)
	IF (NOT WIN32)
		FIND_PACKAGE (PkgConfig REQUIRED)
		pkg_search_module (gdlib REQUIRED gdlib)
		MESSAGE (STATUS "gdlib version: ${gdlib_VERSION} -- OK")
	ENDIF(NOT WIN32)

	IF (NOT gdlib_FOUND)
		FIND_PROGRAM (gdlib_CONFIG_EXECUTABLE gdlib-config
			ONLY_CMAKE_FIND_ROOT_PATH
		)

		IF (gdlib_CONFIG_EXECUTABLE)
			EXECUTE_PROCESS (COMMAND ${gdlib_CONFIG_EXECUTABLE} --version
				OUTPUT_VARIABLE gdlib_VERSION
			)
			STRING (REGEX REPLACE "(\r?\n)+$" "" gdlib_VERSION "${gdlib_VERSION}")

			IF (${gdlib_VERSION} VERSION_LESS ${MIN_GDLIB_VERSION})
				MESSAGE (FATAL_ERROR "gdlib version ${gdlib_VERSION} -- too old")
			ELSE (${gdlib_VERSION} VERSION_LESS ${MIN_GDLIB_VERSION})
				MESSAGE (STATUS "gdlib version ${gdlib_VERSION} -- OK")
				SET (gdlib_FOUND TRUE)
				EXECUTE_PROCESS (COMMAND ${gdlib_CONFIG_EXECUTABLE} --libdir
					OUTPUT_VARIABLE gdlib_LIB_DIR
				)
				EXECUTE_PROCESS (COMMAND ${gdlib_CONFIG_EXECUTABLE} --includedir
					OUTPUT_VARIABLE gdlib_INCLUDE_DIR
				)
				EXECUTE_PROCESS (COMMAND ${gdlib_CONFIG_EXECUTABLE} --ldflags
					OUTPUT_VARIABLE gdlib_LDFLAGS
				)
				EXECUTE_PROCESS (COMMAND ${gdlib_CONFIG_EXECUTABLE} --libs
					OUTPUT_VARIABLE gdlib_LIBRARIES
				)
				EXECUTE_PROCESS (COMMAND ${gdlib_CONFIG_EXECUTABLE} --cflags
					OUTPUT_VARIABLE gdlib_CFLAGS
				)
			ENDIF (${gdlib_VERSION} VERSION_LESS ${MIN_GDLIB_VERSION})
		ENDIF (gdlib_CONFIG_EXECUTABLE)
	ENDIF (NOT gdlib_FOUND)

	IF (gdlib_FOUND)
		SET (gdlib_CFLAGS "${gdlib_CFLAGS} __GD__")
	ENDIF (gdlib_FOUND)
ENDIF (NOT gdlib_FOUND)
