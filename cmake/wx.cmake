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

INCLUDE (CheckCXXSymbolExists)
INCLUDE (ExternalProject)

IF (BUILT_WX)
	SET (BUILT_WX ${BUILT_WX} CACHE BOOL "Remeber that wx was built" FORCE)

	EXTERNALPROJECT_ADD (WX
		GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
		GIT_TAG v3.1.2
		CONFIGURE_COMMAND ""
		BUILD_COMMAND ""
		INSTALL_COMMAND ""
		EXCLUDE_FROM_ALL TRUE
	)

	EXTERNALPROJECT_GET_PROPERTY (WX SOURCE_DIR)

	IF (WIN32)
		SET (wxWidgets_ROOT_DIR ${CMAKE_INSTALL_PREFIX})
	ELSE (WIN32)
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
	ENDIF (WIN32)
ENDIF (BUILT_WX)

IF (NOT DOWNLOAD_AND_BUILD_DEPS)
	SET (wx_REQUIRED "REQUIRED")
ELSE (NOT DOWNLOAD_AND_BUILD_DEPS)
	UNSET (wx_REQUIRED)
ENDIF (NOT DOWNLOAD_AND_BUILD_DEPS)

IF (wx_NEED_BASE)
	SET (BASE "base")
	LIST (APPEND WX_COMPONENTS BASE)

	ADD_LIBRARY (wxWidgets::BASE
		UNKNOWN
		IMPORTED
	)
ENDIF (wx_NEED_BASE)

IF (wx_NEED_ADV)
	SET (ADV "adv")
	LIST (APPEND WX_COMPONENTS ADV)

	ADD_LIBRARY (wxWidgets::ADV
		UNKNOWN
		IMPORTED
	)
ENDIF (wx_NEED_ADV)

IF (wx_NEED_GUI)
	SET (CORE "core")
	LIST (APPEND WX_COMPONENTS CORE)

	ADD_LIBRARY (wxWidgets::CORE
		UNKNOWN
		IMPORTED
	)
ENDIF (wx_NEED_GUI)

IF (wx_NEED_NET)
	SET (NET "net")
	LIST (APPEND WX_COMPONENTS NET)

	ADD_LIBRARY (wxWidgets::NET
		UNKNOWN
		IMPORTED
	)
ENDIF (wx_NEED_NET)

IF (DOWNLOAD_AND_BUILD_DEPS)
	UNSET (WX_REQUIRED)
ELSE (DOWNLOAD_AND_BUILD_DEPS)
	SET (WX_REQUIRED "REQUIRED")
ENDIF (DOWNLOAD_AND_BUILD_DEPS)

IF (wxWidgets_BASE_DEFINITIONS)
	SET (WX_QUIET QUIET)
ENDIF (wxWidgets_BASE_DEFINITIONS)

IF (WX_COMPONENTS)
	FOREACH (COMPONENT ${WX_COMPONENTS})
		IF (${COMPONENT} STREQUAL ADV AND wxWidgets_VERSION_STRING VERSION_GREATER_EQUAL 3.1.2 AND NOT WX_QUIET)
			MESSAGE (STATUS "wx_Version 3.1.2 or newer detected. Disabling wx_ADV")
			CONTINUE()
		ENDIF (${COMPONENT} STREQUAL ADV AND wxWidgets_VERSION_STRING VERSION_GREATER_EQUAL 3.1.2 AND NOT WX_QUIET)

		IF (NOT wxWidgets_${COMPONENT}_LIBRARY AND NOT (wxWidgets_${COMPONENT}_LIBRARY_RELEASE AND wxWidgets_${COMPONENT}_LIBRARY_DEBUG))
			FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} ${WX_QUIET} ${WX_REQUIRED} COMPONENTS ${${COMPONENT}})
		ENDIF (NOT wxWidgets_${COMPONENT}_LIBRARY AND NOT (wxWidgets_${COMPONENT}_LIBRARY_RELEASE AND wxWidgets_${COMPONENT}_LIBRARY_DEBUG))

		IF (WIN32)
			SET_PROPERTY (TARGET wxWidgets::${COMPONENT}
							PROPERTY IMPORTED_LOCATION_RELEASE ${WX_${${COMPONENT}}}
			)

			SET_PROPERTY (TARGET wxWidgets::${COMPONENT}
							PROPERTY IMPORTED_LOCATION_DEBUG ${WX_${${COMPONENT}}d}
			)

			SET (wxWidgets_DEFINITIONS ${wxWidgets_DEFINITIONS} WXUSINGDLL)

			IF (${COMPONENT} STREQUAL CORE)
				SET (wxWidgets_DEFINITIONS ${wxWidgets_DEFINITIONS} wxUSE_GUI=1)
				SET (CMAKE_REQUIRED_INCLUDES ${wxWidgets_INCLUDE_DIRS})

				IF (NOT UNICODE_SUPPORT)
					UNSET (UNICODE_SUPPORT CACHE)

					CHECK_CXX_SYMBOL_EXISTS (wxUSE_UNICODE
						wx/setup.h
						UNICODE_SUPPORT
					)
				ENDIF (NOT UNICODE_SUPPORT)

				UNSET (CMAKE_REQUIRED_INCLUDES)

				IF (UNICODE_SUPPORT)
					SET (wxWidgets_DEFINITIONS ${wxWidgets_DEFINITIONS} _UNICODE)
				ENDIF (UNICODE_SUPPORT)
			ENDIF (${COMPONENT} STREQUAL CORE)

			SET (wxWidgets_${COMPONENT}_LIBRARY_RELEASE ${WX_${${COMPONENT}}} CACHE STRING "Libs to use when linking to ${COMPONENT}" FORCE)
			SET (wxWidgets_${COMPONENT}_LIBRARY_DEBUG ${WX_${${COMPONENT}}d} CACHE STRING "Libs to use when linking to ${COMPONENT}" FORCE)

			MARK_AS_ADVANCED (wxWidgets_${COMPONENT}_LIBRARY_RELEASE
				wxWidgets_${COMPONENT}_LIBRARY_DEBUG
			)
		ELSE (WIN32)
			FOREACH (LIB IN LISTS wxWidgets_LIBRARIES)
				IF ("${LIB}" MATCHES "^-l(.*)$")
					SET (LIB_TO_SEARCH ${CMAKE_MATCH_1})

					FIND_LIBRARY (${LIB_TO_SEARCH}_SEARCH
						${LIB_TO_SEARCH}
						PATHS ${wxWidgets_LIBRARY_DIRS}
					)

					IF (${LIB_TO_SEARCH}_SEARCH AND ${${LIB_TO_SEARCH}_SEARCH} MATCHES ${${COMPONENT}})
						SET_PROPERTY (TARGET wxWidgets::${COMPONENT}
							PROPERTY IMPORTED_LOCATION ${${LIB_TO_SEARCH}_SEARCH}
						)
					ELSE (${LIB_TO_SEARCH}_SEARCH AND ${${LIB_TO_SEARCH}_SEARCH} MATCHES ${${COMPONENT}})
						FOREACH (TGT ${WX_COMPONENTS})
							IF (${${LIB_TO_SEARCH}_SEARCH} MATCHES ${${TGT}} AND TARGET wxWidgets::${TGT})
								TARGET_LINK_LIBRARIES (wxWidgets::${COMPONENT}
									INTERFACE wxWidgets::${TGT}
								)
							ELSEIF (NOT TARGET wxWidgets::${TGT})
								MESSAGE (FATAL_ERROR "Tried to add dependency for wxWidgets::${TGT} but didn't find a target wxWidgets::${COMPONENT}")
							ENDIF (${${LIB_TO_SEARCH}_SEARCH} MATCHES ${${TGT}} AND TARGET wxWidgets::${TGT})
						ENDFOREACH (TGT ${WX_COMPONENTS})
					ENDIF (${LIB_TO_SEARCH}_SEARCH AND ${${LIB_TO_SEARCH}_SEARCH} MATCHES ${${COMPONENT}})
				ENDIF ("${LIB}" MATCHES "^-l(.*)$")
			ENDFOREACH (LIB IN LISTS wxWidgets_LIBRARIES)

			SET (wxWidgets_${COMPONENT}_LIBRARY ${wxWidgets_${COMPONENT}_LIBRARY} CACHE STRING "Libs to use when linking to ${COMPONENT}" FORCE)

			MARK_AS_ADVANCED (wxWidgets_${COMPONENT_NAME}_LIBRARY
				wxWidgets_${COMPONENT_NAME}_DEFINITIONS
			)
		ENDIF (WIN32)

		SET_TARGET_PROPERTIES (wxWidgets::${COMPONENT} PROPERTIES
			INTERFACE_COMPILE_DEFINITIONS "${wxWidgets_DEFINITIONS};$<$<CONFIG:Debug>:__WXDEBUG__>"
			INTERFACE_INCLUDE_DIRECTORIES "${wxWidgets_INCLUDE_DIRS}"
		)

		UNSET (wxWidgets_DEFINITIONS)
	ENDFOREACH (COMPONENT ${WX_COMPONENTS})

	IF (wxWidgets_VERSION_STRING VERSION_LESS 3.1.2)
		TARGET_LINK_LIBRARIES (wxWidgets::ADV
			INTERFACE wxWidgets::BASE
		)
	ENDIF (wxWidgets_VERSION_STRING VERSION_LESS 3.1.2)

	TARGET_LINK_LIBRARIES (wxWidgets::CORE
		INTERFACE wxWidgets::BASE
	)
	TARGET_LINK_LIBRARIES (wxWidgets::NET
		INTERFACE wxWidgets::BASE
	)

	SET (wxWidgets_INCLUDE_DIRS ${wxWidgets_INCLUDE_DIRS} CACHE STRING "Where to find wx header files" FORCE)

	MARK_AS_ADVANCED (wxWidgets_INCLUDE_DIRS
		wxWidgets_BASE_DEFINITIONS
	)
ENDIF (WX_COMPONENTS)

IF (DOWNLOAD_AND_BUILD_DEPS AND NOT wxWidgets_FOUND AND NOT BUILT_WX)
	IF (WIN32)
		EXTERNALPROJECT_ADD (WX
			GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
			GIT_TAG v3.1.2
			GIT_PROGRESS TRUE
			BUILD_IN_SOURCE FALSE
			CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DwxUSE_IPV6=OFF -DwxBUILD_COMPATIBILITY=2.8
			BUILD_COMMAND ${CMAKE_COMMAND} --build . --config Debug
			COMMAND ${CMAKE_COMMAND} --build . --config Release
			INSTALL_COMMAND ${CMAKE_COMMAND} --install . --config Debug
			COMMAND ${CMAKE_COMMAND} --install . --config Release
		)
	ELSE (WIN32)
		EXTERNALPROJECT_ADD (WX
			GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
			GIT_TAG v3.1.2
			GIT_PROGRESS TRUE
			BUILD_IN_SOURCE TRUE
			CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
			CONFIGURE_COMMAND ./autogen.sh
			BUILD_COMMAND ./configure --enable-debug --prefix=<SOURCE_DIR>
			INSTALL_COMMAND ${CMAKE_MAKE_PROGRAM}
		)
	ENDIF (WIN32)


	LIST (APPEND EXTERNAL_DEPS WX)
	SET (RECONF_COMMAND ${RECONF_COMMAND} -DBUILT_WX=TRUE)
ENDIF (DOWNLOAD_AND_BUILD_DEPS AND NOT wxWidgets_FOUND AND NOT BUILT_WX)
