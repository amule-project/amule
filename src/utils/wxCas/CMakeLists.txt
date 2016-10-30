ADD_SUBDIRECTORY (docs)
ADD_SUBDIRECTORY (src)

INSTALL (FILES wxcas.xpm
	DESTINATION "${DATADIR}/pixmaps"
)

INSTALL (FILES wxcas.desktop
	DESTINATION "${DATADIR}/applications"
)
