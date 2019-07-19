ADD_EXECUTABLE (CTagTest
	CTagTest.cpp
	${CMAKE_SOURCE_DIR}/src/SafeFile.cpp
	${CMAKE_SOURCE_DIR}/src/MemFile.cpp
	${CMAKE_SOURCE_DIR}/src/Tag.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

TARGET_COMPILE_DEFINITIONS (CTagTest
	PRIVATE "-DMULEUNIT"
	PRIVATE ${wxWidgets_BASE_DEFS}
)

TARGET_INCLUDE_DIRECTORIES (CTagTest
	PRIVATE "${CMAKE_SOURCE_DIR}/src"
	PRIVATE "${CMAKE_SOURCE_DIR}/src/include"
	PRIVATE "${CMAKE_SOURCE_DIR}/src/libs"
	PRIVATE "${CMAKE_SOURCE_DIR}/unittests"
)

TARGET_LINK_LIBRARIES (CTagTest
	muleunit
	${wxWidgets_BASE_LIBRARIES}
)

ADD_EXECUTABLE (CUInt128Test
	CUInt128Test.cpp
	${CMAKE_SOURCE_DIR}/src/kademlia/utils/UInt128.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

TARGET_COMPILE_DEFINITIONS (CUInt128Test
	PRIVATE "-DMULEUNIT"
	PRIVATE ${wxWidgets_BASE_DEFS}
)

TARGET_INCLUDE_DIRECTORIES (CUInt128Test
	PRIVATE "${CMAKE_SOURCE_DIR}/src"
	PRIVATE "${CMAKE_SOURCE_DIR}/src/libs"
	PRIVATE "${CMAKE_SOURCE_DIR}/unittests"
)

TARGET_LINK_LIBRARIES (CUInt128Test
	muleunit
	${wxWidgets_BASE_LIBRARIES}
)

ADD_EXECUTABLE (FileDataIOTest
	FileDataIOTest.cpp
	${CMAKE_SOURCE_DIR}/src/SafeFile.cpp
	${CMAKE_SOURCE_DIR}/src/CFile.cpp
	${CMAKE_SOURCE_DIR}/src/MemFile.cpp
	${CMAKE_SOURCE_DIR}/src/kademlia/utils/UInt128.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/StringFunctions.cpp
	${CMAKE_SOURCE_DIR}/src/Tag.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

TARGET_LINK_LIBRARIES (FileDataIOTest
	muleunit
	${wxWidgets_BASE_LIBRARIES}
)

TARGET_COMPILE_DEFINITIONS (FileDataIOTest
	PRIVATE "-DMULEUNIT"
	PRIVATE ${wxWidgets_BASE_DEFS}
)

TARGET_INCLUDE_DIRECTORIES (FileDataIOTest
	PRIVATE "${CMAKE_SOURCE_DIR}/src"
	PRIVATE "${CMAKE_SOURCE_DIR}/src/include"
	PRIVATE "${CMAKE_SOURCE_DIR}/src/libs"
	PRIVATE "${CMAKE_SOURCE_DIR}/unittests"
)

ADD_EXECUTABLE (FormatTest
	FormatTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

TARGET_COMPILE_DEFINITIONS (FormatTest
	PRIVATE "-DMULEUNIT"
	PRIVATE ${wxWidgets_BASE_DEFS}
)

TARGET_INCLUDE_DIRECTORIES (FormatTest
	PRIVATE "${CMAKE_SOURCE_DIR}/src"
	PRIVATE "${CMAKE_SOURCE_DIR}/src/libs"
	PRIVATE "${CMAKE_SOURCE_DIR}/unittests"
)

TARGET_LINK_LIBRARIES (FormatTest
	muleunit
	${wxWidgets_BASE_LIBRARIES}
)

ADD_EXECUTABLE (NetworkFunctionsTest
	NetworkFunctionsTest.cpp
	${CMAKE_SOURCE_DIR}/src/NetworkFunctions.cpp
	${CMAKE_SOURCE_DIR}/src/LibSocket.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

TARGET_COMPILE_DEFINITIONS (NetworkFunctionsTest
	PRIVATE "-DMULEUNIT"
	PRIVATE "-DEC_REMOTE"
	PRIVATE ${wxWidgets_BASE_DEFS}
	PRIVATE ${wxWidgets_NET_DEFS}
)

TARGET_INCLUDE_DIRECTORIES (NetworkFunctionsTest
	PRIVATE "${CMAKE_SOURCE_DIR}/src"
	PRIVATE "${CMAKE_SOURCE_DIR}/src/include"
	PRIVATE "${CMAKE_SOURCE_DIR}/src/libs"
	PRIVATE "${CMAKE_SOURCE_DIR}/unittests"
)

TARGET_LINK_LIBRARIES (NetworkFunctionsTest
	muleunit
	${wxWidgets_NET_LIBRARIES}
)

IF (ENABLE_BOOST)
	TARGET_LINK_LIBRARIES (NetworkFunctionsTest boost_system)
ENDIF (ENABLE_BOOST)

ADD_EXECUTABLE (PathTest
	PathTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/StringFunctions.cpp
)

TARGET_COMPILE_DEFINITIONS (PathTest
	PRIVATE "-DMULEUNIT"
	PRIVATE ${wxWidgets_BASE_DEFS}
)

TARGET_INCLUDE_DIRECTORIES (PathTest
	PRIVATE "${CMAKE_SOURCE_DIR}/src/libs"
	PRIVATE "${CMAKE_SOURCE_DIR}/unittests"
)

TARGET_LINK_LIBRARIES (PathTest
	muleunit
	${wxWidgets_BASE_LIBRARIES}
)

ADD_EXECUTABLE (RangeMapTest
	RangeMapTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

TARGET_COMPILE_DEFINITIONS (RangeMapTest
	PRIVATE "-DMULEUNIT"
	PRIVATE ${wxWidgets_BASE_DEFS}
)

TARGET_INCLUDE_DIRECTORIES (RangeMapTest
	PRIVATE "${CMAKE_SOURCE_DIR}/src"
	PRIVATE "${CMAKE_SOURCE_DIR}/src/libs"
	PRIVATE "${CMAKE_SOURCE_DIR}/unittests"
)

TARGET_LINK_LIBRARIES (RangeMapTest
	muleunit
	${wxWidgets_BASE_LIBRARIES}
)

ADD_EXECUTABLE (StringFunctionsTest
	StringFunctionsTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${CMAKE_SOURCE_DIR}/src/libs/common/StringFunctions.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
)

TARGET_COMPILE_DEFINITIONS (StringFunctionsTest
	PRIVATE "-DMULEUNIT"
	PRIVATE ${wxWidgets_BASE_DEFS}
)

TARGET_INCLUDE_DIRECTORIES (StringFunctionsTest
	PRIVATE "${CMAKE_SOURCE_DIR}/unittests"
	PRIVATE "${CMAKE_SOURCE_DIR}/src/libs"
)

TARGET_LINK_LIBRARIES (StringFunctionsTest
	muleunit
	${wxWidgets_BASE_LIBRARIES}
)

ADD_EXECUTABLE (TextFileTest
	TextFileTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/StringFunctions.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/TextFile.cpp
)

TARGET_COMPILE_DEFINITIONS (TextFileTest
	PRIVATE "-DMULEUNIT"
	PRIVATE ${wxWidgets_BASE_DEFS}
	PRIVATE "-DSRCDIR=${CMAKE_CURRENT_SOURCE_DIR}"
)

TARGET_INCLUDE_DIRECTORIES (TextFileTest
	PRIVATE "${CMAKE_SOURCE_DIR}/src"
	PRIVATE "${CMAKE_SOURCE_DIR}/src/include"
	PRIVATE "${CMAKE_SOURCE_DIR}/src/libs"
	PRIVATE "${CMAKE_SOURCE_DIR}/unittests"
)

TARGET_LINK_LIBRARIES (TextFileTest
	muleunit
	${wxWidgets_BASE_LIBRARIES}
)
