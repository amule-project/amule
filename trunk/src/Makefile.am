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

kad_sources = kademlia/io/ByteIO.cpp \
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
	kademlia/routing/Ping.cpp \
	kademlia/routing/RoutingBin.cpp \
	kademlia/routing/RoutingZone.cpp \
	kademlia/utils/LittleEndian.cpp

amulegui_SOURCES = \
	amule-gui.cpp \
	amule-remote-gui.cpp \
	amuleDlg.cpp \
	AddFriend.cpp \
	BarShader.cpp \
	CatDialog.cpp \
	CFile.cpp \
	ChatSelector.cpp \
	ChatWnd.cpp \
	ClientCredits.cpp \
	ClientDetailDialog.cpp \
	ClientListCtrl.cpp \
	ColorFrameCtrl.cpp \
	CommentDialog.cpp \
	CommentDialogLst.cpp \
	DataToText.cpp \
	DirectoryTreeCtrl.cpp \
	DownloadListCtrl.cpp \
	ED2KLink.cpp \
	ECSocket.cpp \
	ECPacket.cpp \
	ECSpecialTags.cpp \
	EditServerListDlg.cpp \
	FileDetailDialog.cpp \
	FileDetailListCtrl.cpp \
	FileFunctions.cpp \
	Format.cpp \
	FriendListCtrl.cpp \
	KadDlg.cpp \
	KadSearchListCtrl.cpp \
	KnownFile.cpp \
	listctrl.cpp \
	Logger.cpp \
	MD5Sum.cpp \
	MemFile.cpp \
	MuleDebug.cpp \
	MuleGifCtrl.cpp \
	MuleListCtrl.cpp \
	MuleNotebook.cpp \
	MuleTextCtrl.cpp \
	muuli_wdr.cpp \
	NetworkFunctions.cpp \
	OtherFunctions.cpp \
	OScopeCtrl.cpp \
	PartFile.cpp \
	Packet.cpp \
	Preferences.cpp \
	PrefsUnifiedDlg.cpp \
	Proxy.cpp \
	RLE.cpp \
	SHA.cpp \
	SHAHashSet.cpp \
	SafeFile.cpp \
	SearchDlg.cpp \
	SearchListCtrl.cpp \
	Server.cpp \
	ServerWnd.cpp \
	SharedFilesCtrl.cpp \
	SharedFilesWnd.cpp \
	ServerListCtrl.cpp \
	Statistics.cpp \
	StatisticsDlg.cpp \
	StatTree.cpp \
	StringFunctions.cpp \
	TransferWnd.cpp 

amule_SOURCES = \
	AddFriend.cpp \
	AddFileThread.cpp \
	AICHSyncThread.cpp \
	amule.cpp \
	amule-gui.cpp \
	amuleDlg.cpp \
	BarShader.cpp \
	BaseClient.cpp \
	ChatSelector.cpp \
	ChatWnd.cpp \
	CatDialog.cpp \
	CFile.cpp \
	ClientCredits.cpp \
	ClientDetailDialog.cpp \
	ClientList.cpp \
	ClientListCtrl.cpp \
	ClientUDPSocket.cpp \
	ColorFrameCtrl.cpp \
	CommentDialog.cpp \
	CommentDialogLst.cpp \
	DataToText.cpp \
	DeadSourceList.cpp \
	DirectoryTreeCtrl.cpp \
	DownloadClient.cpp \
	DownloadListCtrl.cpp \
	DownloadQueue.cpp \
	ECSocket.cpp \
	ECPacket.cpp \
	ECSpecialTags.cpp \
	ED2KLink.cpp \
	EditServerListDlg.cpp \
	EMSocket.cpp \
	ExternalConn.cpp \
	FileDetailDialog.cpp \
	FileDetailListCtrl.cpp \
	FileFunctions.cpp \
	Format.cpp \
	Friend.cpp \
	FriendList.cpp \
	FriendListCtrl.cpp \
	HTTPDownload.cpp \
	IPFilter.cpp \
	KnownFile.cpp \
	KadDlg.cpp \
	KadSearchListCtrl.cpp \
	KnownFileList.cpp \
	ListenSocket.cpp \
	listctrl.cpp \
	Logger.cpp \
	MemFile.cpp \
	MD5Sum.cpp \
	MuleDebug.cpp \
	MuleGifCtrl.cpp \
	MuleListCtrl.cpp \
	MuleNotebook.cpp \
	MuleTextCtrl.cpp \
	muuli_wdr.cpp \
	NetworkFunctions.cpp \
	OScopeCtrl.cpp \
	OtherFunctions.cpp \
	Packet.cpp \
	PartFile.cpp \
	PartFileConvert.cpp \
	Preferences.cpp \
	PrefsUnifiedDlg.cpp \
	Proxy.cpp \
	RLE.cpp \
	SafeFile.cpp \
	SearchDlg.cpp \
	SearchList.cpp \
	SearchListCtrl.cpp \
	Server.cpp \
	ServerConnect.cpp \
	ServerList.cpp \
	ServerListCtrl.cpp \
	ServerSocket.cpp \
	ServerUDPSocket.cpp \
	ServerWnd.cpp \
	SharedFileList.cpp \
	SharedFilesCtrl.cpp \
	SharedFilesWnd.cpp \
	SHA.cpp \
	SHAHashSet.cpp \
	StateMachine.cpp \
	Statistics.cpp \
	StatisticsDlg.cpp \
	StatTree.cpp \
	StringFunctions.cpp \
	TransferWnd.cpp \
	UploadBandwidthThrottler.cpp \
	UploadClient.cpp \
	UploadQueue.cpp \
	kademlia/utils/UInt128.cpp


amuled_SOURCES = \
	AICHSyncThread.cpp \
	AddFileThread.cpp \
	amule.cpp \
	amuled.cpp \
	BaseClient.cpp \
	CFile.cpp \
	ClientCredits.cpp \
	ClientList.cpp \
	ClientUDPSocket.cpp \
	DataToText.cpp \
	DeadSourceList.cpp \
	DownloadClient.cpp \
	DownloadQueue.cpp \
	ECSocket.cpp \
	ECPacket.cpp \
	ECSpecialTags.cpp \
	ED2KLink.cpp \
	EMSocket.cpp \
	ExternalConn.cpp \
	FileFunctions.cpp \
	Format.cpp \
	Friend.cpp \
	FriendList.cpp \
	HTTPDownload.cpp \
	IPFilter.cpp \
	KnownFile.cpp \
	KnownFileList.cpp \
	ListenSocket.cpp \
	Logger.cpp \
	MD5Sum.cpp \
	MemFile.cpp \
	MuleDebug.cpp \
	NetworkFunctions.cpp \
	OtherFunctions.cpp \
	Packet.cpp \
	PartFile.cpp \
	PartFileConvert.cpp \
	Preferences.cpp \
	Proxy.cpp \
	RLE.cpp \
	SafeFile.cpp \
	SearchList.cpp \
	Server.cpp \
	ServerConnect.cpp \
	ServerList.cpp \
	ServerSocket.cpp \
	ServerUDPSocket.cpp \
	SHA.cpp \
	SHAHashSet.cpp \
	SharedFileList.cpp \
	StateMachine.cpp \
	Statistics.cpp \
	StatTree.cpp \
	StringFunctions.cpp \
	Timer.cpp \
	UploadBandwidthThrottler.cpp \
	UploadClient.cpp \
	UploadQueue.cpp \
	kademlia/utils/UInt128.cpp

if ADD_GSOCKET
if ADD_24GSOCKET
amule_SOURCES += gsocket.c 
else
amule_SOURCES += gsocket-2.5.c
endif
endif

if HAVE_TRAY
if HAVE_OLD_TRAY
amule_SOURCES += eggtrayicon.c gtk2-funcs.c gtkplugxembed.c SysTray.cpp 
amulegui_SOURCES += eggtrayicon.c gtk2-funcs.c gtkplugxembed.c SysTray.cpp
else
amule_SOURCES += MuleTrayIcon.cpp
amulegui_SOURCES += MuleTrayIcon.cpp
endif
endif

if USE_EMBEDDED_CRYPTO
amule_SOURCES += CryptoPP.cpp
amuled_SOURCES += CryptoPP.cpp
amulegui_SOURCES += CryptoPP.cpp
endif

amule_CFLAGS = $(UTF8_SYSTRAY_FLAGS) $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS) $(BFD_FLAGS)
amule_CXXFLAGS = $(UTF8_SYSTRAY_FLAGS) $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS) $(BFD_FLAGS)
amule_LDADD = $(WX_LIBS) $(ZLIB_LIBS) $(CRYPTOLIBS) $(RESOLV_LIB) $(BFD_LIB) $(X11LIBS)

if COMPILE_KAD
amule_SOURCES += $(kad_sources)
amule_CFLAGS += -D__COMPILE_KAD__
amule_CXXFLAGS += -D__COMPILE_KAD__
endif

amulegui_CFLAGS = $(UTF8_SYSTRAY_FLAGS) $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS) $(BFD_FLAGS) -DCLIENT_GUI -DEC_REMOTE -DNO_GCC_PRAGMA
amulegui_CXXFLAGS = $(UTF8_SYSTRAY_FLAGS) $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS) $(BFD_FLAGS) -DCLIENT_GUI -DEC_REMOTE -DNO_GCC_PRAGMA
amulegui_LDADD = $(WX_LIBS) $(ZLIB_LIBS) $(CRYPTOLIBS) $(BFD_LIB) $(X11LIBS)

if COMPILE_KAD
amulegui_CFLAGS += -D__COMPILE_KAD__
amulegui_CXXFLAGS += -D__COMPILE_KAD__
endif

if HAVE_TRAY
if HAVE_OLD_TRAY
amule_LDADD += $(GTK_LIBS) 
amulegui_LDADD += $(GTK_LIBS) 
endif
endif

amuled_CFLAGS = $(WXBASE_CFLAGS) $(BFD_FLAGS) -DAMULE_DAEMON 
amuled_CXXFLAGS = $(WXBASE_CXXFLAGS) $(BFD_FLAGS) -DAMULE_DAEMON 
amuled_LDADD = $(WXBASE_LIBS) $(ZLIB_LIBS) $(CRYPTOLIBS) $(RESOLV_LIB) $(BFD_LIB)

if COMPILE_KAD
amuled_SOURCES += $(kad_sources)
amuled_CFLAGS += -D__COMPILE_KAD__
amuled_CXXFLAGS += -D__COMPILE_KAD__
endif

ed2k_SOURCES = ED2KLinkParser.cpp
ed2k_CFLAGS =

amulecmd_SOURCES = TextClient.cpp MD5Sum.cpp ECSocket.cpp ExternalConnector.cpp ECPacket.cpp ECSpecialTags.cpp OtherFunctions.cpp StringFunctions.cpp Format.cpp MuleDebug.cpp
amulecmd_CFLAGS = $(WXBASE_CFLAGS) $(BFD_FLAGS) -DEC_REMOTE
amulecmd_CXXFLAGS = $(WXBASE_CXXFLAGS) $(BFD_FLAGS) -DEC_REMOTE
amulecmd_LDADD = $(WXBASE_LIBS) $(READLINE_LIBS) $(ZLIB_LIBS) $(RESOLV_LIB) $(BFD_LIB)

amulecmdDLG_SOURCES = TextClient.cpp MD5Sum.cpp ECSocket.cpp ExternalConnector.cpp ECPacket.cpp ECSpecialTags.cpp OtherFunctions.cpp StringFunctions.cpp Format.cpp MuleDebug.cpp
amulecmdDLG_CFLAGS = $(WX_CFLAGS) $(BFD_FLAGS) -DAMULECMDDLG -DEC_REMOTE
amulecmdDLG_CXXFLAGS = $(WX_CXXFLAGS) $(BFD_FLAGS) -DAMULECMDDLG  -DEC_REMOTE
amulecmdDLG_LDADD = $(WX_LIBS) $(ZLIB_LIBS) $(RESOLV_LIB) $(BFD_LIB)

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
		 muuli_wdr.h \
		 NetworkFunctions.h \
		 OPCodes.h \
		 OScopeCtrl.h \
		 OtherFunctions.h \
		 OtherStructs.h \
		 Packet.h \
		 PartFile.h \
		 PartFileConvert.h \
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
amule_LDADD += amulerc.$(OBJEXT)
amulegui_LDADD += amulerc.$(OBJEXT)

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
CLEANFILES = ECVersion.h

ECVersion.h: utils/mkFileSum$(EXEEXT) utils/scripts/gen_ECVersion ECcodes.h
	@$(top_srcdir)/src/utils/scripts/gen_ECVersion $(top_srcdir)

utils/mkFileSum$(EXEEXT): utils/mkFileSum.c
	$(CC) -o $@ $<
