//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2006 Angel Vidal Veiga ( kry@users.sourceforge.net )
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

#ifndef REMOTECONNECT_H
#define REMOTECONNECT_H


#include "ECMuleSocket.h"
#include "ECPacket.h"		// Needed for CECPacket

class CECPacketHandlerBase {
	public:
		virtual ~CECPacketHandlerBase() { }
		virtual void HandlePacket(const CECPacket *) = 0;
};

class CECLoginPacket : public CECPacket {
	public:
		CECLoginPacket(const wxString &pass,
						const wxString& client, const wxString& version);
};

#warning Kry TODO - move to abstract layer.
class CRemoteConnect : public CECMuleSocket {
private:
	// State enums for connection SM ( client side ) in case of async processing
	enum { 
		EC_INIT,         // initial state
		EC_CONNECT_SENT, // socket connect request sent
		EC_REQ_SENT,     // sent auth request to core, waiting for reply
		EC_OK,           // core replyed "ok"
		EC_FAIL          // core replyed "bad"
	} m_ec_state;
	
	// fifo of handlers for on-the-air requests. all EC concept is working in fcfs
	// order, so it is ok to assume that order of replies is same as order of requests
	std::list<CECPacketHandlerBase *> m_req_fifo;
	int m_req_count;
	int m_req_fifo_thr;

	wxEvtHandler* m_notifier;

	wxString m_connectionPassword;
	wxString m_server_reply;
	wxString m_client;
	wxString m_version;
	
public:
	// The event handler is used for notifying connect/close 
	CRemoteConnect(wxEvtHandler* evt_handler);

	bool ConnectToCore(
		const wxString &host, int port,
		const wxString& login, const wxString &pass,
		const wxString& client, const wxString& version);

	const wxString& GetServerReply() const { return m_server_reply; }

	bool RequestFifoFull()
	{
		return m_req_count > m_req_fifo_thr;
	}
	
	virtual void OnConnect(); // To override connection events
	virtual void OnClose(); // To override close events

	void SendRequest(CECPacketHandlerBase *handler, CECPacket *request);
	void SendPacket(CECPacket *request);
	
	/********************* EC API ********************/
	

	/* Misc */

	// Shuts down aMule
	void ShutDown();

	// Handles a ED2K link
	void Ed2kLink(wxString* link);


	/* Kad */
	
	// Connects Kad network
	void StartKad();
	
	// Disconnects Kad network
	void StopKad();
	
	
	/* ED2K */
	
	// Connects to ED2K. If ip and port are not 0, connect 
	// to the specific port. Otherwise, connect to any.
	void ConnectED2K(uint32 ip, uint16 port);
	
	// Disconnects from ED2K
	void DisconnectED2K();


	/* Servers */
	
	// Adds a server
	void AddServer(uint32 ip,
		       uint16 port);

	// Remove specific server
	// Returns: Error message or empty string for no error
	void RemoveServer(uint32 ip,
			  uint16 port);

	// Returns ED2K server list
	void GetServerList();

	// Updates ED2K server from a URL
	void UpdateServerList(wxString url);


	/* Search */
	
	// Starts new search
	void StartSearch();

	// Stops current search
	void StopSearch();

	// Returns search progress in %%
	void GetSearchProgress();

	// Add 1 or more of found files to download queue
	void DownloadSearchResult(uint32* file);


	/* Statistics */

	// Returns aMule statistics
	void GetStatistics();

	// Returns aMule connection status
	void GetConnectionState();


	/* Queue/File handling */

	// Returns downloads queue
	void GetDlQueue(CMD4Hash* file);

	// Returns uploads queue
	void GetUpQueue(CMD4Hash* file);

	// Returns waiting queue
	void GetWtQueue(CMD4Hash* file);

	// Drops no needed sources
	void DropNoNeededSources(CMD4Hash* file);

	// Drops full queue sources
	void DropFullQueueSources(CMD4Hash* file);

	// Drops high queue rating sources
	void DropHighQueueSources(CMD4Hash* file);

	// Cleans up sources
	void CleanUpSources(CMD4Hash* file);

	// Swaps A4AF to a file
	void SwapA4AFThis(CMD4Hash* file);

	// Swaps A4AF to a file (auto)
	void SwapA4AFThisAuto(CMD4Hash* file);

	// Swaps A4AF to any other files
	void SwapA4AFOthers(CMD4Hash* file);

	// Pauses download(s)
	void Pause(CMD4Hash* file);

	// Resumes download(s)
	void Resume(CMD4Hash* file);

	// Stops download(s)
	void Stop(CMD4Hash* file);

	// Sets priority for a download
	void SetPriority(CMD4Hash* file,
			 uint8 priority);

	// Deletes a download
	void Delete(CMD4Hash* file);

	// Sets category for a download
	void SetCategory(CMD4Hash* file,
			 wxString category);


	/* Shared files */

	// Returns a list of shared files
	void GetSharedFiles();

	// Sets priority for 1 or more shared files
	void SetSharedPriority(CMD4Hash* file,
			       uint8 priority);

	// Reloads shared file list
	void ReloadSharedFiles();

	// Adds a directory to shared file list
	void AddDirectoryToSharedFiles(wxString dir);

	// Renames a file
	void RenameFile(CMD4Hash file,
			wxString name);


	/* Logging */

	// Adds a new debug log line
	void AddLogline();

	// Adds a new debug log line
	void AddDebugLogLine();

	// Retrieves the log
	void GetLog();

	// Returns the last log line.
	void GetLastLogLine();

	// Retrieves the debug log
	void GetDebugLog();

	// Retrieves the server info log
	void GetServerInfo();

	// Clears the log
	void ClearLog();

	// Clears the debug log
	void ClearDebugLog();

	// Clears server info log
	void ClearServerInfo();


	/* Preferences */

	// Request for Preferences
	void GetPreferences();

	// Setting the preferences
	void SetPreferencesCategories();
	void SetPreferencesGeneral(wxString userNick,
				   CMD4Hash userHash);
	void SetPreferencesConnections(uint32 LineDownloadCapacity,
				       uint32 LineUploadCapacity,
				       uint16 MaxDownloadSpeed,
				       uint16 MaxUploadSpeed,
				       uint16 UploadSlotAllocation,
				       uint16 TCPPort,
				       uint16 UDPPort,
				       bool DisableUDP,
				       uint16 MaxSourcesPerFile,
				       uint16 MaxConnections,
				       bool EnableAutoConnect,
				       bool EnableReconnect,
				       bool EnableNetworkED2K,
				       bool EnableNetworkKademlia);
	void SetPreferencesMessageFilter(bool Enabled,
					 bool FilterAll,
					 bool AllowFromFriends,
					 bool FilterFromUnknownClients,
					 bool FilterByKeyword,
					 wxString Keywords);
	void SetPreferencesRemoteCrtl(bool RunOnStartup,
				      uint16 Port,
				      bool Guest,
				      CMD4Hash GuestPasswdHash,
				      bool UseGzip,
				      uint32 RefreshInterval,
				      wxString Template);
	void SetPreferencesOnlineSig(bool Enabled);
	void SetPreferencesServers(bool RemoveDeadServers,
				   uint16 RetriesDeadServers,
				   bool AutoUpdate,
				   // bool URLList, TODO: Implement this!
				   bool AddFromServer,
				   bool AddFromClient,
				   bool UsePrioritySystem,
				   bool SmartLowIDCheck,
				   bool SafeServerConnection,
				   bool AutoConnectStaticOnly,
				   bool ManualHighPriority);
	void SetPreferencesFiles(bool ICHEnabled,
				 bool AIHCTrust,
				 bool NewPaused,
				 bool NewDownloadAutoPriority,
				 bool PreviewPriority,
				 bool NewAutoULPriotiry,
				 bool UploadFullChunks,
				 bool StartNextPaused,
				 bool ResumeSameCategory,
				 bool SaveSources,
				 bool ExtractMetadata,
				 bool AllocateFullChunks,
				 bool AllocateFullSize,
				 bool CheckFreeSpace,
				 uint32 MinFreeSpace);
	void SetPreferencesSrcDrop(uint8 NoNeeded,
				   bool DropFQS,
				   bool DropHQRS,
				   uint16 HQRSValue,
				   uint16 AutodropTimer);
	void SetPreferencesDirectories();
	void SetPreferencesStatistics();
	void SetPreferencesSecurity(uint8 CanSeeShares,
				    uint32 FilePermissions,
				    uint32 DirPermissions,
				    bool IPFilterEnabled,
				    bool IPFilterAutoUpdate,
				    wxString IPFilterUpdateURL,
				    uint8 IPFilterLevel,
				    bool IPFilterFilterLAN,
				    bool UseSecIdent);
	void SetPreferencesCoreTweaks(uint16 MaxConnectionsPerFive,
				      bool Verbose,
				      uint32 FileBuffer,
				      uint32 ULQueue,
				      uint32 SRVKeepAliveTimeout);

	// Creates new category
	void CreateCategory(uint32 category,
			    wxString title,
			    wxString folder,
			    wxString comment,
			    uint32 color,
			    uint8 priority);

	// Updates existing category
	void UpdateCategory(uint32 category,
			    wxString title,
			    wxString folder,
			    wxString comment,
			    uint32 color,
			    uint8 priority);

	// Deletes existing category
	void DeleteCategory(uint32 category);

	// Retrieves the statistics graphs
	void GetStatsGraphs();

	// Retrieves the statistics tree
	void GetStatsTree();

private:
	virtual const CECPacket *OnPacketReceived(const CECPacket *packet);
	bool ConnectionEstablished(const CECPacket *reply);
};

DECLARE_LOCAL_EVENT_TYPE(wxEVT_EC_CONNECTION, wxEVT_USER_FIRST + 1000)

class wxECSocketEvent : public wxEvent {
public:
	wxECSocketEvent(int id, int event_id)	: wxEvent(event_id, id) {}
	wxECSocketEvent(int id)			: wxEvent(-1, id) {}
	wxECSocketEvent(int id, bool result, const wxString& reply) : wxEvent(-1, id)
	{
		m_value = result;
		m_server_reply = reply;
	}
	wxEvent *Clone(void) const		{ return new wxECSocketEvent(*this); }
	long GetResult() const			{ return m_value; }
	const wxString& GetServerReply() const	{ return m_server_reply; }
private:
	bool m_value;
	wxString m_server_reply;
};

#endif // REMOTECONNECT_H

// File_checked_for_headers
