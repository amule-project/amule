cmake_minimum_required (VERSION 3.10...3.31)
project (amule)

set (MIN_BOOST_VERSION 1.70)
set (MIN_CRYPTOPP_VERSION 5.6)
set (MIN_GDLIB_VERSION 2.0.0)
set (MIN_WX_VERSION 3.2.0)
set (PACKAGE "amule")
set (PACKAGE_BUGREPORT "admin@amule.org")
set (PACKAGE_NAME "aMule")
set (PACKAGE_STRING "aMule SVN")
set (PACKAGE_TARNAME "amule")
set (PACKAGE_URL \"\")
set (PACKAGE_VERSION "SVN")
set (VERSION "GIT")
set (DEFAULT_BUILD_TYPE "Release")
set (RECONF_COMMAND ${CMAKE_COMMAND})
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set the possible values of build type for cmake-gui
if (CMAKE_CONFIGURATION_TYPES)
	set (CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE
		STRING "Semicolon separated list of supported configuration types, only supports debug and release, anything else will be ignored" FORCE
	)

	set_property (CACHE CMAKE_CONFIGURATION_TYPES PROPERTY STRINGS
		"Debug" "Release"
	)
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message (STATUS "Setting build type to '${DEFAULT_BUILD_TYPE}' as none was specified.")
	set (CMAKE_BUILD_TYPE "${DEFAULT_BUILD_TYPE}" CACHE
		STRING "Choose the type of build." FORCE
	)
endif()

# Try to use ccache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "ccache found: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    # Add other languages if needed, e.g., CMAKE_CUDA_COMPILER_LAUNCHER
else()
    message(STATUS "ccache not found.")
endif()

include (cmake/CmDaB.cmake)
include (cmake/manpage_install.cmake)
include (cmake/options.cmake)
include (cmake/search-dirs.cmake)

if (BUILD_AMULECMD OR BUILD_WEBSERVER)
	include (cmake/FindReadline.cmake)
endif()

if (BUILD_CAS)
	include (cmake/gdlib.cmake)
	include (cmake/getopt_long.cmake)
endif()

if (BUILD_WEBSERVER OR NEED_ZLIB)
	include (cmake/zlib.cmake)
endif()

if (BUILD_WEBSERVER)
	include (cmake/png.cmake)
endif()

include (cmake/boost.cmake)

if (ENABLE_IP2COUNTRY)
	include (cmake/ip2country.cmake)
endif()

if (ENABLE_NLS)
	include (cmake/nls.cmake)
endif()

if (ENABLE_UPNP)
	include (cmake/upnp.cmake)
endif()

if (NEED_GLIB_CHECK)
	include (cmake/glib21.cmake)
endif()

if (NEED_LIB_CRYPTO)
	include (cmake/cryptopp.cmake)
endif()

if (wx_NEEDED)
	include (cmake/wx.cmake)
endif()

#
# SVNDATE is consumed by config.h.cm (#define SVNDATE "...") and
# version.rc.in (Windows version resource), and ends up in the binary's
# `--version` output and the amuled startup banner.
#
# Two modes:
#   * Distro packagers building from a tarball (no .git) pass
#     -DSVNDATE="rev. foo" on the cmake command line.  That value lands
#     in the cache and is respected here unconditionally.
#   * Source builds derive the value from `git describe` on EVERY
#     configure so subsequent commits show up in the banner without a
#     full reconfigure.  The previous code wrapped this in
#     `if (NOT SVNDATE)` and stored the result with `CACHE STRING FORCE`,
#     which froze the value at the first configure forever — every
#     incremental rebuild thereafter embedded the stale revision string.
#
# CMAKE_CONFIGURE_DEPENDS makes the build system re-run cmake when the
# git state changes, so `cmake --build` alone is enough to pick up new
# commits in the banner.
#
# We need three files, not just .git/HEAD:
#   * .git/HEAD — touched on `git checkout <branch>` (symref retarget),
#     `git checkout --detach`, and explicit `git update-ref HEAD ...`.
#   * the branch-ref file (.git/refs/heads/<branch>) that HEAD points at —
#     touched on `git commit`, `git reset --hard`, `git pull --ff-only`,
#     and any other op that moves the branch tip.  Tracking only HEAD
#     misses these because HEAD is a symref text file, not the SHA itself.
#   * .git/packed-refs — touched by `git gc` / `git pack-refs`, which can
#     consolidate the branch ref file out of existence.  After a pack,
#     the SHA lives only in packed-refs.
#
if (NOT DEFINED CACHE{SVNDATE})
	find_package (Git)
	if (GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
		execute_process (
			COMMAND ${GIT_EXECUTABLE} describe
			OUTPUT_VARIABLE GIT_INFO_WC_REVISION
			OUTPUT_STRIP_TRAILING_WHITESPACE
			RESULT_VARIABLE GIT_INFO_RESULT
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		if (GIT_INFO_RESULT EQUAL 0 AND GIT_INFO_WC_REVISION)
			set (SVNDATE "rev. ${GIT_INFO_WC_REVISION}")
			message (STATUS "git revision ${SVNDATE} found")
		endif()
		# When HEAD sits exactly on a tag, override the SVN/GIT
		# placeholders set above with the tag name so artifact
		# filenames, the About dialog, the tray menu label, and the
		# Windows version resource all reflect the release. Tagless
		# builds keep "SVN"/"GIT" — the existing dev marker. The
		# AMULE_TAGGED_RELEASE flag is also forwarded to config.h so
		# ClientVersion.h can suppress the __SVN__ marker on releases.
		execute_process (
			COMMAND ${GIT_EXECUTABLE} describe --tags --exact-match HEAD
			OUTPUT_VARIABLE GIT_TAG
			OUTPUT_STRIP_TRAILING_WHITESPACE
			RESULT_VARIABLE GIT_TAG_RESULT
			ERROR_QUIET
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		if (GIT_TAG_RESULT EQUAL 0 AND GIT_TAG)
			set (VERSION "${GIT_TAG}")
			set (PACKAGE_VERSION "${GIT_TAG}")
			set (PACKAGE_STRING "aMule ${GIT_TAG}")
			set (AMULE_TAGGED_RELEASE 1)
			# Suppress the "(Snapshot: ...)" trailer that
			# MuleVersion.h's `#ifdef SVNDATE` gate appends to
			# GetMuleVersion(). On a tagged build the SVNDATE the
			# earlier git-describe block produced ("rev. <tag>")
			# would render as "Initialising aMule <tag> ...
			# (Snapshot: rev. <tag>)" — both redundant and
			# inaccurate, since a tagged release isn't a snapshot.
			# Pairs with the `#define` → `#cmakedefine` change in
			# config.h.cm so an unset SVNDATE yields `#undef
			# SVNDATE` instead of `#define SVNDATE ""`, which
			# otherwise leaves the empty parens in place.
			unset (SVNDATE)
			message (STATUS "building tagged release: ${PACKAGE_STRING}")
		endif()
	else()
		# No `.git` directory.  If we're sitting in an extracted git
		# archive — e.g. GitHub's "Download ZIP" button on a tag or
		# commit, or a tarball someone produced via `git archive` —
		# `.git_archival.txt` will exist with its `$Format:...$`
		# placeholders substituted to the actual SHA / tag / describe
		# output.  Parse it as a fallback so the resulting binary
		# self-identifies the same way a `.git`-present clone of the
		# same commit would.  See `.gitattributes` for the
		# `export-subst` binding that triggers the substitution.
		set (_archival_file "${CMAKE_SOURCE_DIR}/.git_archival.txt")
		if (EXISTS "${_archival_file}")
			file (READ "${_archival_file}" _archival_content)
			# Skip un-substituted templates: a working-tree copy
			# of the file (someone tar'd up `src/` without running
			# `git archive`) still contains literal `$Format:`
			# markers.  These are the signal that `git archive`
			# never touched the file — fall through to defaults.
			if (NOT _archival_content MATCHES "\\\$Format:")
				string (REGEX MATCH "describe-name:[ \t]*([^\r\n]+)" _ "${_archival_content}")
				set (_describe "${CMAKE_MATCH_1}")
				string (STRIP "${_describe}" _describe)
				if (_describe)
					# `git describe --tags` output:
					#   - exact-tag: just the tag name ("3.0.0", "v3.0.0")
					#   - off-tag:   "<tag>-<N>-g<sha>" ("2.3.3-296-ge3f87f77f")
					# Detect the off-tag form by the `-N-g<hex>`
					# trailer; otherwise treat as a tagged release.
					if (_describe MATCHES "-[0-9]+-g[0-9a-f]+$")
						set (SVNDATE "rev. ${_describe}")
						message (STATUS "git archival metadata: ${SVNDATE} found")
					else()
						set (VERSION "${_describe}")
						set (PACKAGE_VERSION "${_describe}")
						set (PACKAGE_STRING "aMule ${_describe}")
						set (AMULE_TAGGED_RELEASE 1)
						message (STATUS "git archival metadata: tagged release ${PACKAGE_STRING}")
					endif()
				endif()
			endif()
		endif()
	endif()
	set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
		"${CMAKE_SOURCE_DIR}/.git/HEAD")
	# Also depend on .git_archival.txt so that if it's swapped in
	# (e.g. someone re-extracts an archive, or hand-edits the file),
	# cmake re-runs and re-parses on the next build.
	if (EXISTS "${CMAKE_SOURCE_DIR}/.git_archival.txt")
		set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
			"${CMAKE_SOURCE_DIR}/.git_archival.txt")
	endif()
	# Resolve HEAD's symref target so we can also depend on the branch
	# ref file directly.  This catches commits/resets that move the
	# branch tip without retargeting HEAD.
	if (EXISTS "${CMAKE_SOURCE_DIR}/.git/HEAD")
		file (READ "${CMAKE_SOURCE_DIR}/.git/HEAD" _amule_head_contents LIMIT 200)
		string (STRIP "${_amule_head_contents}" _amule_head_contents)
		if (_amule_head_contents MATCHES "^ref: (.+)$")
			set (_amule_head_ref "${CMAKE_MATCH_1}")
			if (EXISTS "${CMAKE_SOURCE_DIR}/.git/${_amule_head_ref}")
				set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
					"${CMAKE_SOURCE_DIR}/.git/${_amule_head_ref}")
			endif()
		endif()
	endif()
	# Also depend on packed-refs in case the branch ref is packed
	# (loose ref file may not exist after `git gc`).
	if (EXISTS "${CMAKE_SOURCE_DIR}/.git/packed-refs")
		set_property (DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
			"${CMAKE_SOURCE_DIR}/.git/packed-refs")
	endif()
endif()

include (cmake/bfd.cmake)
include (cmake/atomic.cmake)

# glib-2.0 headers — needed for the Wayland wl_app_id / X11 WM_CLASS
# binding via g_set_prgname() (called from amule.cpp on the monolithic
# / daemon side, and from amule-remote-gui.cpp on the amulegui side
# once the remote-GUI fix lands as a separate PR). amulecmd and
# amuleweb don't need glib. Detect it here best-effort so the
# optional `if (GLIB_FOUND)` blocks downstream pick it up when
# present (e.g. amulegui's MuleTrayIcon SNI backend), and promote
# the missing-dep case to a hard configure-time error when a target
# that depends on it is being built (#562 — the silent QUIET
# surfaced as a confusing `glib.h: No such file` mid-compile for
# fresh-checkout users).
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	find_package (PkgConfig QUIET)
	if (PkgConfig_FOUND)
		pkg_check_modules (GLIB QUIET glib-2.0)
	endif()

	if ((BUILD_MONOLITHIC OR BUILD_DAEMON OR BUILD_REMOTEGUI) AND NOT GLIB_FOUND)
		message (FATAL_ERROR
			"glib-2.0 development headers not found, but they are "
			"required to build amule (monolithic), amuled, or "
			"amulegui — these targets call g_set_prgname() to bind "
			"the Wayland wl_app_id / X11 WM_CLASS to the .desktop "
			"filename, which needs <glib.h>. "
			"Install one of:\n"
			"  - libglib2.0-dev   (Debian / Ubuntu / Mint)\n"
			"  - glib2-devel      (Fedora / RHEL / Rocky)\n"
			"  - dev-libs/glib    (Gentoo)\n"
			"  - glib             (Arch / Manjaro)\n"
			"…then re-run cmake AFTER deleting the build directory "
			"(`rm -rf build`); pkg_check_modules only runs at "
			"configure time, so installing the package on top of a "
			"cached configure won't help. pkg-config itself is also "
			"required — install `pkg-config` if cmake didn't find it.")
	endif()
endif()

# Optional StatusNotifierItem (SNI) backend for the system-tray icon.
# wxGTK's wxTaskBarIcon talks the legacy GtkStatusIcon API, which
# GNOME Shell removed in 3.26 and modern wlroots compositors never
# implemented. Apps that need a visible tray icon on Ubuntu (with the
# AppIndicators extension), KDE Plasma, and Sway speak the SNI D-Bus
# protocol directly via libayatana-appindicator3. When the dev
# headers are present we build a SNI backend into MuleTrayIcon; on
# platforms where the library is missing (Windows, macOS, builds
# without the dep) we fall back to wxTaskBarIcon.
if (BUILD_MONOLITHIC AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
	find_package (PkgConfig QUIET)
	if (PkgConfig_FOUND)
		# `0.1` is the API version (never bumped, every release uses it),
		# not a library version pin. Try the Ayatana fork first since
		# that's the maintained one; fall back to Canonical's legacy
		# `appindicator3-0.1` for distros that haven't migrated yet.
		# The two are API-compatible at the headers/symbols level.
		pkg_check_modules (AYATANA_APPINDICATOR QUIET ayatana-appindicator3-0.1)
		if (NOT AYATANA_APPINDICATOR_FOUND)
			pkg_check_modules (AYATANA_APPINDICATOR QUIET appindicator3-0.1)
		endif()
		if (AYATANA_APPINDICATOR_FOUND)
			message (STATUS "AppIndicator3 found: ${AYATANA_APPINDICATOR_MODULE_NAME} ${AYATANA_APPINDICATOR_VERSION} — tray icon uses SNI backend")
			set (WITH_LIBAYATANA_APPINDICATOR ON)
		else()
			message (STATUS "AppIndicator3 not found (looked for ayatana-appindicator3-0.1 and appindicator3-0.1) — tray icon falls back to legacy GtkStatusIcon, invisible on modern GNOME/wlroots")
		endif()
	endif()
endif()

configure_file (
	config.h.cm
	config.h
)

if (WIN32)
	configure_file (
		version.rc.in
		version.rc
	)

	# Silences 80+ CI spam lines from MSYS2 `winsock2.h:15` which prints
	# `#warning Please include winsock2.h before windows.h` whenever
	# <windows.h> pulls in the legacy WinSock API first. Defining
	# WIN32_LEAN_AND_MEAN tells windows.h to skip that legacy include,
	# leaving <winsock2.h> to be pulled in cleanly by wx headers.
	# Upstream wx 3.3 does this inside wrapwin.h; wx 3.2 does not.
	add_compile_definitions(WIN32_LEAN_AND_MEAN)
endif()

if (BUILD_MONOLITHIC)
	# Files use the canonical AppStream / Flatpak / Wayland app id
	# `org.amule.aMule`. Wayland compositors bind windows to launcher
	# icons by matching wl_app_id (set via g_set_prgname() in amule's
	# OnInit) against the .desktop filename without extension; the id
	# also doubles as the AppStream component id and the Flatpak app id
	# so AppImage, Flatpak, and distro packages all resolve to the same
	# entity in software catalogs.
	install (FILES org.amule.aMule.desktop
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
	)
	install (FILES org.amule.aMule.png
		DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/128x128/apps"
	)
endif()

if (BUILD_REMOTEGUI)
	install (FILES org.amule.aMule.gui.desktop
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
	)
endif()

install (FILES org.amule.aMule.metainfo.xml
	DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
)

if (ENABLE_NLS)
	include (FindGettext)
	add_subdirectory (po)
endif()

add_subdirectory (docs)
add_subdirectory (src)

if (BUILD_TESTING)
	enable_testing()
	add_subdirectory (unittests)
endif()

message (STATUS "

	Configured aMule ${PACKAGE_VERSION}${SVN_REVISION} for '${CMAKE_SYSTEM}' on '${CMAKE_SYSTEM_PROCESSOR}'.

	aMule enabled options:

	**** aMule Core ****
	Prefix where aMule should be installed?				${CMAKE_PREFIX_PATH}
	Should aMule be compiled with i18n support?			${ENABLE_NLS}
	Which mode should aMule be compiled in?				${CMAKE_BUILD_TYPE}
	Should aMule be compiled with UPnP support?			${ENABLE_UPNP}
	Should aMule be compiled with IP2country support?		${ENABLE_IP2COUNTRY}
	Should aMule monolithic application be built?			${BUILD_MONOLITHIC}
	Should aMule daemon version be built?				${BUILD_DAEMON}
	Should aMule remote gui be built?				${BUILD_REMOTEGUI}

	**** aMule TextClient ****
	Should aMule Command Line Client be built?			${BUILD_AMULECMD}

	**** aMule WebServer ****
	Should aMule WebServer be built?				${BUILD_WEBSERVER}

	**** aMule ED2K Links Handler ****
	Should aMule ED2K Links Handler be built?			${BUILD_ED2K}

	**** aMuleLinkCreator ****
	Should aMuleLinkCreator GUI version (alc) be built?		${BUILD_ALC}
	Should aMuleLinkCreator for console (alcc) be built?		${BUILD_ALCC}

	**** aMule Statistics ****
	Should C aMule Statistics (CAS) be built?			${BUILD_CAS}
	Should aMule GUI Statistics (wxCas) be built?			${BUILD_WXCAS}"
)

message ("
	**** General Libraries and Tools ****
	Should aMule file viewer for console be built?			${BUILD_FILEVIEW}

	Libraries aMule will use to build:"
)

if (NEED_WX)
	message (STATUS "			wxWidgets				${WX_VERSION}")
endif()

message ("			boost				${Boost_VERSION}")

if (NEED_LIB_CRYPTO)
	message ("			crypto++			${CRYPTOPP_VERSION} in ${CRYPTOPP_INCLUDE_PREFIX}")
endif()

if (ENABLE_UPNP)
	message ("			libupnp				${LIBUPNP_VERSION}")
endif()

message ("			libintl				${ENABLE_NLS}")

if (ENABLE_IP2COUNTRY)
	message ("			libmaxminddb			${MAXMINDDB_LIB}")
endif()

if (BUILD_WEBSERVER)
	message ("			libpng				${PNG_VERSION_STRING}")
endif()

if (BUILD_CAS)
	message ("			libgd				${gdlib_VERSION}")
endif()

if (NEED_ZLIB)
	message ("			zlib				${ZLIB_VERSION_STRING}")
endif()

if (BUILD_MONOLITHIC AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
	if (WITH_LIBAYATANA_APPINDICATOR)
		message ("			tray-icon backend		StatusNotifierItem (libayatana-appindicator ${AYATANA_APPINDICATOR_VERSION})")
	else()
		message ("			tray-icon backend		legacy GtkStatusIcon (invisible on modern GNOME/wlroots — install libayatana-appindicator3-dev for SNI)")
	endif()
endif()
