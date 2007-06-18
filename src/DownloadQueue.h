//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2007 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#ifndef DOWNLOADQUEUE_H
#define DOWNLOADQUEUE_H

#include "MD4Hash.h"		// Needed for CMD4Hash
#include "ObservableQueue.h"	// Needed for CObservableQueue
#include "GetTickCount.h" 	// Needed fot GetTickCount


#include <deque>


class CSharedFileList;
class CSearchFile;
class CPartFile;
class CUpDownClient;
class CServer;
class CMemFile;
class CKnownFile;
class CED2KLink;
class CED2KFileLink;
class CED2KServerLink;
class CED2KServerListLink;

namespace Kademlia {
	class CUInt128;
}

/**
 * The download queue houses all active downloads.
 *
 * 
 * This class should be thread-safe.
 */
class CDownloadQueue : public CObservableQueue<CPartFile*>
{
public:
	/**
	 * Constructor.
	 */
	CDownloadQueue();

	/**
	 * Destructor.
	 */
	~CDownloadQueue();

	/**
	 * Loads met-files from the specified directory.
	 *
	 * @param path The directory containing the .met files.
	 */
	void	LoadMetFiles(const wxString& path);

	/**
	 * Main worker function.
	 */
	void	Process();


	/**
	 * Returns a pointer to the file with the specified hash, or NULL.
	 * 
	 * @param filehash The hash to search for.
	 * @return The corresponding file or NULL.
	 */
	CPartFile* GetFileByID(const CMD4Hash& filehash) const;
	
	/**
	 * Returns the file at the specified position in the file-list, or NULL if invalid.
	 *
	 * @param A valid position in the file-list.
	 * @return A valid pointer or NULL if the index was invalid.
	 */
	CPartFile* GetFileByIndex(unsigned int idx) const;
	
	
	/**
	 * Returns true if the file is currently being shared or downloaded 
	 */
	bool	IsFileExisting(const CMD4Hash& fileid) const;
	
	/**
	 * Returns true if the specified file is on the download-queue.
	 */
	bool	IsPartFile(const CKnownFile* file) const;
	
	/**
	 * Updates the file's download active time
	 */
	void OnConnectionState(bool bConnected);
	
	/**
	 * Starts a new download based on the specified search-result.
	 *
	 * @param toadd The search-result to add.
	 * @param category The category to assign to the new download.
	 *
	 * The download will only be started if no identical files are either
	 * being downloaded or shared currently.
	 */
	void	AddSearchToDownload(CSearchFile* toadd, uint8 category);
	
	
	/**
	 * Adds an existing partfile to the queue.
	 *
	 * @param newfile The file to add.
	 * @param paused If the file should be stopped when added.
	 * @param category The category to assign to the file.
	 */
	void	AddDownload(CPartFile* newfile, bool paused, uint8 category);


	/**
	 * Removes the specified file from the queue.
	 *
	 * @param toremove A pointer to the file object to be removed.
	 */
	void	RemoveFile(CPartFile* toremove);
	
	
	/**
	 * Saves the source-seeds of every file on the queue.
	 */
	void	SaveSourceSeeds();
	
	/**
	 * Loads the source-seeds of every file on the queue.
	 */
	void	LoadSourceSeeds();

	
	/**
	 * Adds a potiential new client to the specified file.
	 *
	 * @param sender The owner of the new source.
	 * @param source The client in question, might be deleted!
	 *
	 * This function will check the new client against the already existing
	 * clients. The source will then be queued as is appropriate, or deleted
	 * if it is duplicate of an existing client.
	 */
	void    CheckAndAddSource(CPartFile* sender, CUpDownClient* source);
	
	/**
	 * This function adds already known source to the specified file.
	 *
	 * @param sender The owner fo the new source.
	 * @param source The client in question.
	 *
	 * This function acts like CheckAndAddSource, with the exception that no
	 * checks are made to see if the client is a duplicate. It is assumed that
	 * it is in fact a valid client.
	 */
	void    CheckAndAddKnownSource(CPartFile* sender, CUpDownClient* source);
	
	
	/**
	 * Removes the specified client completly.
	 *
	 * @param toremove The client to be removed.
	 * @param updatewindow NOT USED!
	 * @param bDoStatsUdpate Specifies if the affected files should update their statistics.
	 * @return True if the sources was found and removed.
	 * 
	 * This function will remove the specified source from both normal source
	 * lists, A4AF lists and the downloadqueue-widget. The requestfile of the
	 * source is also reset.
	 */
	bool	RemoveSource(CUpDownClient* toremove, bool updatewindow = true, bool bDoStatsUpdate = true);


	/**
	 * Finds the queued client by IP and UDP-port, by looking at file-sources.
	 *
	 * @param dwIP The IP-address of the client.
	 * @param nUDPPort The UDP-port of the client.
	 * @return The matching client or NULL if none was found.
	 */
	CUpDownClient* GetDownloadClientByIP_UDP(uint32 dwIP, uint16 nUDPPort) const;
	

	/**
	 * Queues the specified file for source-requestion from the connected server.
	 */
	void	SendLocalSrcRequest(CPartFile* sender);

	/**
	 * Removes the specified server from the request-queue.
	 */
	void	RemoveLocalServerRequest(CPartFile* pFile);
	
	/**
	 * Resets all queued server-requests.
	 */
	void	ResetLocalServerRequests();


	/**
	 * Starts the next paused file on the queue, going after priority.
	 * Also checks for categories if enabled on preferences.
	 */
	void	StartNextFile(CPartFile* oldfile);


	/**
	 * Resets the category of all files with the specified category.
	 */
	void	ResetCatParts(uint8 cat);

	/**
	 * Sets the priority of all files with the specified category.
	 */
	void	SetCatPrio(uint8 cat, uint8 newprio);

	/**
	 * Sets the status of all files with the specified category.
	 */
	void	SetCatStatus(uint8 cat, int newstatus);

	/**
	 * Returns the current number of queued files.
	 */
	uint16	GetFileCount() const;

	/**
	 * Returns the current number of downloading files.
	 */
	uint16	GetDownloadingFileCount() const;

	/**
	 * Returns the current number of paused files.
	 */
	uint16	GetPausedFileCount() const;


	/**
	 * This function is called when a DNS lookup is finished.
	 */
	void	OnHostnameResolved(uint32 ip);


	/**
	 * Adds an ed2k or magnet link to download queue.
	 */
	bool	AddLink( const wxString& link, int category = 0 );

	bool	AddED2KLink( const wxString& link, int category = 0 );
	bool	AddED2KLink( const CED2KLink* link, int category = 0 );
	bool	AddED2KLink( const CED2KFileLink* link, int category = 0 );
	bool	AddED2KLink( const CED2KServerLink* link );
	bool	AddED2KLink( const CED2KServerListLink* link );


	/**
	 * Returns the current server which is beening queried by UDP packets.
	 */
	CServer* GetUDPServer() const;

	/**
	 * Set the server to query through UDP packest.
	 */
	void	SetUDPServer( CServer* server );
	

	/**
	 * Stop the source-requests from non-connected servers.
	 */
	void	StopUDPRequests();
	
	/* Kad Stuff */
	
	/**
	 * Add a Kad source to a download
	 */
	 void	KademliaSearchFile(uint32 searchID, const Kademlia::CUInt128* pcontactID, const Kademlia::CUInt128* pkadID, uint8 type, uint32 ip, uint16 tcp, uint16 udp, uint32 serverip, uint16 serverport, uint8 byCryptOptions);
	
	CPartFile* GetFileByKadFileSearchID(uint32 id) const;
	
	bool	DoKademliaFileRequest();
	
	void	SetLastKademliaFileRequest()	{lastkademliafilerequest = ::GetTickCount();}
	
private:
	/**
	 * This function initializes new observers with the current contents of the queue.
	 */
	virtual void ObserverAdded( ObserverType* o );
	

	/**
	 * Helper-function, sorts the filelist so that high-priority files are first.
	 */
	void	DoSortByPriority();
	
	/**
	 * Checks that there is enough free spaces for temp-files at that specified path.
	 *
	 * @param path The path to a folder containing temp-files.
	 */
	void	CheckDiskspace( const wxString& path );

	/**
	 * Parses all links in the ED2KLink file and resets it.
	 */
	void	AddLinksFromFile();

	/**
	 * Stops performing UDP requests.
	 */
	void	DoStopUDPRequests();

	
	void	ProcessLocalRequests();
	
	bool	SendNextUDPPacket();
	int		GetMaxFilesPerUDPServerPacket() const;
	bool	SendGlobGetSourcesUDPPacket(CMemFile& data);
	
	void 	AddToResolve(const CMD4Hash& fileid, const wxString& pszHostname, uint16 port, const wxString& hash, uint8 cryptoptions);

	//! The mutex assosiated with this class, mutable to allow for const functions.
	mutable wxMutex m_mutex;


	uint32		m_datarate;
	uint32		m_lastDiskCheck;
	uint32		m_lastudpsearchtime;
	uint32		m_lastsorttime;
	uint32		m_lastudpstattime;
	uint32		m_nLastED2KLinkCheck;
	uint8		m_cRequestsSentToServer;
	uint32		m_dwNextTCPSrcReq;
	uint8		m_udcounter;
	CServer*	m_udpserver;

	
	/**
	 * Structure used to store sources with dynamic hostnames.
	 */
	struct Hostname_Entry
	{
		//! The ID of the file the source provides.
		CMD4Hash fileid;
		//! The dynamic hostname.
		wxString strHostname;
		//! The user-port of the source.
		uint16 port;
		//! The hash of the source
		wxString hash;
		//! The cryptoptions for the source
		uint8 cryptoptions;
	};

	std::deque<Hostname_Entry>	m_toresolve;
	
	typedef std::deque<CPartFile*> FileQueue;
	FileQueue m_filelist;
	
	std::list<CPartFile*>		m_localServerReqQueue;

	//! Observer used to keep track of which servers have yet to be asked for sources
	CQueueObserver<CServer*>	m_queueServers;
	
	//! Observer used to keep track of which file to send UDP requests for
	CQueueObserver<CPartFile*>	m_queueFiles;
	
	/* Kad Stuff */
	uint32		lastkademliafilerequest;
	
};

#endif // DOWNLOADQUEUE_H
// File_checked_for_headers
