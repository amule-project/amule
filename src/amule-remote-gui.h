#ifndef AMULE_REMOTE_GUI_H
#define AMULE_REMOTE_GUI_H

#include "CTypedPtrList.h"
#include "ECSpecialTags.h"
#include "Statistics.h"

#include <map>
#include <list>
#include <vector>

class CED2KFileLink;
class CServer;
class CKnownFile;
class CSearchFile;
class CPartFile;
class CUpDownClient;

class CRemoteConnect {
		ECSocket *m_ECSocket;
	public:
		CRemoteConnect();
		~CRemoteConnect();

		CECPacket *SendRecv(CECPacket *);
		void Send(CECPacket *);
};

//
// Concept is similar to containers in amuleweb. But without sort and html
// related code.
//
// T - type if item in container
// I - type of id of item
// G - type of tag used to create/update items
//
template <class T, class I, class G = CECTag>
class CRemoteContainer {
	protected:
		CRemoteConnect *m_conn;

		std::list<T *> m_items;
		std::map<T *, I> m_items_hash;
		
		// .size() is O(N) operation in stl
		int m_item_count;
	public:
		CRemoteContainer(CRemoteConnect *conn)
		{
			m_conn = conn;
			m_item_count = 0;
		}
		
		uint32 GetCount()
		{
			return m_item_count;
		}
		
		void AddItem(T *item)
		{
			m_items.push_back(item);
			m_items_hash[this->GetItemID(item)] = item;
		}
	
		T *GetByID(I id)
		{
			// avoid creating nodes
			return m_items_hash.count(id) ? m_items_hash[id] : NULL;
		}
		
		//
		// Following are like basicly same code as in webserver. Eventually it must
		// be same class
		//
		bool DoRequery(int cmd, int tag)
		{
			CECPacket req_sts(cmd, EC_DETAIL_UPDATE);
		
			//
			// Phase 1: request status
			CECPacket *reply = this->m_conn->SendRecv(&req_sts);
			if ( !reply ) {
				return false;
			}
			
			//
			// Phase 2: update status, mark new files for subsequent query
			CECPacket req_full(cmd);
		
			ProcessUpdate(reply, &req_full, tag);
		
			delete reply;
		
			// Phase 3: request full info about files we don't have yet
			if ( req_full.GetTagCount() ) {
				reply = this->m_conn->SendRecv(&req_full);
				if ( !reply ) {
					return false;
				}
				ProcessFull(reply);	
			}
			return true;
		}

		void ProcessFull(CECPacket *reply)
		{
			for (int i = 0;i < reply->GetTagCount();i++) {
				G *tag = (G *)reply->GetTagByIndex(i);
				// initialize item data from EC tag
				T *item = this->CreateItem(tag);
				AddItem(item);
			}
		}

		void ProcessUpdate(CECPacket *reply, CECPacket *full_req, int req_type)
		{
			std::set<I> core_files;
			for (int i = 0;i < reply->GetTagCount();i++) {
				G *tag = (G *)reply->GetTagByIndex(i);
		
				core_files.insert(tag->ID());
				if ( m_items_hash.count(tag->ID()) ) {
					T *item = m_items_hash[tag->ID()];
					item->ProcessUpdate(tag);
				} else {
					full_req->AddTag(CECTag(req_type, tag->ID()));
				}
			}
			std::list<I> del_ids;
			for(typename std::list<T>::iterator j = this->m_items.begin(); j != this->m_items.end(); j++) {
				if ( core_files.count(j->ID()) == 0 ) {
					// item may contain data that need to be freed externally, before
					// dtor is called and memory freed
					
					this->DeleteItem(*j);
					
					del_ids.push_back(j->ID());
				}
			}
			for(typename std::list<I>::iterator j = del_ids.begin(); j != del_ids.end(); j++) {
				m_items_hash.erase(*j);
				for(typename std::list<T>::iterator k = this->m_items.begin(); k != this->m_items.end(); k++) {
					if ( *j == k->ID() ) {
						this->m_items.erase(k);
						break;
					}
				}
			}
		}
};

class CServerConnectRem {
		CRemoteConnect *m_Conn;
		uint32 m_ID;
	public:
		CServerConnectRem(CRemoteConnect *);
		bool IsConnected() { return (m_ID != 0) && (m_ID != 0xffffffff); }
		bool IsConnecting() { return m_ID == 0xffffffff; }
		bool IsLowID() { return m_ID > 16777216; }
		uint32 GetClientID() { return m_ID; }
		CServer *GetCurrentServer();
		
		//
		// Actions
		//
		void ConnectToServer(CServer* server);
		void ConnectToAnyServer();
		void StopConnectionTry();
		void Disconnect();
};

class CServerListRem : public CRemoteContainer<CServer, uint32, CEC_Server_Tag> {
	public:
		CServerListRem(CRemoteConnect *);
		void GetUserFileStatus(uint32 &total_user, uint32 &total_file);
		CServer *GetServerByAddress(const wxString& address, uint16 port);

		//
		// Actions
		//
		void RemoveServer(CServer* server);
		void UpdateServerMetFromURL(wxString url);
		void SaveServermetToFile();
		
		//
		// template
		//
		CServer *CreateItem(CEC_Server_Tag *);
		void DeleteItem(CServer *);
		uint32 GetItemID(CServer *);
};

class CUpQueueRem : public CRemoteContainer<CUpDownClient, uint32, CEC_UpDownClient_Tag> {
	public:
		CUpQueueRem(CRemoteConnect *);
		
		uint32 GetWaitingUserCount();
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
		
		//
		// template
		//
		CUpDownClient *CreateItem(CEC_UpDownClient_Tag *);
		void DeleteItem(CUpDownClient *);
		uint32 GetItemID(CUpDownClient *);
};

class CDownQueueRem : public CRemoteContainer<CUpDownClient, uint32, CEC_UpDownClient_Tag> {
	public:
		CDownQueueRem(CRemoteConnect *);
		
		uint32 GetKBps();
		uint32 GetDownDatarateOverhead();
		bool CompletedFilesExist();

		uint32 GetFileCount();
		CKnownFile *GetFileByID(CMD4Hash);
		CPartFile *GetFileByIndex(unsigned int idx);
		bool IsPartFile(const CKnownFile* totest) const;
		//
		// Actions
		//
		void StopUDPRequests();
		void AddFileLinkToDownload(CED2KFileLink*, uint8);
		bool AddED2KLink(const wxString &link, int category = 0);
		void UnsetCompletedFilesExist();
		void ResetCatParts(int cat);
		//
		// template
		//
		CUpDownClient *CreateItem(CEC_UpDownClient_Tag *);
		void DeleteItem(CUpDownClient *);
		uint32 GetItemID(CUpDownClient *);
};

class CSharedFilesRem : public CRemoteContainer<CKnownFile, CMD4Hash, CEC_SharedFile_Tag> {
	public:
		CSharedFilesRem(CRemoteConnect *);;
		
		CKnownFile *GetFileByID(const CMD4Hash& filehash);
		uint32 GetCount();
		//
		// Actions
		//
		void AddFilesFromDirectory(wxString );
		void Reload(bool sendtoserver = true, bool firstload = false);
};

class CKnownFilesRem : public CRemoteContainer<CKnownFile, CMD4Hash> {
	public:
		CKnownFilesRem(CRemoteConnect *);
		
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
		CClientListRem(CRemoteConnect *);
		
		const std::multimap<uint32, CUpDownClient*>& GetClientList();
		uint32 GetBannedCount();
		
		//
		// Actions
		//
		void AddClient(CUpDownClient *);
		void FilterQueues();
};

class CIPFilterRem {
		CRemoteConnect *m_conn;
	public:
		//
		// Actions
		//
		void Reload();
		void Update(wxString strURL = wxEmptyString);
};

class CSearchListRem : public CRemoteContainer<CSearchFile, CMD4Hash, CEC_SearchFile_Tag> {
	public:
		CSearchListRem(CRemoteConnect *);
		
		std::map<long, std::vector<CSearchFile *> > m_Results;
		//
		// Actions
		//
		void Clear();
		void NewSearch(wxString type, uint32 search_id);
		void StopGlobalSearch();
};

class CListenSocketRem {
	public:
		uint32 GetPeakConnections();
};

class CStatisticsRem {
		uint32 m_Uptime;
		uint32 m_SendBytes, m_RecvBytes;
		uint32 m_UpOverhead, m_DownOverhead;
	public:
		uint32 GetUptimeMsecs() { return m_Uptime; }
		uint32 GetUptimeSecs() { return GetUptimeMsecs()/1000; }
		uint32 GetSessionReceivedBytes() { return m_RecvBytes; }
		uint32 GetSessionSentBytes() { return m_SendBytes; }
		uint32 GetUpDatarateOverhead() { return m_UpOverhead; }
		uint32 GetDownDatarateOverhead() { return m_DownOverhead; }
		
		unsigned GetHistory(unsigned cntPoints, double sStep, double sFinal, float **ppf, StatsGraphType which_graph);
		
		GraphUpdateInfo GetPointsForUpdate();
		StatsTree statstree;
};

#endif /* AMULE_REMOTE_GUI_H */
