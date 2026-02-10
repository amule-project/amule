# Uncomment one or more of the following lines to tell cmake, where to find it's header and
# libs. You can specify the path also using -D<VAR> from commandline.

# The location, where the bison executable can be found
#SET (BISON_EXECUTABLE "${CMAKE_SOURCE_DIR}/../win_flex_bison-latest/win_bison.exe")

# The location, where to find the crypto++ header-files
#SET (BOOSTROOT "${CMAKE_SOURCE_DIR}/../boost_1_70_0")

# The location, where to find the crypto++ header-files
#SET (CRYTOPP_HEADER_PATH "${CMAKE_SOURCE_DIR}/../cryptopp")

# The location, where the linkable output of crypto++ compilation can be found.
#SET (CRYTOPP_LIB_SEARCH_PATH "${CMAKE_SOURCE_DIR}/../cryptopp/Win32/Output/Debug")

# The location, where the flex executable can be found
#SET (FLEX_EXECUTABLE "${CMAKE_SOURCE_DIR}/../win_flex_bison-latest/win_flex.exe")

# The location, where the headers of libgeoip can be found.
#SET (GEOIP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../geoip-api-c-master/libGeoIP")

# The location, where to find libpng headers.
#SET (PNG_HEADER_DIR "${CMAKE_SOURCE_DIR}/../lpng1626")

# The location, where to find libpng.
#SET (PNG_LIB_DIR "${CMAKE_SOURCE_DIR}/../lpng1626/projects/vstudio/Debug")

# The location, where you extracted wx-sources
#SET (wxWidgets_ROOT_DIR "${CMAKE_SOURCE_DIR}/../wxWidgets")

# The location, where to find zlib.
#SET (ZLIB_ROOT "${CMAKE_SOURCE_DIR}/../zlib-1.2.8")

if (BISON_EXECUTABLE)
	set (SEARCH_DIR_BISON ${BISON_EXECUTABLE})
endif()

if (BOOST_INCLUDE_DIR)
	set (SEARCH_DIR_BOOST ${BOOST_INCLUDE_DIR})
endif()

if (CRYTOPP_HEADER_PATH)
	set (SEARCH_DIR_CRYPTOPP_HEADER ${CRYTOPP_HEADER_PATH})
endif()

if (CRYTOPP_LIB_SEARCH_PATH)
	set (SEARCH_DIR_CRYPTOPP_LIB ${CRYTOPP_LIB_SEARCH_PATH})
endif()

if (FLEX_EXECUTABLE)
	set (SEARCH_DIR_FLEX ${FLEX_EXECUTABLE})
endif()

if (GEOIP_INCLUDE_DIR)
	set (SEARCH_DIR_GEOIP ${GEOIP_INCLUDE_DIR})
endif()

if (PNG_HEADER_DIR)
	set (SEARCH_DIR_PNG_HEADER ${PNG_HEADER_DIR})
endif()

if (PNG_LIB_DIR)
	set (SEARCH_DIR_PNG_LIB ${PNG_LIB_DIR})
endif()

if (wxWidgets_ROOT_DIR)
	set (SEARCH_DIR_wxWidgets ${wxWidgets_ROOT_DIR})
endif()

if (ZLIB_ROOT)
	set (SEARCH_DIR_ZLIB ${ZLIB_ROOT})
endif()

set (SEARCH_DIR_BISON ${SEARCH_DIR_BISON} CACHE FILEPATH "Location of the bison executable" FORCE)
set (SEARCH_DIR_BOOST ${SEARCH_DIR_BOOST} CACHE PATH "Search hint for boost" FORCE)
set (SEARCH_DIR_CRYPTOPP_HEADER ${SEARCH_DIR_CRYPTOPP_HEADER} CACHE PATH "Search hint for crypto++ headers" FORCE)
set (SEARCH_DIR_CRYPTOPP_LIB ${SEARCH_DIR_CRYPTOPP_LIB} CACHE PATH "Search hint for crypto++ library" FORCE)
set (SEARCH_DIR_FLEX ${SEARCH_DIR_FLEX} CACHE FILEPATH "Location of the flex executable" FORCE)
set (SEARCH_DIR_GEOIP ${SEARCH_DIR_GEOIP} CACHE PATH "Search hint for geoip" FORCE)
set (SEARCH_DIR_PNG_HEADER ${SEARCH_DIR_PNG_HEADER} CACHE PATH "Search hint for libpng headers" FORCE)
set (SEARCH_DIR_PNG_LIB ${SEARCH_DIR_PNG_LIB} CACHE PATH "Search hint for libpng library" FORCE)
set (SEARCH_DIR_UPNP ${SEARCH_DIR_UPNP} CACHE PATH "Search hint for libupnp library" FORCE)
set (SEARCH_DIR_wxWidgets ${SEARCH_DIR_wxWidgets} CACHE PATH "Search hint for wxconfig executable" FORCE)
set (SEARCH_DIR_ZLIB ${SEARCH_DIR_ZLIB} CACHE PATH "Search hint for zlib" FORCE)
mark_as_advanced (FORCE SEARCH_DIR_BISON)
mark_as_advanced (FORCE SEARCH_DIR_BOOST)
mark_as_advanced (FORCE SEARCH_DIR_CRYPTOPP_HEADER)
mark_as_advanced (FORCE SEARCH_DIR_CRYPTOPP_LIB)
mark_as_advanced (FORCE SEARCH_DIR_FLEX)
mark_as_advanced (FORCE SEARCH_DIR_GEOIP)
mark_as_advanced (FORCE SEARCH_DIR_PNG_HEADER)
mark_as_advanced (FORCE SEARCH_DIR_PNG_LIB)
mark_as_advanced (FORCE SEARCH_DIR_UPNP)
mark_as_advanced (FORCE SEARCH_DIR_wxWidgets)
mark_as_advanced (FORCE SEARCH_DIR_ZLIB)

if (SEARCH_DIR_BISON AND NOT BISON_EXECUTABLE)
	set (BISON_EXECUTABLE ${SEARCH_DIR_BISON})
endif()

if (SEARCH_DIR_BOOST AND NOT BOOST_INCLUDE_DIR)
	set (BOOST_ROOT ${SEARCH_DIR_BOOST})
endif()

if (SEARCH_DIR_CRYPTOPP_HEADER AND NOT CRYTOPP_HEADER_PATH)
	set (CRYTOPP_HEADER_PATH ${SEARCH_DIR_CRYPTOPP_HEADER})
endif()

if (SEARCH_DIR_CRYPTOPP_LIB AND NOT CRYTOPP_LIB_SEARCH_PATH)
	set (CRYTOPP_LIB_SEARCH_PATH ${SEARCH_DIR_CRYPTOPP_LIB})
endif()

if (SEARCH_DIR_FLEX AND NOT FLEX_EXECUTABLE)
	set (FLEX_EXECUTABLE ${SEARCH_DIR_FLEX})
endif()

if (SEARCH_DIR_GEOIP AND NOT GEOIP_INCLUDE_DIR)
	set (GEOIP_INCLUDE_DIR ${SEARCH_DIR_GEOIP})
endif()

if (SEARCH_DIR_PNG_HEADER AND NOT PNG_HEADER_DIR)
	set (PNG_HEADER_DIR ${SEARCH_DIR_PNG_HEADER})
endif()

if (SEARCH_DIR_PNG_LIB AND NOT PNG_LIB_DIR)
	set (PNG_LIB_DIR ${SEARCH_DIR_PNG_LIB})
endif()

if (SEARCH_DIR_wxWidgets AND NOT wxWidgets_ROOT_DIR)
	set (wxWidgets_ROOT_DIR ${SEARCH_DIR_wxWidgets})
endif()

if (SEARCH_DIR_ZLIB AND NOT ZLIB_ROOT)
	set (ZLIB_ROOT ${SEARCH_DIR_ZLIB})
endif()
