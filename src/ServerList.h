//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef SERVERLIST_H
#define SERVERLIST_H

#include "ObservableQueue.h"

class CServer;
class CPacket;

class CServerList : public CObservableQueue<CServer*>
{
	friend class CServerListCtrl;

public:
	CServerList();
	~CServerList();
	bool		Init();
	bool		AddServer(CServer* in_server, bool fromUser = false);
	void		RemoveServer(CServer* in_server);
	void		RemoveAllServers();
	void		RemoveDeadServers();
	bool		LoadServerMet(const CPath& path);
	bool		SaveServerMet();
	void		ServerStats();
	void		ResetServerPos()	{m_serverpos = m_servers.begin();}
	CServer*	GetNextServer(bool bOnlyObfuscated = false);
	size_t		GetServerCount()	{return m_servers.size();}
	CServer*	GetServerByAddress(const wxString& address, uint16 port) const;
	CServer*	GetServerByIP(uint32 nIP) const;
	CServer*	GetServerByIPTCP(uint32 nIP, uint16 nPort) const;
	CServer*	GetServerByIPUDP(uint32 nIP, uint16 nUDPPort, bool bObfuscationPorts = true) const;
	CServer*	GetServerByECID(uint32 ecid) const;
	void		GetStatus(uint32 &failed, uint32 &user, uint32 &file, uint32 &tuser, uint32 &tfile, float &occ);
	void		GetUserFileStatus( uint32 &user, uint32 &file);
	bool		IsInitialized() const { return m_initialized; }
	void		Sort();
	void		UpdateServerMetFromURL(const wxString& strURL);
	bool		DownloadFinished(uint32 result);
	void		AutoDownloadFinished(uint32 result);
	uint32		GetAvgFile() const;

	std::vector<const CServer*> CopySnapshot() const;

	/** Refilters all servers though the IPFilter. */
	void FilterServers();

	void CheckForExpiredUDPKeys();

	/**
	 * Marks the specified server as static or not.
	 *
	 * @param The server to be marked or unmarked as static.
	 * @param The new static state.
	 *
	 * Other than setting the static setting of the specified server, it
	 * also adds or removes the server from the static-list file.
	 */
	void		SetStaticServer(CServer* server, bool isStatic);
	void		SetServerPrio(CServer* server, uint32 prio);

private:
	virtual void	ObserverAdded( ObserverType* );
	void		AutoUpdate();
	CServer*	GetNextStatServer();

	wxString	m_staticServersConfig;
	void		LoadStaticServers();
	void		SaveStaticServers();
	uint8		current_url_index;

	typedef std::list<CServer*>	CInternalList;
	CInternalList			m_servers;
	CInternalList::const_iterator	m_serverpos;
	CInternalList::const_iterator	m_statserverpos;

	uint32		m_nLastED2KServerLinkCheck;// emanuelw(20030924) added
	wxString	m_URLUpdate;
	bool		m_initialized;
};

#endif // SERVERLIST_H
// File_checked_for_headers
