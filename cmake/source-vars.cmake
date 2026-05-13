if (BUILD_MONOLITHIC OR BUILD_DAEMON)
	set (CORE_SOURCES
		kademlia/kademlia/Kademlia.cpp
		kademlia/kademlia/Prefs.cpp
		kademlia/kademlia/Search.cpp
		kademlia/kademlia/UDPFirewallTester.cpp
		kademlia/net/KademliaUDPListener.cpp
		kademlia/net/PacketTracking.cpp
		kademlia/routing/Contact.cpp
		kademlia/routing/RoutingZone.cpp
		amule.cpp
		BaseClient.cpp
		ClientCreditsList.cpp
		ClientList.cpp
		ClientTCPSocket.cpp
		ClientUDPSocket.cpp
		CorruptionBlackBox.cpp
		DownloadBandwidthThrottler.cpp
		DownloadClient.cpp
		DownloadQueue.cpp
		ECSpecialCoreTags.cpp
		EMSocket.cpp
		EncryptedStreamSocket.cpp
		EncryptedDatagramSocket.cpp
		ExternalConn.cpp
		FriendList.cpp
		IPFilter.cpp
		KnownFileList.cpp
		ListenSocket.cpp
		MuleUDPSocket.cpp
		SearchFile.cpp
		SearchList.cpp
		ServerConnect.cpp
		ServerList.cpp
		ServerSocket.cpp
		ServerUDPSocket.cpp
		SHAHashSet.cpp
		SharedDirWatcher.cpp
		SharedFileList.cpp
		UploadBandwidthThrottler.cpp
		UploadClient.cpp
		UploadDiskIOThread.cpp
		UploadQueue.cpp
		PartFileWriteThread.cpp
		PartFileHashThread.cpp
		ThreadTasks.cpp
	)
endif()

if (BUILD_MONOLITHIC OR BUILD_REMOTEGUI)
	set (GUI_SOURCES
		# wxArtProvider subclass + the C TU it pulls icon bytes
		# from. ${AMULE_ICON_DATA_C} resolves to either the build-
		# generated copy (Python3 found at configure → regenerated
		# from src/icons/*.png by src/icons/embed_icons.py) or the
		# checked-in fallback (Python3 missing → use the file as
		# committed). See src/CMakeLists.txt for the resolution.
		CamuleArtProvider.cpp
		${AMULE_ICON_DATA_C}
		AddFriend.cpp
		amule-gui.cpp
		AppImageIntegration.cpp
		amuleDlg.cpp
		CatDialog.cpp
		ChatSelector.cpp
		ChatWnd.cpp
		ClientDetailDialog.cpp
		CommentDialog.cpp
		CommentDialogLst.cpp
		DirectoryTreeCtrl.cpp
		DownloadListCtrl.cpp
		FileDetailDialog.cpp
		FriendListCtrl.cpp
		GenericClientListCtrl.cpp
		KadDlg.cpp
		MuleTrayIcon.cpp
		OScopeCtrl.cpp
		PrefsUnifiedDlg.cpp
		SearchDlg.cpp
		SearchListCtrl.cpp
		ServerListCtrl.cpp
		ServerWnd.cpp
		SharedFilePeersListCtrl.cpp
		SharedFilesCtrl.cpp
		SharedFilesWnd.cpp
		SourceListCtrl.cpp
		StatisticsDlg.cpp
		TransferWnd.cpp
	)

	if (APPLE)
		# Obj-C++ helper for AppKit access (NSApp activation policy
		# toggle for "minimize to tray" — drops the Dock icon while
		# the main window is hidden so no Dock thumbnail is left).
		list (APPEND GUI_SOURCES MacAppHelper.mm)
	endif()
endif()

if (BUILD_MONOLITHIC OR BUILD_DAEMON OR BUILD_REMOTEGUI)
	set (COMMON_SOURCES
		amuleAppCommon.cpp
		ClientRef.cpp
		ECSpecialMuleTags.cpp
		GetTickCount.cpp
		GuiEvents.cpp
		HTTPDownload.cpp
		KnownFile.cpp
		Logger.cpp
		PartFile.cpp
		Preferences.cpp
		Proxy.cpp
		Server.cpp
		Statistics.cpp
		StatTree.cpp
		UserEvents.cpp
	)
endif()

if (ENABLE_IP2COUNTRY)
	set (IP2COUNTRY
		IP2Country.cpp
		geoip/MaxMindDBDatabase.cpp
	)
endif()

if (ENABLE_UPNP)
	set (UPNP_SOURCES ${CMAKE_SOURCE_DIR}/src/UPnPBase.cpp)
endif()
