#
# CMakeLists.txt for geoip module
#

# maxminddb is already found by cmake/ip2country.cmake which is included in the main CMakeLists.txt
# The geoip module is only built when ENABLE_IP2COUNTRY is enabled, so maxminddb is guaranteed to be available

# Create geoip library
add_library(mulegeoip STATIC
    IP2CountryManager.cpp
    MaxMindDBDatabase.cpp
    DatabaseFactory.cpp
    UpdateScheduler.cpp
)
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")

message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
# Add include directories
target_include_directories(mulegeoip
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}      # For config.h generated at root build directory
        ${CMAKE_SOURCE_DIR}/src/pixmaps/flags_xpm  # Add path to XPM files
        ${CMAKE_SOURCE_DIR}/src/libs  # For libs/common includes
        ${CMAKE_SOURCE_DIR}/src/include  # For include/tags and include/common includes
        ${maxminddb_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(mulegeoip
    PUBLIC
        maxminddb::maxminddb
        mulecommon
        ${wxWidgets_LIBRARIES}
)

# Set C++ standard
target_compile_features(mulegeoip PRIVATE cxx_std_11)

# Enable MaxMind DB support
target_compile_definitions(mulegeoip PRIVATE ENABLE_MAXMINDDB)

# Platform-specific settings
if(WIN32)
    target_link_libraries(mulegeoip PRIVATE ws2_32 mswsock)
elseif(APPLE)
    target_link_libraries(mulegeoip PRIVATE "-framework SystemConfiguration")
else()
    target_link_libraries(mulegeoip PRIVATE pthread)
endif()

# Installation (if needed)
install(TARGETS mulegeoip
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES
    IP2CountryManager.h
    DatabaseFactory.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/amule/geoip
)