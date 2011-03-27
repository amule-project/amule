//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003-2011 Stu Redman ( sturedman@amule.org / http://www.amule.org )
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

#ifndef CLIENTREF_H
#define CLIENTREF_H

//
// Clients are stored in many places. To prevent problems when a client gets deleted
// and later a pointer to a deleted client is referenced, clients are now stored in 
// this class only. It uses reference counting in the client which deletes the client only 
// after the last reference has been unlinked.
// Also the class is used as abstraction layer for the CUpDownClient class.
//

#include <list>
#include <set>
#include "Types.h"

class CUpDownClient;
class CKnownFile;
class CPartFile;
class CFriend;
class BitVector;
class CMD4Hash;

#ifdef __WXDEBUG__
#define DEBUG_ZOMBIE_CLIENTS
#endif

#ifdef DEBUG_ZOMBIE_CLIENTS
#define LINKED_FROM , wxString from
#define CLIENT_DEBUGSTRING(a) , wxT(a)
#define CCLIENTREF(a, b) CClientRef(a, b)
#else
#define LINKED_FROM
#define CLIENT_DEBUGSTRING(a)
#define CCLIENTREF(a, b) CClientRef(a)
#endif

class CClientRef {

	CUpDownClient * m_client;
#ifdef DEBUG_ZOMBIE_CLIENTS
	wxString	m_from;
#endif

public:
	CClientRef()	{ m_client = NULL; }
	CClientRef(const CClientRef&);
	CClientRef(CUpDownClient * client LINKED_FROM);
	CClientRef(class CEC_UpDownClient_Tag *);
	~CClientRef()	{ Unlink(); }
	CClientRef& operator = (const CClientRef& ref)
	{
#ifdef DEBUG_ZOMBIE_CLIENTS
		m_from = wxT("assigned from ") + ref.m_from;
		Link(ref.m_client, m_from); 
#else
		Link(ref.m_client); 
#endif
		return *this; 
	}


// For sets and maps
	bool operator <  (const CClientRef & other) const { return m_client < other.m_client; }
	bool operator == (const CClientRef & other) const { return m_client == other.m_client; }
	bool operator != (const CClientRef & other) const { return m_client != other.m_client; }

	void swap(CClientRef & other)
	{
		CUpDownClient * c =	m_client;
		m_client = other.m_client;
		other.m_client = c;
#ifdef DEBUG_ZOMBIE_CLIENTS
		m_from.swap(other.m_from);
#endif
	}

	void Link(CUpDownClient * client LINKED_FROM);
	void Unlink();
	bool IsLinked() const		{ return m_client != NULL; }

	// Return client, no matter if it was deleted
	CUpDownClient * GetClient() const	{ return m_client; }
	// Check if client was deleted, if yes unlink and return zero
	CUpDownClient * GetClientChecked();
	// Check if client was deleted, if yes unlink
	CClientRef&		GetRef();

	void Safe_Delete();

// Wrapper methods
	void				ClearUploadFileID() const;		// only wrapper const
	uint32				ECID() const;
	bool				ExtProtocolAvailable() const;
	uint16				GetAvailablePartCount() const;
	const wxString&		GetClientFilename() const;
	const wxString&		GetClientModString() const;
	const wxString&		GetClientOSInfo() const;
	uint8				GetClientSoft() const;
	const wxString&		GetClientVerString() const;
	uint64				GetDownloadedTotal() const;
	uint8				GetDownloadState() const;
	CFriend*			GetFriend() const;
	bool				GetFriendSlot() const;
	wxString			GetFullIP() const;
	uint16				GetKadPort() const;
	float				GetKBpsDown() const;
	uint32				GetIP() const;
	uint16				GetLastDownloadingPart() const;
	uint16				GetNextRequestedPart() const;
	uint8				GetObfuscationStatus() const;
	uint16				GetOldRemoteQueueRank() const;
	const BitVector&	GetPartStatus() const;
	uint16				GetRemoteQueueRank() const;
	CPartFile*			GetRequestFile() const;
	uint32				GetScore() const;
	double 				GetScoreRatio() const;
	uint32				GetServerIP() const;
	const wxString		GetServerName() const;
	uint16				GetServerPort() const;
	const wxString&		GetSoftStr() const;
	const wxString&		GetSoftVerStr() const;
	int					GetSourceFrom() const;	// ESourceFrom
	wxString			GetSecureIdentTextStatus() const;
	uint64				GetTransferredDown() const;
	uint64				GetTransferredUp() const;
	uint16				GetUDPPort() const;
	uint32				GetUploadDatarate() const;
	uint64				GetUploadedTotal() const;
	const CKnownFile*	GetUploadFile() const;
	uint16				GetUploadQueueWaitingPosition() const;
	uint8				GetUploadState() const;
	size_t				GetUpPartCount() const;
	const CMD4Hash&		GetUserHash() const;
	uint32				GetUserIDHybrid() const;
	const wxString&		GetUserName() const;
	uint16_t			GetUserPort() const;
	uint32				GetVersion() const;
	bool				HasDisabledSharedFiles() const;
	bool				HasLowID() const;
	bool				IsBadGuy() const;
	bool				IsFriend() const;
	bool				IsIdentified() const;
	bool				IsRemoteQueueFull() const;
	bool				IsUpPartAvailable(uint16 iPart) const;
	void				RequestSharedFileList() const;
	void				SetCommentDirty(bool bDirty = true) const;		// only wrapper const
	void				SetFriend(CFriend* newfriend) const;			// only wrapper const
	void				SetFriendSlot(bool bNV) const;					// only wrapper const
	bool				SwapToAnotherFile(bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile = NULL) const;	// only wrapper const

};

// efficient std::swap for it
namespace std
{
    template<> inline void swap(CClientRef& a, CClientRef& b)
	{
		a.swap(b);
	}
}


typedef std::list<CClientRef> CClientRefList;
typedef std::set<CClientRef>  CClientRefSet;


#endif

