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

if (NOT gdlib_FOUND)
	if (NOT WIN32)
		find_package (PkgConfig REQUIRED)

		pkg_search_module (gdlib REQUIRED
			IMPORTED_TARGET GLOBAL
			gdlib
		)

		message (STATUS "gdlib version: ${gdlib_VERSION} -- OK")
	endif()

	if (NOT gdlib_FOUND)
		find_program (gdlib_CONFIG_EXECUTABLE gdlib-config
			ONLY_CMAKE_FIND_ROOT_PATH
		)

		if (gdlib_CONFIG_EXECUTABLE)
			execute_process (COMMAND ${gdlib_CONFIG_EXECUTABLE} --version
				OUTPUT_VARIABLE gdlib_VERSION
			)

			string (REGEX REPLACE "(\r?\n)+$" "" gdlib_VERSION "${gdlib_VERSION}")

			if (${gdlib_VERSION} VERSION_LESS ${MIN_GDLIB_VERSION})
				message (FATAL_ERROR "gdlib version ${gdlib_VERSION} -- too old")
			else()
				message (STATUS "gdlib version ${gdlib_VERSION} -- OK")
				set (gdlib_FOUND TRUE)

				execute_process (COMMAND ${gdlib_CONFIG_EXECUTABLE} --libdir
					OUTPUT_VARIABLE gdlib_LIB_DIR
				)

				execute_process (COMMAND ${gdlib_CONFIG_EXECUTABLE} --includedir
					OUTPUT_VARIABLE gdlib_INCLUDE_DIR
				)

				execute_process (COMMAND ${gdlib_CONFIG_EXECUTABLE} --ldflags
					OUTPUT_VARIABLE gdlib_LDFLAGS
				)

				execute_process (COMMAND ${gdlib_CONFIG_EXECUTABLE} --libs
					OUTPUT_VARIABLE gdlib_LIBRARIES
				)

				execute_process (COMMAND ${gdlib_CONFIG_EXECUTABLE} --cflags
					OUTPUT_VARIABLE gdlib_CFLAGS
				)
			endif()
		endif()
	endif()

	if (gdlib_FOUND)
		set (gdlib_CFLAGS "${gdlib_CFLAGS} __GD__")
	endif()
endif()
