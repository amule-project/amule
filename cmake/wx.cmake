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

MACRO (CHECK_WX)
	SET (wxWidgets_USE_LIBS base)
	FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} REQUIRED)

	IF (wx_NEED_BASE)
		STRING (REGEX REPLACE "-L[^;]*;" ";" wxWidgets_BASE_LIBRARIES "${wxWidgets_LIBRARIES}")
		SET (wxWidgets_BASE_LIBRARY_DIRS ${wxWidgets_LIBRARY_DIRS})
		SET (wxWidgets_BASE_DEFS "${wxWidgets_DEFINITIONS};wxUSE_GUI=0;USE_WX_EXTENSIONS")
		UNSET (wxWidgets_LIBRARIES)
		UNSET (wxWidgets_LIBRARY_DIRS)
	ENDIF (wx_NEED_BASE)

	IF (wx_NEED_GUI)
		SET (wxWidgets_USE_LIBS core)
		FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} REQUIRED)
		STRING (REGEX REPLACE "-L[^;]*;" ";" wxWidgets_GUI_LIBRARIES "${wxWidgets_LIBRARIES}")
		SET (wxWidgets_GUI_LIBRARY_DIRS ${wxWidgets_LIBRARY_DIRS})
		SET (wxWidgets_GUI_DEFS "${wxWidgets_DEFINITIONS};USE_WX_EXTENSIONS")
		UNSET (wxWidgets_LIBRARIES)
		UNSET (wxWidgets_LIBRARY_DIRS)

		IF (WIN32)
			SET (wxWidgets_GUI_LIBRARIES "${wxWidgets_GUI_LIBRARIES};${wxWidgets_BASE_LIBRARIES}")
		ENDIF (WIN32)
	ENDIF (wx_NEED_GUI)

	IF (wx_NEED_NET)
		SET (wxWidgets_USE_LIBS net)
		FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} REQUIRED)
		STRING (REGEX REPLACE "-L[^;]*;" ";" wxWidgets_NET_LIBRARIES "${wxWidgets_LIBRARIES}")
		SET (wxWidgets_NET_LIBRARY_DIRS ${wxWidgets_LIBRARY_DIRS})
		SET (wxWidgets_NET_DEFS ${wxWidgets_DEFINITIONS})
		UNSET (wxWidgets_LIBRARIES)
		UNSET (wxWidgets_LIBRARY_DIRS)

		IF (WIN32)
			SET (wxWidgets_NET_LIBRARIES "${wxWidgets_NET_LIBRARIES};${wxWidgets_BASE_LIBRARIES}")
		ENDIF (WIN32)
	ENDIF (wx_NEED_NET)

	IF (wx_NEED_ADV)
		SET (wxWidgets_USE_LIBS adv)
		FIND_PACKAGE (wxWidgets ${MIN_WX_VERSION} REQUIRED)
		STRING (REGEX REPLACE "-L[^;]*;" ";" wxWidgets_ADV_LIBRARIES "${wxWidgets_LIBRARIES}")
		SET (wxWidgets_ADV_LIBRARY_DIRS ${wxWidgets_LIBRARY_DIRS})
		UNSET (wxWidgets_LIBRARIES)
		UNSET (wxWidgets_LIBRARY_DIRS)

		IF (WIN32)
			SET (wxWidgets_ADV_LIBRARIES "${wxWidgets_ADV_LIBRARIES};${wxWidgets_BASE_LIBRARIES}")
		ENDIF (WIN32)
	ENDIF (wx_NEED_ADV)

	INCLUDE_DIRECTORIES (${wxWidgets_INCLUDE_DIRS})

ENDMACRO (CHECK_WX)
