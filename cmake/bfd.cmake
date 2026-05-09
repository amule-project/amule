if (NOT HAVE_BFD)
	include (CheckIncludeFile)
	include (CheckCSourceCompiles)

	# Modern bfd.h (Arch / Fedora / current binutils) starts with
	#   #if !defined PACKAGE && !defined PACKAGE_VERSION
	#   #error config.h must be included before this header
	# so a bare check_include_file() probe always fails -- libbfd is
	# present, but the probe never compiles. Pre-define both as cache
	# stubs for the duration of the probe; bfd uses them as info-print
	# strings only, so the values are cosmetic. Saved-and-restored to
	# avoid leaking into later checks.
	set (_bfd_saved_defs "${CMAKE_REQUIRED_DEFINITIONS}")
	list (APPEND CMAKE_REQUIRED_DEFINITIONS
		"-DPACKAGE=\"${PACKAGE}\""
		"-DPACKAGE_VERSION=\"${PACKAGE_VERSION}\"")
	check_include_file (bfd.h HAVE_BFD)
	set (CMAKE_REQUIRED_DEFINITIONS "${_bfd_saved_defs}")
	unset (_bfd_saved_defs)

	if (HAVE_BFD)
		# Try pkg-config first. Some distros ship bfd.pc with the full
		# transitive LIBS string already enumerated; when it's there
		# that's the cleanest path. Tumbleweed/Ubuntu binutils-devel
		# currently don't ship bfd.pc, so we still need a fallback.
		find_package (PkgConfig QUIET)
		if (PKG_CONFIG_FOUND)
			pkg_check_modules (PC_BFD QUIET bfd)
		endif()

		if (PC_BFD_FOUND AND PC_BFD_STATIC_LIBRARIES)
			# Prefer the --static form: libbfd is shipped as a static
			# archive on most distros, so we need libbfd's private deps
			# (Libs.private:) on the link line, not just the public Libs:.
			set (BFD_LIBRARY ${PC_BFD_STATIC_LIBRARIES} CACHE STRING "Lib to use when linking to bfd")
			message (STATUS "bfd via pkg-config (static): ${PC_BFD_STATIC_LIBRARIES}")
		elseif (PC_BFD_FOUND)
			set (BFD_LIBRARY ${PC_BFD_LIBRARIES} CACHE STRING "Lib to use when linking to bfd")
			message (STATUS "bfd via pkg-config: ${PC_BFD_LIBRARIES}")
		else()
			# Fallback: collect every transitive dep libbfd is known to
			# need on modern binutils, link them all unconditionally.
			# A previous probe-loop approach -- "try a hardcoded
			# combination of subsets, pick the first that compiles" --
			# couldn't keep up with the per-distro mix (libbfd grew zstd
			# in binutils 2.40, sframe in 2.42, ...) and silently matched
			# at sub-minimal combinations whose toy probe linked but
			# whose real aMule link did not.
			#
			# Safe to over-link: --as-needed (Linux default) drops shared
			# libs that don't satisfy any reference; static archives that
			# don't satisfy any reference contribute nothing. So adding
			# every plausibly-needed dep is harmless on systems that
			# don't need it, and load-bearing on systems that do.
			find_library (LIBBFD_TMP bfd)
			if (NOT LIBBFD_TMP)
				message (STATUS "No useable bfd-lib found, disabling support")
			else()
				set (_bfd_libs ${LIBBFD_TMP})
				foreach (_dep iberty zstd sframe z ${LIBINTL} dl)
					if (_dep)
						find_library (LIB_BFD_DEP_${_dep} ${_dep})
						if (LIB_BFD_DEP_${_dep})
							list (APPEND _bfd_libs ${LIB_BFD_DEP_${_dep}})
						endif()
						unset (LIB_BFD_DEP_${_dep} CACHE)
					endif()
				endforeach()
				set (BFD_LIBRARY ${_bfd_libs} CACHE STRING "Lib to use when linking to bfd")
				message (STATUS "bfd link set: ${_bfd_libs}")
				unset (_bfd_libs)
			endif()
		endif()

		if (NOT BFD_LIBRARY)
			set (HAVE_BFD FALSE)
			message (STATUS "bfd.h found but can't link against it, disabling support")
		endif()
	else()
		message (STATUS "bfd.h not found, disabling support")
	endif()

	unset (LIBBFD_TMP CACHE)
endif (NOT HAVE_BFD)
