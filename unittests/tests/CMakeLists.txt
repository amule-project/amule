ADD_EXECUTABLE (CTagTest
	CTagTest.cpp
	${CMAKE_SOURCE_DIR}/src/SafeFile.cpp
	${CMAKE_SOURCE_DIR}/src/MemFile.cpp
	${CMAKE_SOURCE_DIR}/src/Tag.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

TARGET_INCLUDE_DIRECTORIES (CTagTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

TARGET_LINK_LIBRARIES (CTagTest
	muleunit
)

ADD_EXECUTABLE (CUInt128Test
	CUInt128Test.cpp
	${CMAKE_SOURCE_DIR}/src/kademlia/utils/UInt128.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

TARGET_INCLUDE_DIRECTORIES (CUInt128Test
	PRIVATE ${CMAKE_SOURCE_DIR}/src
)

TARGET_LINK_LIBRARIES (CUInt128Test
	muleunit
)

ADD_EXECUTABLE (FileDataIOTest
	FileDataIOTest.cpp
	${CMAKE_SOURCE_DIR}/src/SafeFile.cpp
	${CMAKE_SOURCE_DIR}/src/CFile.cpp
	${CMAKE_SOURCE_DIR}/src/MemFile.cpp
	${CMAKE_SOURCE_DIR}/src/kademlia/utils/UInt128.cpp
	${CMAKE_SOURCE_DIR}/src/Tag.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

TARGET_LINK_LIBRARIES (FileDataIOTest
	muleunit
)

TARGET_INCLUDE_DIRECTORIES (FileDataIOTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

ADD_EXECUTABLE (FormatTest
	FormatTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

TARGET_INCLUDE_DIRECTORIES (FormatTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
)

TARGET_LINK_LIBRARIES (FormatTest
	muleunit
)

ADD_EXECUTABLE (NetworkFunctionsTest
	NetworkFunctionsTest.cpp
	${CMAKE_SOURCE_DIR}/src/NetworkFunctions.cpp
	${CMAKE_SOURCE_DIR}/src/LibSocket.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

TARGET_COMPILE_DEFINITIONS (NetworkFunctionsTest
	PRIVATE "EC_REMOTE"
)

TARGET_INCLUDE_DIRECTORIES (NetworkFunctionsTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

IF (ENABLE_BOOST)
	TARGET_INCLUDE_DIRECTORIES (NetworkFunctionsTest
		PRIVATE ${Boost_INCLUDE_DIR}
	)

	TARGET_LINK_LIBRARIES (NetworkFunctionsTest
		${Boost_LIBRARIES}
	)
ENDIF (ENABLE_BOOST)

TARGET_LINK_LIBRARIES (NetworkFunctionsTest
	muleunit
	wxWidgets::NET
)

ADD_EXECUTABLE (PathTest
	PathTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
)

TARGET_LINK_LIBRARIES (PathTest
	muleunit
)

ADD_EXECUTABLE (RangeMapTest
	RangeMapTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
)

TARGET_INCLUDE_DIRECTORIES (RangeMapTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
)

TARGET_LINK_LIBRARIES (RangeMapTest
	muleunit
)

ADD_EXECUTABLE (StringFunctionsTest
	StringFunctionsTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
)

TARGET_LINK_LIBRARIES (StringFunctionsTest
	muleunit
)

ADD_EXECUTABLE (TextFileTest
	TextFileTest.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/Format.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/strerror_r.c
	${CMAKE_SOURCE_DIR}/src/libs/common/Path.cpp
	${CMAKE_SOURCE_DIR}/src/libs/common/TextFile.cpp
)

TARGET_COMPILE_DEFINITIONS (TextFileTest
	PRIVATE "SRCDIR=${CMAKE_CURRENT_SOURCE_DIR}"
)

TARGET_INCLUDE_DIRECTORIES (TextFileTest
	PRIVATE ${CMAKE_SOURCE_DIR}/src
	PRIVATE ${CMAKE_SOURCE_DIR}/src/include
)

TARGET_LINK_LIBRARIES (TextFileTest
	muleunit
)
