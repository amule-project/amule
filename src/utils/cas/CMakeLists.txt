ADD_SUBDIRECTORY (docs)

ADD_EXECUTABLE (cas
	cas.c
	configfile.c
	functions.c
	graphics.c
	html.c
	lines.c
)

TARGET_COMPILE_DEFINITIONS (cas
		PRIVATE ${gdlib_CFLAGS}
)

TARGET_LINK_LIBRARIES (cas
	${gdlib_LIBRARIES}
	${gdlib_LD_FLAGS}
)

INSTALL (TARGETS cas
	RUNTIME DESTINATION bin
)


INSTALL ( FILES tmp.html stat.png
	DESTINATION "${DATADIR}/cas"
)

