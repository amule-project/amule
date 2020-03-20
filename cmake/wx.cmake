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

IF (BUILT_WX)
	SET (BUILT_WX ${BUILT_WX} CACHE BOOL "Remeber that wx was built" FORCE)

	EXTERNALPROJECT_ADD (WX
		GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
		GIT_TAG v3.1.2
		CONFIGURE_COMMAND ""
		BUILD_COMMAND ""
		INSTALL_COMMAND ""
	)

	EXTERNALPROJECT_GET_PROPERTY (WX SOURCE_DIR)

	INSTALL (CODE
		"EXECUTE_PROCESS (
			COMMAND ${CMAKE_MAKE_PROGRAM} clean
			WORKING_DIRECTORY ${SOURCE_DIR}
		)

		EXECUTE_PROCESS (
			COMMAND ./configure --prefix=${CMAKE_INSTALL_PREFIX}
			WORKING_DIRECTORY ${SOURCE_DIR}
		)

		EXECUTE_PROCESS (
			COMMAND ${CMAKE_MAKE_PROGRAM} install
			WORKING_DIRECTORY ${SOURCE_DIR}
		)"
	)

	SET (wxWidgets_CONFIG_EXECUTABLE ${SOURCE_DIR}/wx-config CACHE FILEPATH "Location of wxWidgets library configuration provider binary (wx-config)." FORCE)

IF (NOT DOWNLOAD_AND_BUILD_DEPS)
	SET (wx_REQUIRED "REQUIRED")
ELSE (NOT DOWNLOAD_AND_BUILD_DEPS)
	UNSET (wx_REQUIRED)
ENDIF (NOT DOWNLOAD_AND_BUILD_DEPS)

IF (wx_NEED_BASE AND NOT wxWidgets_BASE_LIBRARIES)
	FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} COMPONENTS base)
	STRING (REGEX REPLACE "-L[^;]*;" ";" wxWidgets_BASE_LIBRARIES "${wxWidgets_LIBRARIES}")
	SET (wxWidgets_BASE_LIBRARIES ${wxWidgets_BASE_LIBRARIES} CACHE STRING "Libs to use when linking to wxBase" FORCE)
	SET (wxWidgets_BASE_LIBRARY_DIRS ${wxWidgets_LIBRARY_DIRS} CACHE STRING "Where to find the libs to use when linking to wxBase" FORCE)
	SET (wxWidgets_BASE_DEFS "${wxWidgets_DEFINITIONS};wxUSE_GUI=0;USE_WX_EXTENSIONS" CACHE STRING "Compilerflags to use when building with wxBase" FORCE)
	SET (wxWidgets_INCLUDE_DIRS ${wxWidgets_INCLUDE_DIRS} CACHE STRING "Where to find wx header files" FORCE)
	UNSET (wxWidgets_LIBRARIES)
	UNSET (wxWidgets_LIBRARY_DIRS)
ENDIF (wx_NEED_BASE AND NOT wxWidgets_BASE_LIBRARIES)

IF (wx_NEED_GUI AND NOT wxWidgets_GUI_LIBRARIES)
	FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} ${wx_REQUIRED} COMPONENTS core)
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
	FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} ${wx_REQUIRED} COMPONENTS net)
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
	FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} ${wx_REQUIRED} COMPONENTS adv)
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
	IF (WAIT_FOR_BOOST)
		SET (WX_DEPS BOOST-test)
	ENDIF (WAIT_FOR_BOOST)

	IF (WAIT_FOR_UPNP)
		LIST (APPEND WX_DEPS UPNP-test)
	ENDIF (WAIT_FOR_UPNP)

	IF (WAIT_FOR_CRYPTO)
		LIST (APPEND WX_DEPS CRYPTO-test)
	ENDIF (WAIT_FOR_CRYPTO)

	EXTERNALPROJECT_ADD (wxWidgets
		DEPENDS ${WX_DEPS}
		GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
		GIT_TAG v3.1.2
		GIT_PROGRESS TRUE
		PATCH_COMMAND ./autogen.sh
		BUILD_IN_SOURCE TRUE
		CONFIGURE_COMMAND ./configure --prefix=$(pwd) 
		BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
		INSTALL_COMMAND ${CMAKE_COMMAND} ${amule_BINARY_DIR} -DWX_BUILT=TRUE
	)

	LIST (APPEND EXTERNAL_DEPS WX)
	SET (RECONF_COMMAND ${RECONF_COMMAND} -DBUILT_WX=TRUE)
