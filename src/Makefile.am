if WEB
  OPT_WEB = webserver
else
if WEBGUI
  OPT_WEB = webserver
endif
endif

SUBDIRS = utils pixmaps kademlia $(OPT_WEB)

EXTRA_DIST = Makengfile aMule.xpm

bin_PROGRAMS = 

if GUI
bin_PROGRAMS += amulecmdDLG
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

if MONOLITHIC
bin_PROGRAMS += amule
endif

DEFS = -DHAVE_CONFIG_H

# Sources

core_sources = \
	AICHSyncThread.cpp \
	AddFileThread.cpp \
	amule.cpp \
	BaseClient.cpp \
	ClientList.cpp \
	ClientCreditsList.cpp \
	ClientUDPSocket.cpp \
	DeadSourceList.cpp \
	DownloadClient.cpp \
	DownloadQueue.cpp \
	EMSocket.cpp \
	ExternalConn.cpp \
	Friend.cpp \
	FriendList.cpp \
	HTTPDownload.cpp \
	IPFilter.cpp \
	KnownFileList.cpp \
	ListenSocket.cpp \
	MuleUDPSocket.cpp \
	PartFileConvert.cpp \
	SearchList.cpp \
	ServerConnect.cpp \
	ServerList.cpp \
	ServerSocket.cpp \
	ServerUDPSocket.cpp \
	SharedFileList.cpp \
	StateMachine.cpp \
	UploadBandwidthThrottler.cpp \
	UploadClient.cpp \
	UploadQueue.cpp \
	kademlia/io/ByteIO.cpp \
	kademlia/io/DataIO.cpp \
	kademlia/io/FileIO.cpp \
	kademlia/io/IOException.cpp \
	kademlia/kademlia/Indexed.cpp \
	kademlia/kademlia/Kademlia.cpp \
	kademlia/kademlia/Prefs.cpp \
	kademlia/kademlia/Search.cpp \
	kademlia/kademlia/SearchManager.cpp \
	kademlia/net/KademliaUDPListener.cpp \
	kademlia/routing/Contact.cpp \
	kademlia/routing/RoutingBin.cpp \
	kademlia/routing/RoutingZone.cpp \
	kademlia/utils/LittleEndian.cpp 

gui_sources = \
	amule-gui.cpp \
	amuleDlg.cpp \
	AddFriend.cpp \
	BarShader.cpp \
	CatDialog.cpp \
	ChatSelector.cpp \
	ChatWnd.cpp \
	ClientDetailDialog.cpp \
	ClientListCtrl.cpp \
	ColorFrameCtrl.cpp \
	CommentDialog.cpp \
	CommentDialogLst.cpp \
	DirectoryTreeCtrl.cpp \
	DownloadListCtrl.cpp \
	EditServerListDlg.cpp \
	FileDetailDialog.cpp \
	FileDetailListCtrl.cpp \
	FriendListCtrl.cpp \
	KadDlg.cpp \
	listctrl.cpp \
	MuleGifCtrl.cpp \
	MuleListCtrl.cpp \
	MuleNotebook.cpp \
	MuleTextCtrl.cpp \
	muuli_wdr.cpp \
	OScopeCtrl.cpp \
	PrefsUnifiedDlg.cpp \
	SearchDlg.cpp \
	SearchListCtrl.cpp \
	ServerWnd.cpp \
	SharedFilesCtrl.cpp \
	SharedFilesWnd.cpp \
	ServerListCtrl.cpp \
	StatisticsDlg.cpp \
	TransferWnd.cpp 

ec_sources = \
	ECSocket.cpp \
	ECPacket.cpp \
	ECSpecialTags.cpp

remote_common_sources = \
	Format.cpp \
	MD5Sum.cpp \
	MuleDebug.cpp \
	OtherFunctions.cpp \
	PlatformSpecific.cpp \
	StringFunctions.cpp

common_sources = \
	CFile.cpp \
	ClientCredits.cpp \
	DataToText.cpp \
	ED2KLink.cpp \
	FileFunctions.cpp \
	KnownFile.cpp \
	Logger.cpp \
	MemFile.cpp \
	NetworkFunctions.cpp \
	PartFile.cpp \
	Packet.cpp \
	Preferences.cpp \
	Proxy.cpp \
	RLE.cpp \
	SafeFile.cpp \
	Server.cpp \
	Statistics.cpp \
	StatTree.cpp \
	SHA.cpp \
	SHAHashSet.cpp \
	kademlia/utils/UInt128.cpp \
	$(remote_common_sources)

# Libs

core_libs = 
gui_libs =  $(X11LIBS) $(WX_LIBS)
remote_common_libs = $(ZLIB_LIBS) $(RESOLV_LIB) $(BFD_LIB) 
common_libs = $(CRYPTOLIBS) $(remote_common_libs)

# Flags

core_flags = 
gui_flags = $(WX_CXXFLAGS) $(TRAY_FLAGS)  $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS)
common_flags = $(BFD_FLAGS) 

# Extras 

if ADD_GSOCKET
if ADD_24GSOCKET
core_sources += gsocket.c 
else
core_sources += gsocket-2.5.c
endif
endif

if USE_EMBEDDED_CRYPTO
common_sources += CryptoPP.cpp
endif

if HAVE_TRAY
if HAVE_OLD_TRAY
gui_sources += eggtrayicon.c gtk2-funcs.c gtkplugxembed.c SysTray.cpp 
gui_libs += $(GTK_LIBS) 
else
gui_sources += MuleTrayIcon.cpp
endif
endif

if NEED_RC
gui_libs += amulerc.$(OBJEXT)
endif

# --------- Apps ---------

amulegui_SOURCES = \
	amule-remote-gui.cpp \
	$(gui_sources) \
	$(ec_sources) \
	$(common_sources)

amule_SOURCES = \
	$(core_sources) \
	$(gui_sources) \
	$(ec_sources) \
	$(common_sources)

amuled_SOURCES = \
	amuled.cpp \
	Timer.cpp \
	$(core_sources) \
	$(ec_sources) \
	$(common_sources)

amule_CFLAGS = $(core_flags) $(gui_flags) $(common_flags)
amule_CXXFLAGS = $(core_flags) $(gui_flags) $(common_flags)
amule_LDADD = $(core_libs) $(gui_libs) $(common_libs)

amulegui_CFLAGS = $(gui_flags) $(common_flags) -DCLIENT_GUI -DEC_REMOTE
amulegui_CXXFLAGS = $(gui_flags) $(common_flags) -DCLIENT_GUI -DEC_REMOTE
amulegui_LDADD = $(gui_libs) $(common_libs)

amuled_CFLAGS = $(WXBASE_CFLAGS) $(core_flags) $(common_flags) -DAMULE_DAEMON 
amuled_CXXFLAGS = $(WXBASE_CXXFLAGS) $(core_flags) $(common_flags) -DAMULE_DAEMON 
amuled_LDADD = $(WXBASE_LIBS) $(core_libs) $(common_libs)

ed2k_SOURCES = ED2KLinkParser.cpp
# on Win32
if NEED_RC
ed2k_LDADD = -lshlwapi
endif
# on Mac
if NEED_CORESERVICES
ed2k_LDFLAGS = -framework CoreServices
endif

cmd_sources = \
	TextClient.cpp \
	ExternalConnector.cpp \
	$(remote_common_sources) \
	$(ec_sources)

amulecmd_SOURCES = $(cmd_sources) 
amulecmd_CFLAGS = $(WXBASE_CFLAGS) $(common_flags) -DEC_REMOTE
amulecmd_CXXFLAGS = $(WXBASE_CXXFLAGS) $(common_flags) -DEC_REMOTE
amulecmd_LDADD = $(WXBASE_LIBS) $(READLINE_LIBS) $(remote_common_libs)

amulecmdDLG_SOURCES = $(cmd_sources) 
amulecmdDLG_CFLAGS = $(WX_CFLAGS) $(common_flags) -DAMULECMDDLG -DEC_REMOTE
amulecmdDLG_CXXFLAGS = $(WX_CXXFLAGS) $(common_flags) -DAMULECMDDLG  -DEC_REMOTE
amulecmdDLG_LDADD = $(WX_LIBS) $(remote_common_libs)

noinst_HEADERS = AddFileThread.h \
		 AddFriend.h \
		 AICHSyncThread.h \
		 amuleDlg.h \
		 amule.h \
		 amuleIPV4Address.h \
		 BarShader.h \
		 CatDialog.h \
		 CFile.h \
		 ChatSelector.h \
		 ChatWnd.h \
		 ClientCredits.h \
		 ClientCreditsList.h \
		 ClientDetailDialog.h \
		 ClientList.h \
		 ClientListCtrl.h \
		 ClientUDPSocket.h \
		 CMD4Hash.h \
		 ColorFrameCtrl.h \
		 Color.h \
		 CommentDialog.h \
		 CommentDialogLst.h \
		 CryptoPP.h \
		 CryptoPP_Inc.h \
		 CTypedPtrList.h \
		 DirectoryTreeCtrl.h \
		 DownloadListCtrl.h \
		 DownloadQueue.h \
		 ECFileConfig.h \
		 ECSocket.h \
		 ECPacket.h \
		 ECSpecialTags.h \
		 ECcodes.h \
		 ED2KLink.h \
		 EditServerListDlg.h \
		 eggtrayicon.h \
		 EMSocket.h \
		 ArchSpecific.h \
		 ExternalConnector.h \
		 ExternalConn.h \
		 FileDetailDialog.h \
		 FileDetailListCtrl.h \
		 FileFunctions.h \
		 Friend.h \
		 FriendList.h \
		 FriendListCtrl.h \
		 GetTickCount.h \
		 gsocket-fix.h \
		 gsocket.h \
		 gtk2-funcs.h \
		 gtkplugxembed.h \
		 GuiEvents.h \
		 HTTPDownload.h \
		 inetdownload.h \
		 InternalEvents.h \
		 IPFilter.h \
		 KadDlg.h \
		 KnownFile.h \
		 KnownFileList.h \
		 listctrl.h \
		 extern/listctrl.242.h \
		 extern/listctrl.260.h \
		 ListenSocket.h \
		 MD5Sum.h \
		 MemFile.h \
		 MuleDebug.h \
		 MuleGifCtrl.h \
		 MuleListCtrl.h \
		 MuleNotebook.h \
		 MuleTextCtrl.h \
		 MuleTrayIcon.h \
		 MuleUDPSocket.h \
		 muuli_wdr.h \
		 NetworkFunctions.h \
		 OPCodes.h \
		 OScopeCtrl.h \
		 OtherFunctions.h \
		 OtherStructs.h \
		 Packet.h \
		 PartFile.h \
		 PartFileConvert.h \
		 PlatformSpecific.h \
		 Preferences.h \
		 PrefsUnifiedDlg.h \
		 Proxy.h \
		 SafeFile.h \
		 SearchDlg.h \
		 SearchListCtrl.h \
		 SearchList.h \
		 Server.h \
		 ServerConnect.h \
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
		 StateMachine.h \
		 Statistics.h \
		 StatisticsDlg.h \
		 StatTree.h \
		 StringFunctions.h \
		 Format.h \
		 Logger.h \
		 SysTray.h \
		 TextClient.h \
		 Timer.h \
		 TransferWnd.h \
		 Types.h \
		 updownclient.h \
		 UploadBandwidthThrottler.h \
		 UploadQueue.h \
		 xembed.h


MAINTAINERCLEANFILES = Makefile.in

if NEED_RC

amulerc.$(OBJEXT): $(srcdir)/../amule.rc
	@thisdir=`pwd` ; \
	cd $(srcdir)/.. ; \
	rcfile=`basename "$<"` ; \
	echo "windres $(RCFLAGS) --define __WIN95__ --define __WIN32__ --define __GNUWIN32__ -O COFF -i "$${rcfile}" -o \"$@\"" ; \
	windres $(RCFLAGS) --define __WIN95__ --define __WIN32__ --define __GNUWIN32__ -O COFF -i "$${rcfile}" -o "$@" ; \
	cd $${thisdir} ; \
	mv "$(srcdir)/../$@" "$@"
	
endif

BUILT_SOURCES = ECVersion.h
CLEANFILES = ECVersion.h EC-timestamp utils/mkFileSum$(EXEEXT)

utils/mkFileSum$(EXEEXT): utils/mkFileSum.c
	$(CC) -o $@ $<

ECVersion.h: EC-timestamp
	@if test ! -f $@; then \
	  rm -f EC-timestamp; \
	  $(MAKE) EC-timestamp; \
	else :; fi

EC-timestamp: utils/mkFileSum$(EXEEXT) utils/scripts/gen_ECVersion ECcodes.h
	@echo "ECVersion.h timestamp file" > EC-timestamp
	@$(top_srcdir)/src/utils/scripts/gen_ECVersion $(top_srcdir)
