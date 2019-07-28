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
# This file provides the CHECK_WX Macro which checks if wx is at least in a
# version defined by MIN_WX_VERSION. This check will go away, when cmake
# findWX module supports check of version
# 
# Furthermore it sets the LIBS, LIBDIR and DEFS needed for compilation.
# This can be controled by setting wx_NEED_BASE and wx_NEED_GUI vars.
#

INCLUDE (ExternalProject)

IF (NOT DOWNLOAD_AND_BUILD_DEPS)
	SET (wx_REQUIRED "REQUIRED")
ELSE (NOT DOWNLOAD_AND_BUILD_DEPS)
	UNSET (wx_REQUIRED)
ENDIF (NOT DOWNLOAD_AND_BUILD_DEPS)

IF (wx_NEED_BASE AND NOT wxWidgets_BASE_LIBRARIES)
	SET (wxWidgets_USE_LIBS base ${wx_REQUIRED})
	FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} )
	STRING (REGEX REPLACE "-L[^;]*;" ";" wxWidgets_BASE_LIBRARIES "${wxWidgets_LIBRARIES}")
	SET (wxWidgets_BASE_LIBRARIES ${wxWidgets_BASE_LIBRARIES} CACHE STRING "Libs to use when linking to wxBase" FORCE)
	SET (wxWidgets_BASE_LIBRARY_DIRS ${wxWidgets_LIBRARY_DIRS} CACHE STRING "Where to find the libs to use when linking to wxBase" FORCE)
	SET (wxWidgets_BASE_DEFS "${wxWidgets_DEFINITIONS};wxUSE_GUI=0;USE_WX_EXTENSIONS" CACHE STRING "Compilerflags to use when building with wxBase" FORCE)
	SET (wxWidgets_INCLUDE_DIRS ${wxWidgets_INCLUDE_DIRS} CACHE STRING "Where to find wx header files" FORCE)
	UNSET (wxWidgets_LIBRARIES)
	UNSET (wxWidgets_LIBRARY_DIRS)
ENDIF (wx_NEED_BASE AND NOT wxWidgets_BASE_LIBRARIES)

IF (wx_NEED_GUI AND NOT wxWidgets_GUI_LIBRARIES)
	SET (wxWidgets_USE_LIBS core)
	FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} ${wx_REQUIRED})
	STRING (REGEX REPLACE "-L[^;]*;" ";" wxWidgets_GUI_LIBRARIES "${wxWidgets_LIBRARIES}")
	SET (wxWidgets_GUI_LIBRARIES ${wxWidgets_GUI_LIBRARIES} CACHE STRING "Libs to use when linking to wx<GUI-type>" FORCE)
	SET (wxWidgets_GUI_LIBRARY_DIRS ${wxWidgets_LIBRARY_DIRS} CACHE STRING "Where to find the libs to use when linking to wx<GUI-type>" FORCE)
	SET (wxWidgets_GUI_DEFS "${wxWidgets_DEFINITIONS};USE_WX_EXTENSIONS" CACHE STRING "Compilerflags to use when building with wx<GUI-type>" FORCE)
	SET (wxWidgets_INCLUDE_DIRS ${wxWidgets_INCLUDE_DIRS} CACHE STRING "Where to find wx header files" FORCE)
	UNSET (wxWidgets_LIBRARIES)
	UNSET (wxWidgets_LIBRARY_DIRS)

	IF (WIN32)
		SET (wxWidgets_GUI_LIBRARIES "${wxWidgets_GUI_LIBRARIES};${wxWidgets_BASE_LIBRARIES}" CACHE STRING "Libs to use when linking to wx<GUI-type>" FORCE)
	ENDIF (WIN32)
ENDIF (wx_NEED_GUI AND NOT wxWidgets_GUI_LIBRARIES)

IF (wx_NEED_NET AND NOT wxWidgets_NET_LIBRARIES)
	SET (wxWidgets_USE_LIBS net)
	FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} ${wx_REQUIRED})
	STRING (REGEX REPLACE "-L[^;]*;" ";" wxWidgets_NET_LIBRARIES "${wxWidgets_LIBRARIES}")
	SET (wxWidgets_NET_LIBRARIES ${wxWidgets_NET_LIBRARIES} CACHE STRING "Libs to use when linking to wxNet" FORCE)
	SET (wxWidgets_NET_LIBRARY_DIRS ${wxWidgets_LIBRARY_DIRS} CACHE STRING "where to find the libs to use when linking to wxNet" FORCE)
	SET (wxWidgets_NET_DEFS ${wxWidgets_DEFINITIONS} CACHE STRING "Compilerflags to use when building with wxNet" FORCE)
	SET (wxWidgets_INCLUDE_DIRS ${wxWidgets_INCLUDE_DIRS} CACHE STRING "Where to find wx header files" FORCE)
	UNSET (wxWidgets_LIBRARIES)
	UNSET (wxWidgets_LIBRARY_DIRS)

	IF (WIN32)
		SET (wxWidgets_NET_LIBRARIES "${wxWidgets_NET_LIBRARIES};${wxWidgets_BASE_LIBRARIES}" CACHE STRING "Libs to use when linking to wxNet" FORCE)
	ENDIF (WIN32)
ENDIF (wx_NEED_NET AND NOT wxWidgets_NET_LIBRARIES)

IF (wx_NEED_ADV AND NOT wxWidgets_ADV_LIBRARIES)
	SET (wxWidgets_USE_LIBS adv)
	FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} ${wx_REQUIRED})
	STRING (REGEX REPLACE "-L[^;]*;" ";" wxWidgets_ADV_LIBRARIES "${wxWidgets_LIBRARIES}")
	SET (wxWidgets_ADV_LIBRARIES ${wxWidgets_ADV_LIBRARIES} CACHE STRING "Libs to use when linking to wxADV" FORCE)
	SET (wxWidgets_ADV_LIBRARY_DIRS ${wxWidgets_LIBRARY_DIRS} CACHE STRING "where to find the libs to use when linking to wxADV" FORCE)
	SET (wxWidgets_INCLUDE_DIRS ${wxWidgets_INCLUDE_DIRS} CACHE STRING "Where to find wx header files" FORCE)
	UNSET (wxWidgets_LIBRARIES)
	UNSET (wxWidgets_LIBRARY_DIRS)

	IF (WIN32)
		SET (wxWidgets_ADV_LIBRARIES "${wxWidgets_ADV_LIBRARIES};${wxWidgets_BASE_LIBRARIES}" CACHE STRING "Libs to use when linking to wxADV" FORCE)
	ENDIF (WIN32)
ENDIF (wx_NEED_ADV AND NOT wxWidgets_ADV_LIBRARIES)

IF (DOWNLOAD_AND_BUILD_DEPS AND NOT wxWidgets_CONFIG_EXECUTABLE AND NOT WX_BUILT)
	EXTERNALPROJECT_ADD (wxWidgets
		GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
		GIT_TAG v3.1.2
		GIT_PROGRESS TRUE
		CMAKE_ARGS "wxUSE_SOCKETS=TRUE"
		INSTALL_COMMAND ${CMAKE_COMMAND} ${amule_BINARY_DIR} -DwxWidgets_CONFIG_EXECUTABLE=<BINARY_DIR>/wx-config -DWX_BUILT=TRUE
		TEST_COMMAND ${CMAKE_COMMAND} --build ${amule_BINARY_DIR}
	)

	EXTERNALPROJECT_GET_PROPERTY (wxWidgets BINARY_DIR)
	INSTALL (CODE ${CMAKE_COMMAND} --build ${BINARY_DIR} --target install)
	SET (WAIT_FOR_WX TRUE)
ENDIF (DOWNLOAD_AND_BUILD_DEPS AND NOT wxWidgets_CONFIG_EXECUTABLE AND NOT WX_BUILT)
