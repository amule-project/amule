if PLASMAMULE
modulesdir = ${KDE_MODULE_PATH}
modules_PROGRAMS = plasma-engine-plasmamule.so plasma-applet-plasmamule.so

EXTRA_DIST = plasma-engine-plasmamule.dektop plasma-applet-plasmamule.desktop application-x-emulecollection.svg ed2k.protocol magnet.protocol plasmamule-engine-feeder.desktop
BUILT_SOURCES = plasma-engine-plasmamule.moc plasma-applet-plasmamule.moc

servicedir = ${KDE_SERVICE_PATH}
dist_service_DATA = plasma-engine-plasmamule.desktop plasma-applet-plasmamule.desktop ed2k.protocol magnet.protocol

icondir = ${KDE_ICON_PATH}
dist_icon_DATA = application-x-emulecollection.svg

mimedir = ${KDE_MIME_PATH}
dist_mime_DATA = amule.xml

appsdir = ${KDE_APPLNK_PATH}
dist_apps_DATA = plasmamule-engine-feeder.desktop

bin_PROGRAMS = plasmamule-engine-feeder
endif

plasma_engine_plasmamule_so_SOURCES = plasma-engine-plasmamule.cpp qt-emc.cpp
plasma_applet_plasmamule_so_SOURCES = plasma-applet-plasmamule.cpp plasmamule-dbus.cpp
plasmamule_engine_feeder_SOURCES = plasmamule-engine-feeder.cpp plasmamule-dbus.cpp

nodist_plasma_engine_plasmamule_so_SOURCES  = plasma-engine-plasmamule.moc
nodist_plasma_applet_plasmamule_so_SOURCES = plasma-applet-plasmamule.moc

plasma_engine_plasmamule_so_CXXFLAGS = -shared -I${KDE_HEADER_DIR} ${QT_CORE_CXXFLAGS} -DPIC -fPIC ${DEBUGFLAG}
plasma_applet_plasmamule_so_CXXFLAGS = -shared -D__IMG_PATH__=\"${KDE_ICON_PATH}\" -I${KDE_HEADER_DIR} ${QT_GUI_CXXFLAGS} -DPIC -fPIC ${DEBUGFLAG}
plasmamule_engine_feeder_CXXFLAGS = -I${KDE_HEADER_DIR} ${QT_GUI_CXXFLAGS} ${DEBUGFLAG}

plasmamule_engine_feeder_LDFLAGS = ${QT_GUI_LIBS} -lkdecore -lplasma

noinst_HEADERS = plasma-engine-plasmamule.h plasma-applet-plasmamule.h plasmamule-dbus.h qt-emc.h

%.moc : %.h
	${QT_MOC} -o $@ $<

CLEANFILES = *.moc

MAINTAINERCLEANFILES = Makefile.in

install-data-hook: ${USER_INSTALL_HOOK}

USER_INSTALL:
	${UPDATE_MIME_DATABASE} ${KDE_MIME_PATH}
	${KBUILDSYCOCA}
