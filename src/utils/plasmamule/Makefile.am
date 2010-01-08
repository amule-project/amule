if PLASMAMULE
modulesdir = ${KDE_MODULE_PATH}
modules_PROGRAMS = plasma-engine-plasmamule.so plasma-applet-plasmamule.so

EXTRA_DIST = plasma-engine-plasmamule.dektop plasma-applet-plasmamule.desktop amule_applet.svg
BUILT_SOURCES = plasma-engine-plasmamule.moc plasma-applet-plasmamule.moc

servicedir = ${KDE_SERVICE_PATH}
dist_service_DATA = plasma-engine-plasmamule.desktop plasma-applet-plasmamule.desktop

icondir = ${KDE_ICON_PATH}
dist_icon_DATA = amule_applet.svg
endif

plasma_engine_plasmamule_so_SOURCES = plasma-engine-plasmamule.cpp
plasma_applet_plasmamule_so_SOURCES = plasma-applet-plasmamule.cpp

nodist_plasma_engine_plasmamule_so_SOURCES  = plasma-engine-plasmamule.moc
nodist_plasma_applet_plasmamule_so_SOURCES = plasma-applet-plasmamule.moc

plasma_engine_plasmamule_so_CXXFLAGS = -shared -I${KDE_HEADER_DIR} ${QT_CORE_CXXFLAGS} -DPIC -fPIC
plasma_applet_plasmamule_so_CXXFLAGS = -shared -D__IMG_PATH__=\"${KDE_ICON_PATH}\" -I${KDE_HEADER_DIR} ${QT_GUI_CXXFLAGS} -DPIC -fPIC

plasma_engine_plasmamule_so_LD_FLAGS = ${QT_CORE_LIBS}
plasma_applet_plasmamule_so_LD_FLAGS = ${QT_GUI_LIBS}

noinst_HEADERS = plasma-engine-plasmamule.h plasma-applet-plasmamule.h

% : %.moc

%.moc : %.h
	${QT_MOC} -o $@ $<

CLEANFILES = *.moc

MAINTAINERCLEANFILES = Makefile.in
