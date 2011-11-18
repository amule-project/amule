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

#include "ClientRef.h"
#include "amule.h"				// Needed fot theApp

#ifdef CLIENT_GUI
#include "UpDownClientEC.h"	// Needed for CUpDownClient
#else
#include "updownclient.h"	// Needed for CUpDownClient
#endif

#ifdef DEBUG_ZOMBIE_CLIENTS
#define MFROM m_from
#define ASSIGN_MFROM(a) m_from = a
#else
#define MFROM
#define ASSIGN_MFROM(a)
#endif


CClientRef::CClientRef(const CClientRef& ref)
{
	m_client = ref.m_client;
	ASSIGN_MFROM(wxT("copy ctor of ") + ref.m_from);
	if (m_client) {
		m_client->Link(MFROM);
	}
}


CClientRef::CClientRef(CUpDownClient * client LINKED_FROM)
{
	m_client = client;
	ASSIGN_MFROM(from);
	if (m_client) {
		m_client->Link(MFROM);
	}
}


void CClientRef::Link(CUpDownClient * client LINKED_FROM)
{
	Unlink();
	m_client = client;
	ASSIGN_MFROM(from);
	if (m_client) {
		m_client->Link(MFROM);
	}
}


void CClientRef::Unlink()
{
	if (m_client) {
		m_client->Unlink(MFROM);
		m_client = NULL;
	}
}


// in amulegui clients are never deleted except when they are marked as removed through EC
#ifndef CLIENT_GUI

CUpDownClient * CClientRef::GetClientChecked()
{
	if (m_client && m_client->HasBeenDeleted()) {
		m_client->Unlink(MFROM);
		m_client = NULL;
	}
	return m_client;
}


CClientRef&	CClientRef::GetRef()
{
	if (m_client && m_client->HasBeenDeleted()) {
		m_client->Unlink(MFROM);
		m_client = NULL;
	}
	return *this;
}


void CClientRef::Safe_Delete()
{
	CUpDownClient * client = m_client;
	if (client) {
		Unlink();
		client->Safe_Delete();
	}
}
#endif


#define WRAPC(func) CClientRef::func() const { return m_client->func(); }

uint32				WRAPC(ECID)
bool				WRAPC(ExtProtocolAvailable)
uint16				WRAPC(GetAvailablePartCount)
const wxString&		WRAPC(GetClientFilename)
const wxString&		WRAPC(GetClientModString)
const wxString&		WRAPC(GetClientOSInfo)
uint8				WRAPC(GetClientSoft)
const wxString&		WRAPC(GetClientVerString)
uint64				WRAPC(GetDownloadedTotal)
uint8				WRAPC(GetDownloadState)
CFriend*			WRAPC(GetFriend)
bool				WRAPC(GetFriendSlot)
wxString			WRAPC(GetFullIP)
uint32				WRAPC(GetIP)
uint16				WRAPC(GetKadPort)
float				WRAPC(GetKBpsDown)
uint16				WRAPC(GetLastDownloadingPart)
uint16				WRAPC(GetNextRequestedPart)
uint8				WRAPC(GetObfuscationStatus)
uint16				WRAPC(GetOldRemoteQueueRank)
const BitVector&	WRAPC(GetPartStatus)
uint16				WRAPC(GetRemoteQueueRank)
CPartFile*			WRAPC(GetRequestFile)
uint32				WRAPC(GetScore)
double 				WRAPC(GetScoreRatio)
uint32				WRAPC(GetServerIP)
const wxString		WRAPC(GetServerName)
uint16				WRAPC(GetServerPort)
const wxString&		WRAPC(GetSoftStr)
const wxString&		WRAPC(GetSoftVerStr)
int					WRAPC(GetSourceFrom)
uint64				WRAPC(GetTransferredDown)
uint64				WRAPC(GetTransferredUp)
uint32				WRAPC(GetUploadDatarate)
uint64				WRAPC(GetUploadedTotal)
const CKnownFile*	WRAPC(GetUploadFile)
uint16				WRAPC(GetUploadQueueWaitingPosition)
uint8				WRAPC(GetUploadState)
size_t				WRAPC(GetUpPartCount)
uint32				WRAPC(GetUserIDHybrid)
const wxString&		WRAPC(GetUserName)
uint16_t			WRAPC(GetUserPort)
const CMD4Hash&		WRAPC(GetUserHash)
uint32				WRAPC(GetVersion)
bool				WRAPC(HasDisabledSharedFiles)
bool				WRAPC(HasLowID)
bool				WRAPC(IsBadGuy)
bool				WRAPC(IsFriend)
bool				WRAPC(IsIdentified)
bool				WRAPC(IsRemoteQueueFull)
void				WRAPC(RequestSharedFileList)

bool		CClientRef::IsUpPartAvailable(uint16 iPart) const { return m_client->IsUpPartAvailable(iPart); }
void		CClientRef::SetFriend(CFriend* newfriend) const { m_client->SetFriend(newfriend); }
void		CClientRef::SetFriendSlot(bool bNV) const { m_client->SetFriendSlot(bNV); }

#ifndef CLIENT_GUI
void				WRAPC(ClearUploadFileID)
uint16				WRAPC(GetUDPPort)
void		CClientRef::SetCommentDirty(bool bDirty) const { m_client->SetCommentDirty(bDirty); }
#endif

bool		CClientRef::SwapToAnotherFile(bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile) const
{
	return m_client->SwapToAnotherFile(bIgnoreNoNeeded, ignoreSuspensions, bRemoveCompletely, toFile);
}

wxString	CClientRef::GetSecureIdentTextStatus() const
{
	wxString ret;
	if (theApp->CryptoAvailable()) {
		if (m_client->SUINotSupported()) {
			ret = _("Not supported");
		} else if (m_client->SUIFailed()) {
			ret = _("Failed");
		} else if (m_client->SUINeeded()) {
			ret = _("Not complete");
		} else if (m_client->IsBadGuy()) {
			ret = _("Bad Guy");
		} else if (m_client->IsIdentified()) {
			ret = _("Verified - OK");
		}	
	} else {
		ret = _("Not Available");
	}
	return ret;
}

