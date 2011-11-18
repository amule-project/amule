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

#ifndef UPDOWNCLIENTEC_H
#define UPDOWNCLIENTEC_H

#include <ec/cpp/ECID.h>	// Needed for CECID
#include "BitVector.h"		// Needed for BitVector
#include "ClientRef.h"		// Needed for debug defines

class CKnownFile;
class CPartFile;
class CFriend;


class CUpDownClient : public CECID
{
	friend class CUpDownClientListRem;
	friend class CClientRef;
private:
	/**
	 * Please note that only the ClientList is allowed to delete the clients.
	 */
	~CUpDownClient();

	/**
	 * Reference count which is increased whenever client is linked to a clientref.
	 * Clients are to be stored only by ClientRefs, CUpDownClient * are for temporary 
	 * use only.
	 * Linking is done only by CClientRef which is friend, so methods are private.
	 */
	uint16	m_linked;
#ifdef DEBUG_ZOMBIE_CLIENTS
	bool	m_linkedDebug;
	std::multiset<wxString> m_linkedFrom;
	void	Link(const wxString& from)		{ m_linked++; m_linkedFrom.insert(from); }
	void	Unlink(const wxString& from);
	wxString GetLinkedFrom() {
		wxString ret;
		for (std::multiset<wxString>::iterator it = m_linkedFrom.begin(); it != m_linkedFrom.end(); it++) {
			ret += *it + wxT(", ");
		}
		return ret;
	}
#else
	void	Link()		{ m_linked++; }
	void	Unlink();
#endif

public:
	CUpDownClient(class CEC_UpDownClient_Tag *);

	void	RequestSharedFileList();


// Wrapped by CClientRef
	bool				ExtProtocolAvailable() const			{ return m_bEmuleProtocol; }
	uint16				GetAvailablePartCount() const			{ return m_AvailPartCount; }
	const wxString&		GetClientFilename() const				{ return m_clientFilename; }
	const wxString&		GetClientModString() const				{ return m_strModVersion; }
	const wxString&		GetClientOSInfo() const					{ return m_sClientOSInfo; }
	uint8				GetClientSoft() const					{ return m_clientSoft; }
	const wxString&		GetClientVerString() const				{ return m_fullClientVerString; }
	uint64				GetDownloadedTotal() const;
	uint8				GetDownloadState() const				{ return m_nDownloadState; }
	CFriend*			GetFriend() const						{ return m_Friend; }
	bool				GetFriendSlot() const					{ return m_bFriendSlot; }
	wxString			GetFullIP() const						{ return Uint32toStringIP(m_dwUserIP); }
	uint16				GetKadPort() const						{ return m_nKadPort; }
	float				GetKBpsDown() const						{ return m_kBpsDown; }
	uint32				GetIP() const							{ return m_dwUserIP; }
	uint16				GetLastDownloadingPart() const			{ return m_lastDownloadingPart; }
	uint16				GetNextRequestedPart() const			{ return m_nextRequestedPart; }
	uint8				GetObfuscationStatus() const			{ return m_obfuscationStatus; }
	uint16				GetOldRemoteQueueRank() const			{ return m_nOldRemoteQueueRank; }
	const BitVector&	GetPartStatus() const					{ return m_downPartStatus; }
	uint16				GetRemoteQueueRank() const				{ return m_nRemoteQueueRank; }
	CPartFile*			GetRequestFile() const					{ return m_reqfile; }
	uint32				GetScore() const						{ return m_score; }
	double 				GetScoreRatio() const;
	uint32				GetServerIP() const						{ return m_dwServerIP; }
	const wxString		GetServerName() const					{ return m_ServerName; }
	uint16				GetServerPort() const					{ return m_nServerPort; }
	const wxString&		GetSoftStr() const						{ return m_clientSoftString; }
	const wxString&		GetSoftVerStr() const					{ return m_clientVerString; }
	ESourceFrom			GetSourceFrom() const					{ return m_nSourceFrom; }
	uint64				GetTransferredDown() const				{ return m_nTransferredDown; }
	uint64				GetTransferredUp() const				{ return m_nTransferredUp; }
	uint32				GetUploadDatarate() const				{ return m_nUpDatarate; }
	uint64				GetUploadedTotal() const;
	const CKnownFile*	GetUploadFile() const					{ return m_uploadingfile; }
	uint16				GetUploadQueueWaitingPosition() const	{ return m_waitingPosition; }
	uint8				GetUploadState() const					{ return m_nUploadState; }
	size_t				GetUpPartCount() const					{ return m_upPartStatus.size(); }
	const CMD4Hash&		GetUserHash() const						{ return m_UserHash; }
	uint32				GetUserIDHybrid() const					{ return m_nUserIDHybrid; }
	const wxString&		GetUserName() const						{ return m_Username; }
	uint16				GetUserPort() const						{ return m_nUserPort; }
	uint32				GetVersion() const						{ return m_nClientVersion; }
	bool				HasDisabledSharedFiles() const			{ return m_fNoViewSharedFiles; }
	bool				HasLowID() const						{ return IsLowID(m_nUserIDHybrid); }
	bool				IsBadGuy() const						{ return m_identState == IS_IDBADGUY; }
	bool				IsFriend() const						{ return m_Friend != NULL; }
	bool				IsIdentified() const					{ return m_identState == IS_IDENTIFIED; }
	bool				IsRemoteQueueFull() const				{ return m_bRemoteQueueFull; }
	bool				IsUpPartAvailable(uint16 iPart) const	{ return ( iPart < m_upPartStatus.size() ) ? m_upPartStatus.get(iPart) : 0; }
	void				RequestSharedFileList() const;
	void				SetFriend(CFriend* newfriend)			{ m_Friend = newfriend; }
	void				SetFriendSlot(bool bNV)					{ m_bFriendSlot = bNV; }
	bool				SUIFailed() const						{ return m_identState == IS_IDFAILED; }
	bool				SUINeeded() const						{ return m_identState == IS_IDNEEDED; }
	bool				SUINotSupported() const					{ return m_identState == IS_NOTAVAILABLE; }
	bool				SwapToAnotherFile(bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile = NULL);


private:
	bool		m_bEmuleProtocol;
	uint16		m_AvailPartCount;
	wxString	m_clientFilename;
	wxString	m_strModVersion;
	wxString	m_sClientOSInfo;
	uint8		m_clientSoft;
	wxString	m_fullClientVerString;
	uint8		m_nDownloadState;
	CFriend*	m_Friend;
	bool		m_bFriendSlot;
	uint16		m_nKadPort;
	float		m_kBpsDown;
	uint32		m_dwUserIP;
	uint16		m_lastDownloadingPart;
	uint16		m_nextRequestedPart;
	uint8		m_obfuscationStatus;
	uint16		m_nOldRemoteQueueRank;
	BitVector	m_downPartStatus;
	uint16		m_nRemoteQueueRank;
	CPartFile*	m_reqfile;
	uint32		m_score;
	uint32		m_dwServerIP;
	wxString	m_ServerName;
	uint16		m_nServerPort;
	wxString	m_clientSoftString;
	wxString	m_clientVerString;
	ESourceFrom	m_nSourceFrom;
	uint64		m_nTransferredDown;
	uint64		m_nTransferredUp;
	uint32		m_nUpDatarate;
	CKnownFile*	m_uploadingfile;
	uint16		m_waitingPosition;
	uint8		m_nUploadState;
	BitVector	m_upPartStatus;
	CMD4Hash	m_UserHash;
	uint32		m_nUserIDHybrid;
	wxString	m_Username;
	uint16		m_nUserPort;
	uint32		m_nClientVersion;
	bool		m_fNoViewSharedFiles;
	EIdentState	m_identState;
	bool		m_bRemoteQueueFull;

// other stuff
	CClientCredits	*credits;
};


#endif

