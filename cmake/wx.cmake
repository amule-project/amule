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
# This can be controlled by setting wx_NEED_BASE and wx_NEED_GUI vars.
#

include (CheckCXXSymbolExists)

if (wx_NEED_BASE)
	set (BASE "base")
	list (APPEND WX_COMPONENTS BASE)

	add_library (wxWidgets::BASE
		UNKNOWN
		IMPORTED
	)
endif()

if (wx_NEED_ADV)
	set (ADV "adv")
	list (APPEND WX_COMPONENTS ADV)

	add_library (wxWidgets::ADV
		UNKNOWN
		IMPORTED
	)
endif()

if (wx_NEED_GUI)
	set (CORE "core")
	list (APPEND WX_COMPONENTS CORE)

	add_library (wxWidgets::CORE
		UNKNOWN
		IMPORTED
	)
endif()

if (wx_NEED_NET)
	set (NET "net")
	list (APPEND WX_COMPONENTS NET)

	add_library (wxWidgets::NET
		UNKNOWN
		IMPORTED
	)
endif()

if (WX_COMPONENTS)
	foreach (COMPONENT ${WX_COMPONENTS})
		if (${COMPONENT} STREQUAL ADV AND wxWidgets_VERSION_STRING VERSION_GREATER_EQUAL 3.1.2 AND NOT WX_QUIET)
			message (STATUS "wx_Version 3.1.2 or newer detected. Disabling wx_ADV")
			continue()
		endif()

		if (NOT ${${COMPONENT}}_COMPLETE AND NOT (wxWidgets_${COMPONENT}_LIBRARY_RELEASE AND wxWidgets_${COMPONENT}_LIBRARY_DEBUG))
			message (STATUS "Searching for wx-${COMPONENT}")
			find_package (wxWidgets ${MIN_WX_VERSION} QUIET REQUIRED COMPONENTS ${${COMPONENT}})
			message(STATUS "Found usable wx-${COMPONENT}: ${wxWidgets_VERSION_STRING}")
		endif()

		if (WIN32)
			set_property (TARGET wxWidgets::${COMPONENT}
				PROPERTY IMPORTED_LOCATION_RELEASE ${WX_${${COMPONENT}}}
			)

			set_property (TARGET wxWidgets::${COMPONENT}
				PROPERTY IMPORTED_LOCATION_DEBUG ${WX_${${COMPONENT}}d}
			)

			set (wxWidgets_DEFINITIONS ${wxWidgets_DEFINITIONS} WXUSINGDLL)

			if (${COMPONENT} STREQUAL CORE)
				set (wxWidgets_DEFINITIONS ${wxWidgets_DEFINITIONS} wxUSE_GUI=1)
				set (CMAKE_REQUIRED_INCLUDES ${wxWidgets_INCLUDE_DIRS})

				if (NOT UNICODE_SUPPORT)
					unset (UNICODE_SUPPORT CACHE)

					CHECK_CXX_SYMBOL_EXISTS (wxUSE_UNICODE
						wx/setup.h
						UNICODE_SUPPORT
					)
				endif()

				unset (CMAKE_REQUIRED_INCLUDES)

				if (UNICODE_SUPPORT)
					set (wxWidgets_DEFINITIONS ${wxWidgets_DEFINITIONS} _UNICODE)
				endif()
			endif()

			set (wxWidgets_${COMPONENT}_LIBRARY_RELEASE ${WX_${${COMPONENT}}} CACHE STRING "Libs to use when linking to ${COMPONENT}" FORCE)
			set (wxWidgets_${COMPONENT}_LIBRARY_DEBUG ${WX_${${COMPONENT}}d} CACHE STRING "Libs to use when linking to ${COMPONENT}" FORCE)

			mark_as_advanced (wxWidgets_${COMPONENT}_LIBRARY_RELEASE
				wxWidgets_${COMPONENT}_LIBRARY_DEBUG
			)
		else()
			if (${${${COMPONENT}}_COMPLETE})
				set_property (TARGET wxWidgets::${COMPONENT} PROPERTY
					IMPORTED_LOCATION ${${${COMPONENT}}_LOC}
				)

				if (${${COMPONENT}}_DEPS)
					if ("Threads::Threads" IN_LIST ${${COMPONENT}}_DEPS)
						include (FindThreads)
					endif()

					target_link_libraries (wxWidgets::${COMPONENT}
						INTERFACE ${${${COMPONENT}}_DEPS}
					)
				endif()

				if (${${COMPONENT}}_INCS)
					set_property (TARGET wxWidgets::${COMPONENT} PROPERTY
						INTERFACE_INCLUDE_DIRECTORIES ${${${COMPONENT}}_INCS}
					)
				endif()

				if (${${COMPONENT}}_DEFS)
					set_property (TARGET wxWidgets::${COMPONENT} PROPERTY
						INTERFACE_COMPILE_DEFINITIONS ${${${COMPONENT}}_DEFS}
					)
				endif()
			else()
				foreach (LIB IN LISTS wxWidgets_LIBRARIES)
					if ("${LIB}" MATCHES "^-l(.*)$")
						if (${CMAKE_MATCH_1})
							list (APPEND ${COMPONENT}_DEPS wxWidgets::${${CMAKE_MATCH_1}})
							list (REMOVE_ITEM wxWidgets_LIBRARIES "${LIB}")

							foreach (entry IN LISTS wxWidgets_LIBRARIES)
								if (DEFINED ${${CMAKE_MATCH_1}}_DEPS AND "${entry}" IN_LIST "${${CMAKE_MATCH_1}}_DEPS")
									list (REMOVE_ITEM ${${CMAKE_MATCH_1}}_DEPS "${entry}")
									list (REMOVE_ITEM wxWidgets_LIBRARIES "${entry}")
								endif()

								if (DEFINED ${${CMAKE_MATCH_1}}_REMAINS AND "${entry}" IN_LIST "${${CMAKE_MATCH_1}}_REMAINS")
									list (REMOVE_ITEM ${${CMAKE_MATCH_1}}_REMAINS "${entry}")
									list (REMOVE_ITEM wxWidgets_LIBRARIES "${entry}")
								endif()
							endforeach()
						else()
							set (LIB_TO_SEARCH ${CMAKE_MATCH_1})

							find_library (${LIB_TO_SEARCH}_SEARCH
								${LIB_TO_SEARCH}
								PATHS ${wxWidgets_LIBRARY_DIRS}
							)

							if (${LIB_TO_SEARCH}_SEARCH)
								set (${${COMPONENT}}_LOC ${${LIB_TO_SEARCH}_SEARCH}
									CACHE
									INTERNAL
									"location of ${COMPONENT} lib"
									FORCE
								)

								set (${LIB_TO_SEARCH} ${COMPONENT})
								list (REMOVE_ITEM wxWidgets_LIBRARIES "${LIB}")
							endif()
						endif()
					endif()
				endforeach()

				foreach (LIB IN LISTS wxWidgets_LIBRARIES)
					if ("${LIB}" MATCHES "^-L.*")
						list (REMOVE_ITEM wxWidgets_LIBRARIES "${LIB}")
					elseif ("${LIB}" STREQUAL "")
						continue()
					else()
						if (${LIB} STREQUAL "-pthread")
							if (NOT TARGET Threads::Threads)
								include (FindThreads)
							endif()

							if (TARGET Threads::Threads)
								list (APPEND ${COMPONENT}_DEPS "Threads::Threads")
								list (REMOVE_ITEM wxWidgets_LIBRARIES "${LIB}")
							else()
								message (FATAL_ERROR "wxWidgets::${COMPONENT} needs threads, but it was not found")
							endif()
						else()
							message ("${LIB} nicht behandelt")
						endif()
					endif()
				endforeach()

				foreach (dep IN LISTS ${COMPONENT}_DEPS)
					get_target_property (int_deps
						${dep}
						INTERFACE_LINK_LIBRARIES
					)

					if (${int_deps} IN_LIST ${COMPONENT}_DEPS)
						list (REMOVE_ITEM ${COMPONENT}_DEPS ${int_deps})
					endif()

					get_target_property (int_incs
						${dep}
						INTERFACE_INCLUDE_DIRECTORIES
					)

					if (int_incs)
						foreach (inc IN LISTS int_incs)
							if (${inc} IN_LIST wxWidgets_INCLUDE_DIRS)
								list (REMOVE_ITEM wxWidgets_INCLUDE_DIRS ${int_incs})
							endif()
						endforeach()
					endif()

					get_target_property (int_defs
						${dep}
						INTERFACE_COMPILE_DEFINITIONS
					)

					if (int_defs)
						foreach (def IN LISTS int_defs)
							if (${def} IN_LIST wxWidgets_DEFINITIONS)
								list (REMOVE_ITEM wxWidgets_DEFINITIONS ${int_defs})
							endif()
						endforeach()
					endif()
				endforeach()

				set_property (TARGET wxWidgets::${COMPONENT} PROPERTY
					IMPORTED_LOCATION ${${${COMPONENT}}_LOC}
				)

				set (${${COMPONENT}}_DEPS ${${COMPONENT}_DEPS}
					CACHE
					INTERNAL
					"Deps of ${COMPONENT}"
					FORCE
				)

				target_link_libraries (wxWidgets::${COMPONENT}
					INTERFACE ${${COMPONENT}_DEPS}
				)

				set (${${COMPONENT}}_INCS ${wxWidgets_INCLUDE_DIRS}
					CACHE
					INTERNAL
					"Incs of ${COMPONENT}"
					FORCE
				)

				if (wxWidgets_INCLUDE_DIRS)
					set_property (TARGET wxWidgets::${COMPONENT} PROPERTY
						INTERFACE_INCLUDE_DIRECTORIES ${wxWidgets_INCLUDE_DIRS}
					)
				endif()

				set (${${COMPONENT}}_DEFS ${wxWidgets_DEFINITIONS}
					CACHE
					INTERNAL
					"Defs of ${COMPONENT}"
					FORCE
				)

				if (wxWidgets_DEFINITIONS)
					set_property (TARGET wxWidgets::${COMPONENT} PROPERTY
						INTERFACE_COMPILE_DEFINITIONS ${wxWidgets_DEFINITIONS}
					)
				endif()

				set (${${COMPONENT}}_COMPLETE TRUE
					CACHE
					INTERNAL
					"${COMPONENT} is complete"
					FORCE
				)
			endif()
		endif()
	endforeach()
endif()
