AM_CPPFLAGS = $(MULECPPFLAGS)
AM_CFLAGS = $(MULECFLAGS)
AM_CXXFLAGS = $(MULECXXFLAGS)
AM_LDFLAGS = $(MULELDFLAGS)

SUBDIRS = libs utils pixmaps

if WEB
SUBDIRS += webserver
endif

if INSTALL_SKINS
SUBDIRS += skins
endif

EXTRA_DIST = \
	aMule.xpm \
	Parser.y \
	Scanner.h.in \
	Scanner.l \
	IPFilterScanner.l \
	muuli.wdr 

bin_PROGRAMS = 

if MONOLITHIC
bin_PROGRAMS += amule
endif

if COMPILE_CMD
bin_PROGRAMS += amulecmd   
endif

if ED2K
bin_PROGRAMS += ed2k
endif

if AMULE_GUI
bin_PROGRAMS += amulegui
endif

if AMULE_DAEMON
bin_PROGRAMS += amuled
endif

# Sources
noinst_LIBRARIES =
if MONOLITHIC
noinst_LIBRARIES += libmuleappcore.a libmuleappgui.a libmuleappcommon.a
else
if AMULE_DAEMON
noinst_LIBRARIES += libmuleappcore.a libmuleappcommon.a
else
if AMULE_GUI
noinst_LIBRARIES += libmuleappgui.a libmuleappcommon.a
endif
endif
endif


# Common to core/gui/monolithic

libmuleappcommon_a_SOURCES = \
	CFile.cpp \
	ClientCredits.cpp \
	ED2KLink.cpp \
	Friend.cpp \
	GapList.cpp \
	MagnetURI.cpp \
	MemFile.cpp \
	NetworkFunctions.cpp \
	OtherFunctions.cpp \
	Packet.cpp \
	RLE.cpp \
	SafeFile.cpp \
	SHA.cpp \
	Tag.cpp \
	TerminationProcess.cpp \
	Timer.cpp

libmuleappcommon_a_CPPFLAGS = $(AM_CPPFLAGS) $(WXBASE_CPPFLAGS) -I$(srcdir)/libs -I$(srcdir)/include $(CRYPTOPP_CPPFLAGS) $(LIBUPNP_CPPFLAGS)


# Common to core/monolithic

libmuleappcore_a_SOURCES = \
	AsyncDNS.cpp \
	DeadSourceList.cpp \
	FileArea.cpp \
	FileAutoClose.cpp \
	IPFilterScanner.cpp \
	Scanner.cpp \
	Parser.cpp \
	PlatformSpecific.cpp \
	RandomFunctions.cpp \
	RC4Encrypt.cpp \
	StateMachine.cpp \
	TerminationProcessAmuleweb.cpp \
	ThreadScheduler.cpp \
	UPnPBase.cpp \
	kademlia/kademlia/Entry.cpp \
	kademlia/kademlia/SearchManager.cpp \
	kademlia/routing/RoutingBin.cpp \
	kademlia/utils/UInt128.cpp

libmuleappcore_a_CPPFLAGS = $(AM_CPPFLAGS) $(WXBASE_CPPFLAGS) -I$(srcdir)/libs -I$(srcdir)/include $(CRYPTOPP_CPPFLAGS) $(LIBUPNP_CPPFLAGS)


# Common to gui/monolithic

libmuleappgui_a_SOURCES = \
	BarShader.cpp \
	ColorFrameCtrl.cpp \
	EditServerListDlg.cpp \
	extern/wxWidgets/listctrl.cpp \
	FileDetailListCtrl.cpp \
	IP2Country.cpp \
	MuleColour.cpp \
	MuleGifCtrl.cpp \
	MuleListCtrl.cpp \
	MuleNotebook.cpp \
	MuleTextCtrl.cpp \
	MuleCollection.cpp \
	muuli_wdr.cpp 

libmuleappgui_a_CPPFLAGS = $(AM_CPPFLAGS) $(WX_CPPFLAGS) -I$(srcdir)/libs -I$(srcdir)/include $(LIBUPNP_CPPFLAGS) $(GEOIP_CPPFLAGS)

core_sources = \
	amule.cpp \
	BaseClient.cpp \
	CanceledFileList.cpp \
	ClientList.cpp \
	ClientCreditsList.cpp \
	ClientTCPSocket.cpp \
	ClientUDPSocket.cpp \
	CorruptionBlackBox.cpp \
	DownloadClient.cpp \
	DownloadQueue.cpp \
	ECSpecialCoreTags.cpp \
	EMSocket.cpp \
	EncryptedStreamSocket.cpp \
	EncryptedDatagramSocket.cpp \
	ExternalConn.cpp \
	FriendList.cpp \
	IPFilter.cpp \
	KnownFileList.cpp \
	ListenSocket.cpp \
	MuleUDPSocket.cpp \
	SearchFile.cpp \
	SearchList.cpp \
	ServerConnect.cpp \
	ServerList.cpp \
	ServerSocket.cpp \
	ServerUDPSocket.cpp \
	SHAHashSet.cpp \
	SharedFileList.cpp \
	ThreadTasks.cpp \
	UploadBandwidthThrottler.cpp \
	UploadClient.cpp \
	UploadQueue.cpp \
	kademlia/kademlia/Indexed.cpp \
	kademlia/kademlia/Kademlia.cpp \
	kademlia/kademlia/Prefs.cpp \
	kademlia/kademlia/Search.cpp \
	kademlia/kademlia/UDPFirewallTester.cpp \
	kademlia/net/KademliaUDPListener.cpp \
	kademlia/net/PacketTracking.cpp \
	kademlia/routing/Contact.cpp \
	kademlia/routing/RoutingZone.cpp


gui_sources = \
	amule-gui.cpp \
	amuleDlg.cpp \
	AddFriend.cpp \
	CatDialog.cpp \
	ChatSelector.cpp \
	ChatWnd.cpp \
	CommentDialog.cpp \
	CommentDialogLst.cpp \
	GenericClientListCtrl.cpp \
	ClientDetailDialog.cpp \
	DirectoryTreeCtrl.cpp \
	FileDetailDialog.cpp \
	KadDlg.cpp \
	OScopeCtrl.cpp \
	PrefsUnifiedDlg.cpp \
	SearchDlg.cpp \
	ServerWnd.cpp \
	SharedFilesWnd.cpp \
	StatisticsDlg.cpp \
	SearchListCtrl.cpp \
	DownloadListCtrl.cpp \
	SourceListCtrl.cpp \
	SharedFilePeersListCtrl.cpp \
	FriendListCtrl.cpp \
	ServerListCtrl.cpp \
	SharedFilesCtrl.cpp \
	MuleTrayIcon.cpp \
	TransferWnd.cpp 


common_sources = \
	amuleAppCommon.cpp \
	ClientRef.cpp \
	DataToText.cpp \
	ECSpecialMuleTags.cpp \
	KnownFile.cpp \
	GetTickCount.cpp \
	GuiEvents.cpp \
	HTTPDownload.cpp \
	Logger.cpp \
	PartFile.cpp \
	Preferences.cpp \
	Proxy.cpp \
	Server.cpp \
	Statistics.cpp \
	StatTree.cpp \
	UserEvents.cpp

# Libs

core_libs = -L. -lmuleappcore $(LIBUPNP_LDFLAGS) $(LIBUPNP_LIBS)
gui_libs = -L. -lmuleappgui $(WX_LIBS) $(GEOIP_LDFLAGS) $(GEOIP_LIBS)
remote_common_libs = -Llibs/common -Llibs/ec/cpp -lmulecommon -lec $(BFD_LIBS) $(ZLIB_LDFLAGS) $(ZLIB_LIBS) $(RESOLV_LIB)
common_libs = -L. -lmuleappcommon $(remote_common_libs) $(CRYPTOPP_LDFLAGS) $(CRYPTOPP_LIBS)

core_deps = libmuleappcore.a
gui_deps = libmuleappgui.a
remote_common_deps = libs/common/libmulecommon.a libs/ec/cpp/libec.a
common_deps = libmuleappcommon.a $(remote_common_deps)

if SYS_WIN32
core_libs += -lshlwapi
endif

#
# Flags
#
core_flags = $(LIBUPNP_CPPFLAGS) $(LIBUPNP_CFLAGS)
gui_flags = $(WX_CPPFLAGS) $(GEOIP_CPPFLAGS)
common_flags = -I$(srcdir)/libs -Ilibs -I$(srcdir)/include $(CRYPTOPP_CPPFLAGS)

# --------- Apps ---------

amulegui_SOURCES = \
	amule-remote-gui.cpp \
	$(gui_sources) \
	$(ec_sources) \
	$(common_sources)

amule_SOURCES = \
	CaptchaDialog.cpp \
	CaptchaGenerator.cpp \
	PartFileConvert.cpp \
	PartFileConvertDlg.cpp \
	$(core_sources) \
	$(gui_sources) \
	$(ec_sources) \
	$(common_sources)

amuled_SOURCES = \
	amuled.cpp \
	$(core_sources) \
	$(ec_sources) \
	$(common_sources)

amule_DEPENDENCIES = $(common_deps) $(core_deps) $(gui_deps)
amule_CPPFLAGS = $(AM_CPPFLAGS) $(core_flags) $(gui_flags) $(common_flags)
amule_CXXFLAGS = $(AM_CXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CPPFLAGS_ONLY)
amule_LDADD = $(common_libs) $(core_libs) $(gui_libs)

amulegui_DEPENDENCIES = $(common_deps) $(gui_deps)
amulegui_CPPFLAGS = $(AM_CPPFLAGS) $(gui_flags) $(common_flags) -DCLIENT_GUI
amulegui_CXXFLAGS = $(AM_CXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)
amulegui_LDADD = $(gui_libs) $(common_libs) $(WXBASE_LIBS)

amuled_DEPENDENCIES = $(core_deps) $(common_deps)
amuled_CPPFLAGS = $(AM_CPPFLAGS) $(WXBASE_CPPFLAGS) $(core_flags) $(common_flags) -DAMULE_DAEMON 
amuled_CXXFLAGS = $(AM_CXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)
amuled_LDADD = $(common_libs) $(core_libs) $(WXBASE_LIBS)

ed2k_SOURCES = \
	ED2KLinkParser.cpp \
	MagnetURI.cpp \
	MuleCollection.cpp	

ed2k_CPPFLAGS = $(AM_CPPFLAGS) -DUSE_STD_STRING

# on Win32
if SYS_WIN32
ed2k_LDADD = -lshlwapi
endif
# on Mac
if NEED_CORESERVICES
ed2k_LDFLAGS = $(AM_LDFLAGS) -framework CoreServices
endif

amulecmd_SOURCES = \
	TextClient.cpp \
	ExternalConnector.cpp \
	LoggerConsole.cpp \
	OtherFunctions.cpp \
	NetworkFunctions.cpp 

amulecmd_DEPENDENCIES = $(remote_common_deps)
amulecmd_CPPFLAGS = $(AM_CPPFLAGS) $(WXBASE_CPPFLAGS) $(common_flags)
amulecmd_CXXFLAGS = $(AM_CXXFLAGS) $(WX_CFLAGS_ONLY) $(WX_CXXFLAGS_ONLY)
#amulecmd_LDADD = $(WXBASE_LIBS) $(READLINE_LIBS) $(remote_common_libs)
amulecmd_LDADD = -Llibs/common -Llibs/ec/cpp -lmulecommon -lec \
	$(WXBASE_LIBS) $(READLINE_LIBS) $(BFD_LIBS) $(ZLIB_LDFLAGS) $(ZLIB_LIBS) $(RESOLV_LIB)

noinst_HEADERS = \
		AddFriend.h \
		AsyncDNS.h \
		amule-remote-gui.h \
		amuleDlg.h \
		amule.h \
		amuleIPV4Address.h \
		ArchSpecific.h \
		BarShader.h \
		BitVector.h \
		CanceledFileList.h \
		CaptchaDialog.h \
		CaptchaGenerator.h \
		CatDialog.h \
		CFile.h \
		ChatSelector.h \
		ChatWnd.h \
		ClientCredits.h \
		ClientCreditsList.h \
		ClientDetailDialog.h \
		SharedFilePeersListCtrl.h \
		ClientList.h \
		ClientRef.h \
		ClientTCPSocket.h \
		ClientUDPSocket.h \
		ColorFrameCtrl.h \
		CommentDialog.h \
		CommentDialogLst.h \
		Constants.h \
		CorruptionBlackBox.h \
		CryptoPP_Inc.h \
		DataToText.h \
		DeadSourceList.h \
		DirectoryTreeCtrl.h \
		DownloadListCtrl.h \
		DownloadQueue.h \
		ED2KLink.h \
		EditServerListDlg.h \
		EMSocket.h \
		EncryptedDatagramSocket.h \
		EncryptedStreamSocket.h \
		ExternalConnector.h \
		ExternalConn.h \
		FileArea.h \
		FileAutoClose.h \
		FileDetailDialog.h \
		FileDetailListCtrl.h \
		FileLock.h \
		Friend.h \
		FriendListCtrl.h \
		FriendList.h \
		GapList.h \
		GetTickCount.h \
		GenericClientListCtrl.h \
		GuiEvents.h \
		HTTPDownload.h \
		inetdownload.h \
		InternalEvents.h \
		IP2Country.h \
		IPFilter.h \
		IPFilterScanner.h \
		KadDlg.h \
		KnownFile.h \
		KnownFileList.h \
		ListenSocket.h \
		Logger.h \
		MagnetURI.h \
		MD4Hash.h \
		MemFile.h \
		MuleCollection.h \
		MuleColour.h \
		MuleGifCtrl.h \
		MuleListCtrl.h \
		MuleNotebook.h \
		MuleTextCtrl.h \
		MuleThread.h \
		MuleTrayIcon.h \
		MuleUDPSocket.h \
		muuli_wdr.h \
		NetworkFunctions.h \
		OScopeCtrl.h \
		Observable.h \
		ObservableQueue.h \
		OtherFunctions.h \
		OtherStructs.h \
		Packet.h \
		Parser.hpp \
		PartFileConvert.h \
		PartFileConvertDlg.h \
		PartFile.h \
		PlatformSpecific.h \
		Preferences.h \
		PrefsUnifiedDlg.h \
		Proxy.h \
		RangeMap.h \
		RC4Encrypt.h \
		RLE.h \
		RandomFunctions.h \
		SafeFile.h \
		Scanner.h \
		ScopedPtr.h \
		SearchDlg.h \
		SearchExpr.h \
		SearchFile.h \
		SearchListCtrl.h \
		SearchList.h \
		ServerConnect.h \
		Server.h \
		ServerListCtrl.h \
		ServerList.h \
		ServerSocket.h \
		ServerUDPSocket.h \
		ServerWnd.h \
		SHA.h \
		SHAHashSet.h \
		SharedFileList.h \
		SharedFilesCtrl.h \
		SharedFilesWnd.h \
		SourceListCtrl.h \
		StateMachine.h \
		StatisticsDlg.h \
		Statistics.h \
		StatTree.h \
		Tag.h \
		TerminationProcess.h \
		TerminationProcessAmuleweb.h \
		TextClient.h \
		ThreadScheduler.h \
		ThreadTasks.h \
		ThrottledSocket.h \
		Timer.h \
		TransferWnd.h \
		Types.h \
		updownclient.h \
		UpDownClientEC.h \
		UploadBandwidthThrottler.h \
		UploadQueue.h \
		UPnPBase.h \
		UPnPCompatibility.h \
		UserEvents.h \
		extern/wxWidgets/listctrl.h

MAINTAINERCLEANFILES = Makefile.in

$(srcdir)/Parser.cpp: Parser.y
	bison --debug -t -d -v -o $@ $(srcdir)/Parser.y

if GENERATE_FLEX_HEADER
$(srcdir)/Scanner.cpp: Scanner.l Parser.cpp
		$(LEX) --header-file=$(srcdir)/Scanner.h -o $@ $(srcdir)/Scanner.l
else
$(srcdir)/Scanner.cpp: Scanner.l Parser.cpp
		$(LEX) -o $@ $(srcdir)/Scanner.l; \
		echo "// Empty file generated by a flex version unable to create headers" > $(srcdir)/Scanner.h
endif

$(srcdir)/IPFilterScanner.cpp: IPFilterScanner.l
		$(LEX) -Pyyip -o $@ $(srcdir)/IPFilterScanner.l;

# Resources

if NEED_RC

SUFFIXES = .rc

.rc.$(OBJEXT):
	$(RC) $(MULERCFLAGS) -I$(top_srcdir) $(RCFLAGS) -O COFF -i "$<" -o "$@"

nodist_amuled_SOURCES = amuled-version.rc
nodist_amulecmd_SOURCES = amulecmd-version.rc
nodist_ed2k_SOURCES = ed2k-version.rc
amule_LDADD += amulerc.$(OBJEXT)
amule_DEPENDENCIES += amulerc.$(OBJEXT)
amulegui_LDADD += amuleguirc.$(OBJEXT)
amulegui_DEPENDENCIES += amuleguirc.$(OBJEXT)

amulerc.$(OBJEXT): $(srcdir)/../amule.rc amule-version.rc
	cat $^ | \
	( \
		abs_builddir=`pwd` ; \
		abs_top_srcdir=`cd $(top_srcdir) ; pwd` ; \
		cd $(<D) ; \
		$(RC) $(MULERCFLAGS) -I$${abs_top_srcdir} $(RCFLAGS) -O COFF -o "$${abs_builddir}/$@" ; \
	)

amuleguirc.$(OBJEXT): $(srcdir)/../amule.rc amulegui-version.rc
	cat $^ | \
	( \
		abs_builddir=`pwd` ; \
		abs_top_srcdir=`cd $(top_srcdir) ; pwd` ; \
		cd $(<D) ; \
		$(RC) $(MULERCFLAGS) -I$${abs_top_srcdir} $(RCFLAGS) -O COFF -o "$${abs_builddir}/$@" ; \
	)

amule-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"All-Platform P2P Client Based on eMule"/' \
	     -e 's/VER_INTERNALNAME_STR/"amule"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"amule$(EXEEXT)"/' "$<" > "$@"

amuled-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"aMule Daemon"/' \
	     -e 's/VER_INTERNALNAME_STR/"amuled"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"amuled$(EXEEXT)"/' "$<" > "$@"

amulecmd-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"aMule Text Client"/' \
	     -e 's/VER_INTERNALNAME_STR/"amulecmd"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"amulecmd$(EXEEXT)"/' "$<" > "$@"

amulegui-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"aMule Remote GUI (Experimental)"/' \
	     -e 's/VER_INTERNALNAME_STR/"amulegui"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"amulegui$(EXEEXT)"/' "$<" > "$@"

ed2k-version.rc: $(top_builddir)/version.rc
	@sed -e 's/VER_FILEDESCRIPTION_STR/"ED2K Links Handler"/' \
	     -e 's/VER_INTERNALNAME_STR/"ed2k"/' \
	     -e 's/VER_ORIGINALFILENAME_STR/"ed2k$(EXEEXT)"/' "$<" > "$@"

CLEANFILES = *-version.rc

endif


EXTRA__DIST__SUBDIRS = include kademlia
include $(top_srcdir)/automake/dist-hook.am

