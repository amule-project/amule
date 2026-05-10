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


# CHTTPDownloadThread (HTTPDownload.{h,cpp}) is built directly on top of
# wxWebRequest / wxWebSession / wxWebRequestEvent.  Those classes are gated
# on wxUSE_WEBREQUEST in <wx/webrequest.h>, which itself collapses to 0
# when wx was built without a backend — libcurl on Linux/*BSD, WinHTTP on
# Windows, NSURLSession on macOS.  Distro CI never sees this because the
# stock wx packages include the backend, but hand-rolled wx builds and
# Gentoo wxGTK with USE="-curl" don't.  Surface that here so the failure
# is one configure-time line instead of a wall of "wxWebRequest does not
# name a type" cascades during the build.
if (wx_NEED_NET)
	include (CheckCXXSourceCompiles)
	include (CMakePushCheckState)

	# wxWidgets_DEFINITIONS is a list of bare names (e.g. WXUSINGDLL,
	# __WXGTK__, _FILE_OFFSET_BITS=64); CMAKE_REQUIRED_DEFINITIONS wants
	# each entry already prefixed with -D.  Without that, wx/defs.h trips
	# its own "No Target! You should use wx-config program for compilation
	# flags!" #error and the test fails for the wrong reason.
	set (_amule_wx_required_defs)
	foreach (_def IN LISTS wxWidgets_DEFINITIONS)
		list (APPEND _amule_wx_required_defs "-D${_def}")
	endforeach()

	# Compile-only — we are inspecting a preprocessor symbol from
	# wx/setup.h, not exercising any wx symbols, so save the link step
	# (which would need CMAKE_REQUIRED_LIBRARIES wired up).
	cmake_push_check_state (RESET)
	set (CMAKE_REQUIRED_INCLUDES ${wxWidgets_INCLUDE_DIRS})
	set (CMAKE_REQUIRED_DEFINITIONS ${_amule_wx_required_defs})
	set (CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

	check_cxx_source_compiles ("
		#include <wx/webrequest.h>
		#if !wxUSE_WEBREQUEST
		#error wxUSE_WEBREQUEST is not enabled
		#endif
		int probe() { return 0; }
	" amule_HAVE_WXWEBREQUEST)

	cmake_pop_check_state()
	unset (_amule_wx_required_defs)

	if (NOT amule_HAVE_WXWEBREQUEST)
		message (FATAL_ERROR
			"wxWidgets was found but wxUSE_WEBREQUEST is 0 in this build.\n"
			"aMule's HTTP download path requires wxWebRequest, which "
			"needs a backend at wx-build time:\n"
			"  - Linux / *BSD: libcurl   (rebuild wx with --with-libcurl,\n"
			"                            or on Gentoo emerge net-libs/wxGTK\n"
			"                            with USE=\"curl\")\n"
			"  - Windows     : WinHTTP   (always present on supported releases)\n"
			"  - macOS       : NSURLSession (always present)\n"
			"Then re-run cmake.")
	endif()

	# Optional: when libcurl headers are present on the build host, we
	# enable CHTTPDownloadThread's CURLOPT_NOSIGNAL + CURLOPT_CONNECTTIMEOUT_MS
	# tuning via wxWebRequest::GetNativeHandle(). This requires both
	# wx itself to be built with the libcurl backend (wxUSE_WEBREQUEST_CURL=1
	# at runtime) AND <curl/curl.h> at our build time. Probe is
	# unconditional — wx may include the curl backend on platforms
	# whose default is something else (macOS Homebrew wxwidgets builds
	# with libcurl alongside NSURLSession; same for some MSYS2 wx
	# packages). Soft-fails to a STATUS line; the patch silently
	# no-ops when libcurl-dev is absent.
	find_package (CURL QUIET)
	if (CURL_FOUND)
		set (amule_HAVE_LIBCURL 1 CACHE INTERNAL "libcurl headers available")
		message (STATUS "libcurl headers found (${CURL_VERSION_STRING}) — CHTTPDownloadThread CURLOPT tuning enabled")
	else()
		message (STATUS
			"libcurl headers not found — CHTTPDownloadThread will skip "
			"CURLOPT tuning (NOSIGNAL/CONNECTTIMEOUT). Install "
			"libcurl4-openssl-dev (Debian/Ubuntu) or libcurl-devel "
			"(Fedora) to enable.")
	endif()
endif()
