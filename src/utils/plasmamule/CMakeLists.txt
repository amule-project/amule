CMAKE_POLICY (SET CMP0059 OLD)

INCLUDE (${QT_USE_FILE})
INCLUDE (FindKDE4)
INCLUDE (KDE4Macros)
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_BINARY_DIR})

FIND_PACKAGE (SharedMimeInfo)

QT4_GENERATE_MOC (${CMAKE_CURRENT_SOURCE_DIR}/plasma-applet-plasmamule.h ${CMAKE_CURRENT_BINARY_DIR}/plasma-applet-plasmamule.moc)
QT4_GENERATE_MOC (${CMAKE_CURRENT_SOURCE_DIR}/plasma-engine-plasmamule.h ${CMAKE_CURRENT_BINARY_DIR}/plasma-engine-plasmamule.moc)

ADD_EXECUTABLE (plasmamule-engine-feeder
	plasmamule-dbus.cpp
	plasmamule-engine-feeder.cpp
)

TARGET_LINK_LIBRARIES (plasmamule-engine-feeder
	${KDE4_PLASMA_LIBS}
)

KDE4_ADD_PLUGIN (plasma-applet-plasmamule
	plasma-applet-plasmamule.cpp
	plasma-applet-plasmamule.moc
	plasmamule-dbus.cpp
)

TARGET_COMPILE_DEFINITIONS (plasma-applet-plasmamule
	PRIVATE __IMG_PATH__="${ICON_INSTALL_DIR}"
)

TARGET_LINK_LIBRARIES (plasma-applet-plasmamule
	${KDE4_PLASMA_LIBS}
	${KDE4_KIO_LIBS}
)

KDE4_ADD_PLUGIN (plasma-engine-plasmamule
	plasma-engine-plasmamule.cpp
	plasma-engine-plasmamule.moc
	qt-emc.cpp
)

TARGET_LINK_LIBRARIES (plasma-engine-plasmamule
	${KDE4_PLASMA_LIBS}
	${KDE4_KIO_LIBS}
)

INSTALL (TARGETS plasma-engine-plasmamule plasma-applet-plasmamule
	DESTINATION ${PLUGIN_INSTALL_DIR}
)

INSTALL (FILES
	ed2k.protocol
	magnet.protocol
	plasma-applet-plasmamule.desktop
	plasma-engine-plasmamule.desktop
	DESTINATION ${SERVICES_INSTALL_DIR}
)

INSTALL (TARGETS plasmamule-engine-feeder
	RUNTIME DESTINATION bin
)

INSTALL (FILES plasmamule-engine-feeder.desktop
		DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

KDE4_INSTALL_ICONS (${ICON_INSTALL_DIR})

INSTALL (FILES amule.xml
	DESTINATION ${XDG_MIME_INSTALL_DIR}
)

UPDATE_XDG_MIMETYPES (${XDG_MIME_INSTALL_DIR})
