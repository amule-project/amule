//this file is part of aMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.amule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef SEARCHLIST_H
#define SEARCHLIST_H

#include "types.h"		// Needed for uint8, uint16 and uint32
#include "KnownFile.h"		// Needed for CAbstractFile
#include "mfc.h"		// Needed for CMap
#include "CMemFile.h"		// Needed for CMemFile

class CSafeMemFile;

class CSearchFile : public CAbstractFile {
	friend class CPartFile;
public:
	//CSearchFile() {};
	CSearchFile(CMemFile* in_data, uint32 nSearchID, uint32 nServerIP=0, uint16 nServerPort=0, LPCTSTR pszDirectory = NULL);
	CSearchFile(uint32 nSearchID, const uchar* pucFileHash, uint32 uFileSize, LPCTSTR pszFileName, int iFileType, int iAvailability);
	CSearchFile(CSearchFile* copyfrom);
	//CSearchFile(CFile* in_data, uint32 nSearchID);
	~CSearchFile();

	uint32	GetIntTagValue(uint8 tagname);
	char*	GetStrTagValue(uint8 tagname);
	uint32	AddSources(uint32 count);
	uint32	GetSourceCount();
	uint32	GetSearchID() {return m_nSearchID;}

	uint32	GetClientID() const			{ return m_nClientID; }
	void		SetClientID(uint32 nClientID)	{ m_nClientID = nClientID; }
	uint16	GetClientPort() const			{ return m_nClientPort; }
	void		SetClientPort(uint16 nPort)		{ m_nClientPort = nPort; }
	uint32	GetClientServerIP() const		{ return m_nClientServerIP; }
	void		SetClientServerIP(uint32 uIP)   	{ m_nClientServerIP = uIP; }
	uint16	GetClientServerPort() const		{ return m_nClientServerPort; }
	void		SetClientServerPort(uint16 nPort) { m_nClientServerPort = nPort; }
	int		GetClientsCount() const		{ return ((GetClientID() && GetClientPort()) ? 1 : 0) + m_aClients.GetSize(); }
	
	CSearchFile* GetListParent() const		{ return m_list_parent; }
	
	struct SClient {
		SClient() {
			m_nIP = m_nPort = m_nServerIP = m_nServerPort = 0;
		}
		SClient(uint32 nIP, unsigned int nPort, uint32 nServerIP, unsigned int nServerPort) {
			m_nIP = nIP;
			m_nPort = nPort;
			m_nServerIP = nServerIP;
			m_nServerPort = nServerPort;
		}
		uint32 m_nIP;
		uint32 m_nServerIP;
		uint16 m_nPort;
		uint16 m_nServerPort;
	};
	void AddClient(SClient* client) { m_aClients.Add(client); }
	const CArray<SClient*,SClient*>& GetClients() { return m_aClients; }

	struct SServer {
		SServer() {
			m_nIP = m_nPort = 0;
			m_uAvail = 0;
		}
		SServer(uint32 nIP, unsigned int nPort) {
			m_nIP = nIP;
			m_nPort = nPort;
			m_uAvail = 0;
		}
		uint32 m_nIP;
		uint16 m_nPort;
		unsigned int m_uAvail;
	};
	void AddServer(SServer* server) { m_aServers.Add(server); }
	const CArray<SServer*,SServer*>& GetServers() const { return m_aServers; }
	SServer* GetServer(int iServer) { return m_aServers[iServer]; }


private:
	uint8	clientip[4];
	uint16	clientport;
	uint32	m_nSearchID;
	CArray<CTag*,CTag*> taglist;

	uint32	m_nClientID;
	uint16	m_nClientPort;
	uint32	m_nClientServerIP;
	uint16	m_nClientServerPort;
	CArray<SClient*,SClient*> m_aClients;
	CArray<SServer*,SServer*> m_aServers;
	bool		 m_list_bExpanded;
	uint16	 m_list_childcount;
	CSearchFile* m_list_parent;
	bool		 m_bPreviewPossible;
	//CArray<CxImage*,CxImage*> m_listImages;
	LPSTR m_pszDirectory;
};

class CSearchList
{
friend class CSearchListCtrl;
public:
	CSearchList();
	~CSearchList();
	void	Clear();
	void	NewSearch(CString resTypes, uint16 nSearchID);
	uint16	ProcessSearchanswer(char* packet, uint32 size, CUpDownClient* Sender = NULL);
	uint16	ProcessSearchanswer(char* packet, uint32 size, uint32 nServerIP, uint16 nServerPort);
	uint16	ProcessSearchanswer(char* in_packet, uint32 size, CUpDownClient* Sender, bool* pbMoreResultsAvailable, LPCTSTR pszDirectory);
    	uint16	ProcessUDPSearchanswer(CSafeMemFile* packet, uint32 nServerIP, uint16 nServerPort);	
   // uint16	ProcessUDPSearchanswer(char* packet, uint32 size);

	uint16	GetResultCount();
	uint16	GetResultCount(uint32 nSearchID);
	void	RemoveResults(  uint32 nSearchID );
	void	RemoveResults( CSearchFile* todel );
	void	ShowResults(uint32 nSearchID);
	CString GetWebList(CString linePattern,int sortby,bool asc) const;
	void	AddFileToDownloadByHash(const uchar* hash)		{AddFileToDownloadByHash(hash,0);}
	void	AddFileToDownloadByHash(const uchar* hash, uint8 cat);
	uint16	GetFoundFiles(uint32 searchID);

private:
	bool AddToList(CSearchFile* toadd, bool bClientResponse = false);
	CTypedPtrList<CPtrList, CSearchFile*> list;
	CMap<uint32, uint32, uint16, uint16> foundFilesCount;

	CString myHashList;
	CString resultType;
	
	uint32	m_nCurrentSearch;
};

#endif // SEARCHLIST_H
