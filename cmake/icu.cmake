# Find ICU (International Components for Unicode)
# This module finds the ICU libraries and defines:
#  ICU_FOUND - True if ICU was found
#  ICU_INCLUDE_DIRS - Include directories for ICU
#  ICU_LIBRARIES - Libraries to link against
#  ICU_VERSION - Version of ICU

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(ICU_UC icu-uc)
    pkg_check_modules(ICU_I18N icu-i18n)
endif()

# If pkg-config didn't find ICU, try manual search
if(NOT ICU_UC_FOUND)
    find_path(ICU_INCLUDE_DIR
        NAMES unicode/utypes.h
        PATHS
            /usr/include
            /usr/local/include
            /opt/local/include
    )

    find_library(ICU_UC_LIB
        NAMES icuuc icuucd
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/local/lib
            /usr/lib/x86_64-linux-gnu
    )

    find_library(ICU_I18N_LIB
        NAMES icui18n icui18nd
        PATHS
            /usr/lib
            /usr/local/lib
            /opt/local/lib
            /usr/lib/x86_64-linux-gnu
    )

    if(ICU_INCLUDE_DIR AND ICU_UC_LIB AND ICU_I18N_LIB)
        set(ICU_UC_FOUND TRUE)
        set(ICU_I18N_FOUND TRUE)
        set(ICU_UC_INCLUDE_DIRS ${ICU_INCLUDE_DIR})
        set(ICU_UC_LIBRARIES ${ICU_UC_LIB})
        set(ICU_I18N_LIBRARIES ${ICU_I18N_LIB})
    endif()
endif()

# Combine results
if(ICU_UC_FOUND AND ICU_I18N_FOUND)
    set(ICU_FOUND TRUE)
    set(ICU_INCLUDE_DIRS ${ICU_UC_INCLUDE_DIRS})
    set(ICU_LIBRARIES ${ICU_UC_LIBRARIES} ${ICU_I18N_LIBRARIES})
    
    if(DEFINED ICU_UC_VERSION)
        set(ICU_VERSION ${ICU_UC_VERSION})
    else()
        # Try to get version from header file
        if(EXISTS "${ICU_INCLUDE_DIR}/unicode/uversion.h")
            file(READ "${ICU_INCLUDE_DIR}/unicode/uversion.h" ICU_VERSION_H)
            string(REGEX MATCH "U_ICU_VERSION_MAJOR_NUM ([0-9]+)" _ ${ICU_VERSION_H})
            set(ICU_VERSION_MAJOR ${CMAKE_MATCH_1})
            string(REGEX MATCH "U_ICU_VERSION_MINOR_NUM ([0-9]+)" _ ${ICU_VERSION_H})
            set(ICU_VERSION_MINOR ${CMAKE_MATCH_1})
            set(ICU_VERSION "${ICU_VERSION_MAJOR}.${ICU_VERSION_MINOR}")
        endif()
    endif()
    
    message(STATUS "Found ICU: ${ICU_INCLUDE_DIRS}, ${ICU_LIBRARIES}")
    message(STATUS "ICU Version: ${ICU_VERSION}")
else()
    set(ICU_FOUND FALSE)
    message(STATUS "ICU not found, will use wxWidgets encoding fallback")
endif()

# Hide cache variables from GUI
mark_as_advanced(
    ICU_INCLUDE_DIR
    ICU_UC_LIB
    ICU_I18N_LIB
)
