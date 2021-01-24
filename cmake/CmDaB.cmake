# This file is part of the CmDab Project.
#
# Copyright (c) 2021 Vollstrecker (werner@vollstreckernet.de)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License Version 3as published by
# the Free Software Foundation; of the License
#
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, check
# https://github.com/Vollstrecker/CmDaB/blob/main/LICENSE

#[=======================================================================[.rst:
CmDaB.cmake
------------------

.. only:: html

  .. contents::

Overview
^^^^^^^^

  This module adds an option called DOWNLOAD_AND_BUILD_DEPS to your project.
  If this option is activated by the user, this module will download and
  include the main project files to be used within your project.

#]=======================================================================]

include (CMakeDependentOption)

Find_Package (Git)
cmake_dependent_option (DOWNLOAD_AND_BUILD_DEPS "Get all missing stuff" OFF ${Git_FOUND} OFF)

if (DOWNLOAD_AND_BUILD_DEPS)
	include (FetchContent)

	FetchContent_Declare (
		CmDaB
		GIT_REPOSITORY https://github.com/Vollstrecker/CmDaB.git
		GIT_TAG main
	)

	FetchContent_GetProperties(CmDaB)

	if (NOT CmDaB_cmdab_POPULATED)
		FetchContent_MakeAvailable (CmDaB)
	endif()
endif()
