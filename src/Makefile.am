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

if WEB
bin_PROGRAMS += amuleweb
endif

if WEBGUI
bin_PROGRAMS += amulewebDLG
endif

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

BUILT_SOURCES =

#LIBS = @LIBS@ 
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
	Format.cpp \
	FriendListCtrl.cpp \
	KadDlg.cpp \
	KnownFile.cpp \
	listctrl.cpp \
	Logger.cpp \
	MD5Sum.cpp \
	MemFile.cpp \
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
	filefn.cpp \
	Format.cpp \
	Friend.cpp \
	FriendList.cpp \
	FriendListCtrl.cpp \
	HTTPDownload.cpp \
	IPFilter.cpp \
	KnownFile.cpp \
	KadDlg.cpp \
	KnownFileList.cpp \
	ListenSocket.cpp \
	listctrl.cpp \
	Logger.cpp \
	MemFile.cpp \
	MD5Sum.cpp \
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
	Preferences.cpp \
	PrefsUnifiedDlg.cpp \
	Proxy.cpp \
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
	Statistics.cpp \
	StatisticsDlg.cpp \
	StateMachine.cpp \
	StringFunctions.cpp \
	TransferWnd.cpp \
	UploadClient.cpp \
	UploadQueue.cpp \
	kademlia/io/ByteIO.cpp \
	kademlia/io/DataIO.cpp \
	kademlia/io/FileIO.cpp \
	kademlia/io/IOException.cpp \
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
	DeadSourceList.cpp \
        DownloadClient.cpp \
        DownloadQueue.cpp \
        ECSocket.cpp \
	ECPacket.cpp \
	ECSpecialTags.cpp \
        ED2KLink.cpp \
        EMSocket.cpp \
        ExternalConn.cpp \
        filefn.cpp \
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
	NetworkFunctions.cpp \
        OtherFunctions.cpp \
        Packet.cpp \
        PartFile.cpp \
        Preferences.cpp \
	Proxy.cpp \
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
	StringFunctions.cpp \
	Timer.cpp \
        UploadClient.cpp \
        UploadQueue.cpp \
	kademlia/io/ByteIO.cpp \
	kademlia/io/DataIO.cpp \
	kademlia/io/FileIO.cpp \
	kademlia/io/IOException.cpp \
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
endif

amule_CFLAGS = $(UTF8_SYSTRAY_FLAGS) $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS)
amule_CXXFLAGS = $(UTF8_SYSTRAY_FLAGS) $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS) $(CURL_FLAGS)
amule_LDADD = $(WX_LIBS) $(ZLIB_LIBS) $(CURL_LIBS) $(CRYPTOLIBS) $(RESOLV_LIB)

amulegui_CFLAGS = $(UTF8_SYSTRAY_FLAGS) $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS) -DCLIENT_GUI -DEC_REMOTE -DNO_GCC_PRAGMA
amulegui_CXXFLAGS = $(UTF8_SYSTRAY_FLAGS) $(TRAY_FLAGS) $(WX_CXXFLAGS) $(GTK_CFLAGS) $(GTK_DEFS) $(XRCFLAGS) $(CURL_FLAGS) -DCLIENT_GUI -DEC_REMOTE -DNO_GCC_PRAGMA
amulegui_LDADD = $(WX_LIBS) $(GTK_LIBS) $(ZLIB_LIBS) $(CURL_LIBS) $(CRYPTOLIBS)

if HAVE_TRAY
if HAVE_OLD_TRAY
amule_LDADD += $(GTK_LIBS) 
amulegui_LDADD += $(GTK_LIBS) 
endif
endif

amuled_CFLAGS = $(WXBASE_CFLAGS) -DAMULE_DAEMON 
amuled_CXXFLAGS = $(WXBASE_CXXFLAGS) -DAMULE_DAEMON 
amuled_LDADD = $(WXBASE_LIBS) $(ZLIB_LIBS) $(CURL_LIBS) $(CRYPTOLIBS) $(RESOLV_LIB)

ed2k_SOURCES = ED2KLinkParser.cpp
ed2k_CFLAGS =

amulecmd_SOURCES = TextClient.cpp MD5Sum.cpp ECSocket.cpp ExternalConnector.cpp ECPacket.cpp ECSpecialTags.cpp OtherFunctions.cpp StringFunctions.cpp
amulecmd_CFLAGS = $(WXBASE_CFLAGS) -DEC_REMOTE
amulecmd_CXXFLAGS = $(WXBASE_CXXFLAGS) -DEC_REMOTE
amulecmd_LDADD = $(WXBASE_LIBS) $(READLINE_LIBS) $(ZLIB_LIBS) $(RESOLV_LIB)

amulecmdDLG_SOURCES = TextClient.cpp MD5Sum.cpp ECSocket.cpp ExternalConnector.cpp ECPacket.cpp ECSpecialTags.cpp OtherFunctions.cpp StringFunctions.cpp
amulecmdDLG_CFLAGS = $(WX_CFLAGS) -DAMULECMDDLG -DEC_REMOTE
amulecmdDLG_CXXFLAGS = $(WX_CXXFLAGS) -DAMULECMDDLG  -DEC_REMOTE
amulecmdDLG_LDADD = $(WX_LIBS) $(ZLIB_LIBS) $(RESOLV_LIB)

amuleweb_SOURCES =  WebServer.cpp WebInterface.cpp WebSocket.cpp \
	MD5Sum.cpp ECSocket.cpp ExternalConnector.cpp ECPacket.cpp OtherFunctions.cpp ECSpecialTags.cpp StringFunctions.cpp NetworkFunctions.cpp Format.cpp
amuleweb_CFLAGS = $(WXBASE_CFLAGS) $(LIBPNG_CFLAGS) $(AMULEWEB_DEFS) -DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\""
amuleweb_CXXFLAGS = $(WXBASE_CXXFLAGS) $(LIBPNG_CXXFLAGS) $(AMULEWEB_DEFS) -DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\""
amuleweb_LDADD = $(WXBASE_LIBS) $(ZLIB_LIBS) $(READLINE_LIBS) $(LIBPNG_LDFLAGS) $(LIBPNG_LIBS) $(RESOLV_LIB)

amulewebDLG_SOURCES = WebServer.cpp WebInterface.cpp WebSocket.cpp \
	 MD5Sum.cpp ECSocket.cpp ExternalConnector.cpp ECPacket.cpp OtherFunctions.cpp ECSpecialTags.cpp StringFunctions.cpp NetworkFunctions.cpp Format.cpp
amulewebDLG_CFLAGS = $(WX_CFLAGS) $(LIBPNG_CFLAGS) $(AMULEWEB_DEFS) -DAMULEWEBDLG -DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\""
amulewebDLG_CXXFLAGS = $(WX_CXXFLAGS) $(LIBPNG_CXXFLAGS) $(AMULEWEB_DEFS) -DAMULEWEBDLG  -DEC_REMOTE -DWEBSERVERDIR="\"$(pkgdatadir)/webserver\""
amulewebDLG_LDADD = $(WX_LIBS) $(ZLIB_LIBS) $(LIBPNG_LDFLAGS) $(LIBPNG_LIBS) $(RESOLV_LIB)

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
		 filefn.h \
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
		 IPFilter.h \
		 KadDlg.h \
		 KnownFile.h \
		 KnownFileList.h \
		 listctrl.h \
		 extern/listctrl.242.h \
		 extern/listctrl.254.h \
		 extern/listctrl.old.h \
		 ListenSocket.h \
		 MD5Sum.h \
		 MemFile.h \
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
		 Statistics.h \
		 StatisticsDlg.h \
		 StateMachine.h \
		 StringFunctions.h \
		 Format.h \
		 Logger.h \
		 SysTray.h \
		 TextClient.h \
		 TransferWnd.h \
		 tree.hh \
		 Types.h \
		 updownclient.h \
		 UploadBandwidthThrottler.h \
		 UploadQueue.h \
		 WebInterface.h \
		 WebServer.h \
		 WebSocket.h \
		 xembed.h


MAINTAINERCLEANFILES = Makefile.in
