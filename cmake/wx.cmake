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
# Thin wrapper over CMake's stock FindwxWidgets module that exposes
# wxWidgets::{BASE,CORE,NET,ADV} as INTERFACE IMPORTED targets, letting
# callers continue to write `target_link_libraries(tgt PRIVATE wxWidgets::CORE)`.
# The component set is driven by wx_NEED_{BASE,GUI,NET,ADV} flags set up
# in cmake/options.cmake.
#
# wx 3.1.2+ merged the ADV component into CORE, so `adv` is not requested
# from find_package; wxWidgets::ADV is still created when asked for so that
# existing generator-expression references in src/ keep resolving.
#

set (_amule_wx_components)
set (_amule_wx_targets)

if (wx_NEED_BASE)
	list (APPEND _amule_wx_components base)
	list (APPEND _amule_wx_targets BASE)
endif()

if (wx_NEED_GUI)
	list (APPEND _amule_wx_components core)
	list (APPEND _amule_wx_targets CORE)
endif()

if (wx_NEED_NET)
	list (APPEND _amule_wx_components net)
	list (APPEND _amule_wx_targets NET)
endif()

if (wx_NEED_ADV)
	list (APPEND _amule_wx_targets ADV)
endif()

find_package (wxWidgets ${MIN_WX_VERSION} REQUIRED COMPONENTS ${_amule_wx_components})

# MSYS2's wx-config points at the unicode wx build but does not emit
# -DUNICODE / -D_UNICODE, so <wx/msw/winundef.h> ends up in a mixed
# ANSI/UNICODE state (LoadBitmapA vs LoadBitmapW, LPCTSTR vs LPCSTR
# mismatches). Stock FindwxWidgets just forwards wx-config's output, so
# the same gap appears here — propagate the defines manually on MinGW.
if (MINGW)
	list (APPEND wxWidgets_DEFINITIONS UNICODE _UNICODE)
endif()

# Stock FindwxWidgets returns a single combined LIBRARIES / INCLUDE_DIRS /
# DEFINITIONS set for all requested components; it does not expose per-
# component flags. All per-component INTERFACE targets forward the same
# set — duplicate link references are harmless.
foreach (_target IN LISTS _amule_wx_targets)
	if (NOT TARGET wxWidgets::${_target})
		add_library (wxWidgets::${_target} INTERFACE IMPORTED)

		target_link_libraries (wxWidgets::${_target}
			INTERFACE ${wxWidgets_LIBRARIES}
		)

		target_include_directories (wxWidgets::${_target}
			INTERFACE ${wxWidgets_INCLUDE_DIRS}
		)

		target_compile_definitions (wxWidgets::${_target}
			INTERFACE ${wxWidgets_DEFINITIONS}
		)
	endif()
endforeach()

unset (_amule_wx_components)
unset (_amule_wx_targets)
