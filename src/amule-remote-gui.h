//
// This file is part of the aMule Project.
//
// Copyright (c) 2005-2011 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//

#ifndef AMULE_REMOTE_GUI_H
#define AMULE_REMOTE_GUI_H


#include <ec/cpp/RemoteConnect.h>		// Needed for CRemoteConnect


#include "Statistics.h"
#include "Preferences.h"
#include "Statistics.h"
#include "RLE.h"
#include "SearchList.h"			// Needed for CSearchFile


class CED2KFileLink;
class CServer;
class CKnownFile;
class CSearchFile;
class CPartFile;
class CClientRef;
class CStatistics;
class CPath;

class wxEvtHandler;
class wxTimer;
class wxTimerEvent;
	
#include <wx/dialog.h>

class CEConnectDlg : public wxDialog {
	wxString host;
	int port;
	
	wxString pwd_hash;
	wxString login, passwd;
	bool m_save_user_pass;
	
	DECLARE_EVENT_TABLE()
public:
	CEConnectDlg();
	
	void OnOK(wxCommandEvent& event);
	
	wxString Host() { return host; }
	int Port() { return port; }

	wxString Login() { return login; }
	wxString PassHash();
	bool SaveUserPass() { return m_save_user_pass; }
};

DECLARE_LOCAL_EVENT_TYPE(wxEVT_EC_INIT_DONE, wxEVT_USER_FIRST + 1001)

class wxECInitDoneEvent : public wxEvent {
public:
	wxECInitDoneEvent() : wxEvent(-1, wxEVT_EC_INIT_DONE)
	{
	}

	wxEvent *Clone(void) const
	{
		return new wxECInitDoneEvent(*this);
	}
};

class CPreferencesRem : public CPreferences, public CECPacketHandlerBase {
	CRemoteConnect *m_conn;
	uint32 m_exchange_send_selected_prefs;
	uint32 m_exchange_recv_selected_prefs;

	virtual void HandlePacket(const CECPacket *packet);
public:
	CPreferencesRem(CRemoteConnect *);

	bool CreateCategory(Category_Struct *& category, const wxString& name, const CPath& path,
						const wxString& comment, uint32 color, uint8 prio);
	bool UpdateCategory(uint8 cat, const wxString& name, const CPath& path,
						const wxString& comment, uint32 color, uint8 prio);

	void RemoveCat(uint8 cat);
	
	bool LoadRemote();
	void SendToRemote();
};

//
// T - type if item in container
// I - type of id of item
// G - type of tag used to create/update items
//
template <class T, class I, class G = CECTag>
class CRemoteContainer : public CECPacketHandlerBase {
protected:
	enum {
		IDLE,            // no request in the air
		STATUS_REQ_SENT, // sent request for item status
		FULL_REQ_SENT    // sent request for full info
	} m_state;
	
	CRemoteConnect *m_conn;

	std::list<T *> m_items;
	std::map<I, T *> m_items_hash;
	
	// .size() is O(N) operation in stl
	int m_item_count;
	
	// use incremental tags algorithm
	bool m_inc_tags;
	
	// command that will be used in full request
	int m_full_req_cmd, m_full_req_tag;
	
	virtual void HandlePacket(const CECPacket *packet)
	{
		switch(this->m_state) {
			case IDLE: wxFAIL; // not expecting anything
			case STATUS_REQ_SENT:
				// if derived class choose not to proceed, return - but with good status
				this->m_state = IDLE;
				if ( this->Phase1Done(packet) ) {
					if (this->m_inc_tags) {
						// Incremental tags: new items always carry full info.
						ProcessUpdate(packet, NULL, m_full_req_tag);
					} else {
						// Non-incremental tags: we might get partial info on new items.
						// Collect all this items in a tag, and then request full info about them.
						CECPacket req_full(this->m_full_req_cmd);
				
						ProcessUpdate(packet, &req_full, m_full_req_tag);
					
						// Phase 3: request full info about files we don't have yet
						if ( req_full.HasChildTags() ) {
							m_conn->SendRequest(this, &req_full);
							this->m_state = FULL_REQ_SENT;
						}
					}
				}
				break;
			case FULL_REQ_SENT:
				ProcessFull(packet);
				m_state = IDLE;
				break;
		}
	}
public:
	CRemoteContainer(CRemoteConnect *conn, bool inc_tags)
	{
		m_state = IDLE;
		
		m_conn = conn;
		m_item_count = 0;
		m_inc_tags = inc_tags;
	}
	
	virtual ~CRemoteContainer()
	{
	}
	
	typedef typename std::list<T *>::iterator iterator;
	iterator begin() { return m_items.begin(); }
	iterator end() { return m_items.end(); }

	uint32 GetCount()
	{
		return m_item_count;
	}
	
	void AddItem(T *item)
	{
		m_items.push_back(item);
		m_items_hash[GetItemID(item)] = item;
		m_item_count++;
	}

	T *GetByID(I id)
	{
		// avoid creating nodes
		return m_items_hash.count(id) ? m_items_hash[id] : NULL;
	}

	void Flush()
	{
		m_items.clear();
		m_items_hash.clear();
		m_item_count = 0;
	}
			
	//
	// Flush & reload
	//
	/*
	We usually don't keep outdated code as comments, but this blocking implementation
	shows the overall procedure well. It had to be scattered for the event driven implementation.

	bool FullReload(int cmd)
	{
		CECPacket req(cmd);
		CScopedPtr<const CECPacket> reply(this->m_conn->SendRecvPacket(&req));
		if ( !reply.get() ) {
			return false;
		}
		for(typename std::list<T *>::iterator j = this->m_items.begin(); j != this->m_items.end(); j++) {
			this->DeleteItem(*j);
		}
		
		Flush();
		
		ProcessFull(reply.get());
		
		return true;
	}
	*/
	void FullReload(int cmd)
	{
		if ( this->m_state != IDLE ) {
			return;
		}

		for(typename std::list<T *>::iterator j = this->m_items.begin(); j != this->m_items.end(); j++) {
			this->DeleteItem(*j);
		}
		
		Flush();

		CECPacket req(cmd);
		this->m_conn->SendRequest(this, &req);
		this->m_state = FULL_REQ_SENT;
		this->m_full_req_cmd = cmd;
	}
	
	//
	// Following are like basicly same code as in webserver. Eventually it must
	// be same class
	//
	void DoRequery(int cmd, int tag)
	{
		if ( this->m_state != IDLE ) {
			return;
		}
		CECPacket req_sts(cmd, m_inc_tags ? EC_DETAIL_INC_UPDATE : EC_DETAIL_UPDATE);
		this->m_conn->SendRequest(this, &req_sts);
		this->m_state = STATUS_REQ_SENT;
		this->m_full_req_cmd = cmd;
		this->m_full_req_tag = tag;
	}
	/*
	We usually don't keep outdated code as comments, but this blocking implementation
	shows the overall procedure well. It had to be scattered for the event driven implementation.

	bool DoRequery(int cmd, int tag)
	{
		CECPacket req_sts(cmd, m_inc_tags ? EC_DETAIL_INC_UPDATE : EC_DETAIL_UPDATE);
	
		//
		// Phase 1: request status
		CScopedPtr<const CECPacket> reply(this->m_conn->SendRecvPacket(&req_sts));
		if ( !reply.get() ) {
			return false;
		}
		
		if ( !this->Phase1Done(reply.get()) ) {
			// if derived class choose not to proceed, return - but with good status
			return true;
		}
		//
		// Phase 2: update status, mark new files for subsequent query
		CECPacket req_full(cmd);
	
		ProcessUpdate(reply.get(), &req_full, tag);
	
		reply.reset();
	
		if ( !m_inc_tags ) {
			// Phase 3: request full info about files we don't have yet
			if ( req_full.GetTagCount() ) {
				reply.reset(this->m_conn->SendRecvPacket(&req_full));
				if ( !reply.get() ) {
					return false;
				}
				ProcessFull(reply.get());
			}
		}
		return true;
	}
	*/

	void ProcessFull(const CECPacket *reply)
	{
		for (CECPacket::const_iterator it = reply->begin(); it != reply->end(); it++) {
			G *tag = (G *) & *it;
			// initialize item data from EC tag
			AddItem(CreateItem(tag));
		}
	}

	void RemoveItem(iterator & it)
	{
		I item_id = GetItemID(*it);
		// reduce count
		m_item_count--;
		// remove from map
		m_items_hash.erase(item_id);
		// item may contain data that need to be freed externally, before
		// dtor is called and memory freed
		DeleteItem(*it);

		m_items.erase(it);
	}

	virtual void ProcessUpdate(const CECTag *reply, CECPacket *full_req, int req_type)
	{
		std::set<I> core_files;
		for (CECPacket::const_iterator it = reply->begin(); it != reply->end(); it++) {
			G *tag = (G *) & *it;
			if ( tag->GetTagName() != req_type ) {
				continue;
			}
	
			core_files.insert(tag->ID());
			if ( m_items_hash.count(tag->ID()) ) {
				// Item already known: update it
				T *item = m_items_hash[tag->ID()];
				ProcessItemUpdate(tag, item);
			} else {
				// New item
				if (full_req) {
					// Non-incremental mode: we have only partial info
					// so we need to request full info before we can use the item
					full_req->AddTag(CECTag(req_type, tag->ID()));
				} else {
					// Incremental mode: new items always carry full info,
					// so we can add it right away
					AddItem(CreateItem(tag));
				}
			}
		}
		for(iterator it = begin(); it != end();) {
			iterator it2 = it++;
			if ( core_files.count(GetItemID(*it2)) == 0 ) {
				RemoveItem(it2);
			}
		}
	}

	virtual T *CreateItem(G *)
	{
		return 0;
	}
	virtual void DeleteItem(T *)
	{
	}
	virtual I GetItemID(T *)
	{
		return I();
	}
	virtual void ProcessItemUpdate(G *, T *)
	{
	}

	virtual bool Phase1Done(const CECPacket *)
	{
		return true;
	}
};

class CServerConnectRem : public CECPacketHandlerBase {
	CRemoteConnect *m_Conn;
	uint32 m_ID;
	
	CServer *m_CurrServer;
	
	virtual void HandlePacket(const CECPacket *packet);
	
public:
	CServerConnectRem(CRemoteConnect *);
	bool ReQuery();
	
	bool IsConnected() { return (m_ID != 0) && (m_ID != 0xffffffff); }
	bool IsConnecting() { return m_ID == 0xffffffff; }
	bool IsLowID() { return m_ID < 16777216; }
	uint32 GetClientID() { return m_ID; }
	CServer *GetCurrentServer() { return m_CurrServer; }
	
	//
	// Actions
	//
	void ConnectToServer(CServer* server);
	void ConnectToAnyServer();
	void StopConnectionTry();
	void Disconnect();
};

class CServerListRem : public CRemoteContainer<CServer, uint32, CEC_Server_Tag> {
	uint32 m_TotalUser, m_TotalFile;
	
	virtual void HandlePacket(const CECPacket *packet);
public:
	CServerListRem(CRemoteConnect *);
	void GetUserFileStatus(uint32 &total_user, uint32 &total_file)
	{
		total_user = m_TotalUser;
		total_file = m_TotalFile;
	}
	
	void UpdateUserFileStatus(CServer *server);
	
	CServer* GetServerByAddress(const wxString& address, uint16 port) const;
	CServer* GetServerByIPTCP(uint32 nIP, uint16 nPort) const;

	//
	// Actions
	//
	void RemoveServer(CServer* server);
	void UpdateServerMetFromURL(wxString url);
	void SetStaticServer(CServer* server, bool isStatic);
	void SetServerPrio(CServer* server, uint32 prio);
	void SaveServerMet() {}	// not needed here
	void FilterServers() {}	// not needed here
	
	//
	// template
	//
	CServer *CreateItem(CEC_Server_Tag *);
	void DeleteItem(CServer *);
	uint32 GetItemID(CServer *);
	void ProcessItemUpdate(CEC_Server_Tag *, CServer *);
};

class CUpDownClientListRem : public CRemoteContainer<CClientRef, uint32, CEC_UpDownClient_Tag> {
public:
	CUpDownClientListRem(CRemoteConnect *);

	void FilterQueues() {}	// not needed here
	//
	// template
	//
	CClientRef *CreateItem(CEC_UpDownClient_Tag *);
	void DeleteItem(CClientRef *);
	uint32 GetItemID(CClientRef *);
	void ProcessItemUpdate(CEC_UpDownClient_Tag *, CClientRef *);
};

class CDownQueueRem : public std::map<uint32, CPartFile*> {
	CRemoteConnect *m_conn;
public:
	CDownQueueRem(CRemoteConnect * conn) { m_conn = conn; }
	
	CPartFile* GetFileByID(uint32 id);
	
	//
	// User actions
	//
	void Prio(CPartFile *file, uint8 prio);
	void AutoPrio(CPartFile *file, bool flag);
	void Category(CPartFile *file, uint8 cat);
	
	void SendFileCommand(CPartFile *file, ec_tagname_t cmd);
	//
	// Actions
	//
	void StopUDPRequests() {}
	void AddFileLinkToDownload(CED2KFileLink*, uint8);
	bool AddLink(const wxString &link, uint8 category = 0);
	void UnsetCompletedFilesExist();
	void ResetCatParts(int cat);
	void AddSearchToDownload(CSearchFile* toadd, uint8 category);
	void ClearCompleted(const ListOfUInts32 & ecids);
};

class CSharedFilesRem  : public std::map<uint32, CKnownFile*> {
	CRemoteConnect *m_conn;
public:
	CSharedFilesRem(CRemoteConnect * conn);
	
	CKnownFile *GetFileByID(uint32 id);

	void SetFilePrio(CKnownFile *file, uint8 prio);

	//
	// Actions
	//
	void AddFilesFromDirectory(const CPath&);
	void Reload(bool sendtoserver = true, bool firstload = false);
	bool RenameFile(CKnownFile* file, const CPath& newName);
	void SetFileCommentRating(CKnownFile* file, const wxString& newComment, int8 newRating);
};

class CKnownFilesRem : public CRemoteContainer<CKnownFile, uint32, CEC_SharedFile_Tag> {
	CKnownFile * CreateKnownFile(CEC_SharedFile_Tag *tag, CKnownFile *file = NULL);
	CPartFile  * CreatePartFile(CEC_PartFile_Tag *tag);

public:
	CKnownFilesRem(CRemoteConnect * conn);
	
	CKnownFile *FindKnownFileByID(uint32 id) { return GetByID(id); }

	uint16 requested;
	uint32 transferred;
	uint16 accepted;

	//
	// template
	//
	CKnownFile *CreateItem(CEC_SharedFile_Tag *) { wxFAIL; return NULL; }	// unused, required by template
	void DeleteItem(CKnownFile *);
	uint32 GetItemID(CKnownFile *);
	void ProcessItemUpdate(CEC_SharedFile_Tag *, CKnownFile *);
	bool Phase1Done(const CECPacket *) { return true; }
	void ProcessUpdate(const CECTag *reply, CECPacket *full_req, int req_type);

	void ProcessItemUpdatePartfile(CEC_PartFile_Tag *, CPartFile *);
};

class CIPFilterRem {
	CRemoteConnect *m_conn;
public:
	CIPFilterRem(CRemoteConnect *conn);
	
	//
	// Actions
	//
	void Reload();
	void Update(wxString strURL = wxEmptyString);
	bool IsReady() const { return true; }
};

class CSearchListRem : public CRemoteContainer<CSearchFile, uint32, CEC_SearchFile_Tag> {
	virtual void HandlePacket(const CECPacket *);
public:
	CSearchListRem(CRemoteConnect *);
	
	int m_curr_search;
	typedef std::map<long, CSearchResultList> ResultMap;
	ResultMap m_results;

	const CSearchResultList& GetSearchResults(long nSearchID);
	void RemoveResults(long nSearchID);
	const CSearchResultList& GetSearchResults(long nSearchID) const;
	//
	// Actions
	//

	wxString StartNewSearch(uint32* nSearchID, SearchType search_type,
		const CSearchList::CSearchParams& params);
		
	void StopSearch(bool globalOnly = false);
	
	//
	// template
	//
	CSearchFile *CreateItem(CEC_SearchFile_Tag *);
	void DeleteItem(CSearchFile *);
	uint32 GetItemID(CSearchFile *);
	void ProcessItemUpdate(CEC_SearchFile_Tag *, CSearchFile *);
	bool Phase1Done(const CECPacket *);
};

class CFriendListRem : public CRemoteContainer<CFriend, uint32, CEC_Friend_Tag> {
	virtual void HandlePacket(const CECPacket *);
public:
	CFriendListRem(CRemoteConnect *);

	void		AddFriend(const CClientRef& toadd);
	void		AddFriend(const CMD4Hash& userhash, uint32 lastUsedIP, uint32 lastUsedPort, const wxString& name);
	void		RemoveFriend(CFriend* toremove);
	void		RequestSharedFileList(CFriend* Friend);
	void		RequestSharedFileList(CClientRef& client);
	void		SetFriendSlot(CFriend* Friend, bool new_state);

	//
	// template
	//
	CFriend *CreateItem(CEC_Friend_Tag *);
	void DeleteItem(CFriend *);
	uint32 GetItemID(CFriend *);
	void ProcessItemUpdate(CEC_Friend_Tag *, CFriend *);
};

class CStatsUpdaterRem : public CECPacketHandlerBase {
	virtual void HandlePacket(const CECPacket *);
public:
	CStatsUpdaterRem() {}
};

class CStatTreeRem : public CECPacketHandlerBase {
	virtual void HandlePacket(const CECPacket *);
	CRemoteConnect *m_conn;
public:
	CStatTreeRem(CRemoteConnect * conn) { m_conn = conn; }
	void DoRequery();
};

class CListenSocketRem {
	uint32 m_peak_connections;
public:
	uint32 GetPeakConnections() { return m_peak_connections; }
};

class CamuleRemoteGuiApp : public wxApp, public CamuleGuiBase, public CamuleAppCommon {
	wxTimer*	poll_timer;

	virtual int InitGui(bool geometry_enable, wxString &geometry_string);

	bool OnInit();

	int OnExit();

	void OnPollTimer(wxTimerEvent& evt);
	
	void OnECConnection(wxEvent& event);
	void OnECInitDone(wxEvent& event);
	void OnNotifyEvent(CMuleGUIEvent& evt);
	void OnFinishedHTTPDownload(CMuleInternalEvent& event);

	CStatsUpdaterRem m_stats_updater;
public:

	void Startup();

	bool ShowConnectionDialog();

	class CRemoteConnect *m_connect;

	CEConnectDlg *dialog;

	bool CopyTextToClipboard(wxString strText);

	virtual int ShowAlert(wxString msg, wxString title, int flags);

	void ShutDown(wxCloseEvent &evt);

	CPreferencesRem *glob_prefs;

	//
	// Provide access to core data thru EC
	CServerConnectRem *serverconnect;
	CServerListRem *serverlist;
	CDownQueueRem *downloadqueue;
	CSharedFilesRem *sharedfiles;
	CKnownFilesRem *knownfiles;
	CUpDownClientListRem *clientlist;
	CIPFilterRem *ipfilter;
	CSearchListRem *searchlist;
	CFriendListRem *friendlist;
	CListenSocketRem *listensocket;
	CStatTreeRem * stattree;

	CStatistics *m_statistics;

	bool AddServer(CServer *srv, bool fromUser = false);

	uint32 GetPublicIP();

	wxString GetLog(bool reset = false);
	wxString GetServerLog(bool reset = false);

	void AddServerMessageLine(wxString &msg);
	void AddRemoteLogLine(const wxString& line);

	void SetOSFiles(wxString ) { /* onlinesig is created on remote side */ }

	bool IsConnected() const { return IsConnectedED2K() || IsConnectedKad(); }
	bool IsFirewalled() const;
	bool IsConnectedED2K() const;
	bool IsConnectedKad() const 
	{ 
		return ((m_ConnState & CONNECTED_KAD_OK) 
				|| (m_ConnState & CONNECTED_KAD_FIREWALLED));
	}
	bool IsFirewalledKad() const { return (m_ConnState & CONNECTED_KAD_FIREWALLED) != 0; }
	
	bool IsKadRunning() const { return ((m_ConnState & CONNECTED_KAD_OK) 
				|| (m_ConnState & CONNECTED_KAD_FIREWALLED)
				|| (m_ConnState & CONNECTED_KAD_NOT)); }

	// Check Kad state (UDP)
	bool IsFirewalledKadUDP() const		{ return theStats::IsFirewalledKadUDP(); }
	bool IsKadRunningInLanMode() const	{ return theStats::IsKadRunningInLanMode(); }
	// Kad stats
	uint32 GetKadUsers() const			{ return theStats::GetKadUsers(); }
	uint32 GetKadFiles() const			{ return theStats::GetKadFiles(); }
	uint32 GetKadIndexedSources() const	{ return theStats::GetKadIndexedSources(); }
	uint32 GetKadIndexedKeywords() const{ return theStats::GetKadIndexedKeywords(); }
	uint32 GetKadIndexedNotes() const	{ return theStats::GetKadIndexedNotes(); }
	uint32 GetKadIndexedLoad() const	{ return theStats::GetKadIndexedLoad(); }
	// True IP of machine
	uint32 GetKadIPAdress() const		{ return theStats::GetKadIPAdress(); }
	// Buddy status
	uint8	GetBuddyStatus() const		{ return theStats::GetBuddyStatus(); }
	uint32	GetBuddyIP() const			{ return theStats::GetBuddyIP(); }
	uint32	GetBuddyPort() const		{ return theStats::GetBuddyPort(); }

	void StartKad();
	void StopKad();
	
	/** Bootstraps kad from the specified IP (must be in hostorder). */
	void BootstrapKad(uint32 ip, uint16 port);
	/** Updates the nodes.dat file from the specified url. */
	void UpdateNotesDat(const wxString& str);

	void DisconnectED2K();

	bool CryptoAvailable() const;
	
	uint32 GetED2KID() const;
	uint32 GetID() const;
	void ShowUserCount();
	
	uint8 m_ConnState;
	uint32 m_clientID;

	wxLocale	m_locale;
	// This KnownFile collects all currently uploading clients for display in the upload list control
	CKnownFile * m_allUploadingKnownFile;

	DECLARE_EVENT_TABLE()
};

DECLARE_APP(CamuleRemoteGuiApp)

extern CamuleRemoteGuiApp *theApp;

#endif /* AMULE_REMOTE_GUI_H */

// File_checked_for_headers
