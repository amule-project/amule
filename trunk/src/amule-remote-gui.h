#ifndef AMULE_REMOTE_GUI_H
#define AMULE_REMOTE_GUI_H

#include "CTypedPtrList.h"
#include <map>
#include <vector>

class CED2KFileLink;
class CServer;
class CKnownFile;
class CSearchFile;
class CUpDownClient;

class CServerConnectRem {
	public:
		bool IsConnected();
		bool IsConnecting();
		bool IsLowID();
		uint32 GetClientID();
		
		//
		// Actions
		//
		void ConnectToServer(CServer* server);
		void ConnectToAnyServer();
		void StopConnectionTry();
		void Disconnect();
};

class CServerListRem {
	public:
		void GetUserFileStatus(uint32 &total_user, uint32 &total_file);
		CServer *GetServerByAddress(const wxString& address, uint16 port);

		//
		// Actions
		//
		void RemoveServer(CServer* server);
		void UpdateServerMetFromURL(wxString url);
		void SaveServermetToFile();
};

class CUpQueueRem {
	public:
		uint32 GetKBps();
		uint32 GetUpDatarateOverhead();
		POSITION GetFirstFromUploadList();
		CUpDownClient *GetNextFromUploadList(POSITION &curpos);
		
		POSITION GetFirstFromWaitingList();
		CUpDownClient *GetNextFromWaitingList(POSITION &curpos);
		//
		// Actions
		//
		void AddUpDataOverheadOther(uint32);
};

class CDownQueueRem {
	public:
		uint32 GetKBps();
		uint32 GetDownDatarateOverhead();
		bool CompletedFilesExist();

		//
		// Actions
		//
		void StopUDPRequests();
		void AddFileLinkToDownload(CED2KFileLink*, uint8);
		bool AddED2KLink(const wxString &link, int category = 0);
		void UnsetCompletedFilesExist();
		CKnownFile *GetFileByID(CMD4Hash);
};

class CSharedFilesRem {
	public:
		CKnownFile *GetFileByID(const CMD4Hash& filehash);
		//
		// Actions
		//
		void AddFilesFromDirectory(wxString );
		void Reload(bool sendtoserver = true, bool firstload = false);
};

class CKnownFilesRem {
	public:
		CKnownFile *FindKnownFileByID(const CMD4Hash& filehash);

        uint16 requested;
        uint32 transfered;
        uint16 accepted;
};

class CClientCreditsRem {
	public:
		bool CryptoAvailable();
};

class CClientListRem {
	public:
		const std::multimap<uint32, CUpDownClient*>& GetClientList();
		
		//
		// Actions
		//
		void AddClient(CUpDownClient *);
		void FilterQueues();
};

class CIPFilterRem {
	public:
		//
		// Actions
		//
		void Reload();
		void Update(wxString strURL = wxEmptyString);
};

class CSearchListRem {
	public:
	
		std::map<long, std::vector<CSearchFile *> > m_Results;
		//
		// Actions
		//
		void Clear();
		void NewSearch(wxString type, uint32 search_id);
		void StopGlobalSearch();
};
	
#endif /* AMULE_REMOTE_GUI_H */
