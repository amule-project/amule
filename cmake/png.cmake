if (PNG_HEADER_DIR)
	set (CMAKE_INCLUDE_PATH ${PNG_HEADER_DIR})
endif()

if (PNG_LIB_DIR)
	set (CMAKE_LIBRARY_PATH ${PNG_LIB_DIR})
endif()

include (FindPNG)

if (PNG_FOUND)
	set (WITH_LIBPNG TRUE)
endif()

if (PNG_HEADER_DIR)
	set (CMAKE_INCLUDE_PATH)
endif()

if (PNG_LIB_DIR)
	set (CMAKE_LIBRARY_PATH)
endif()
