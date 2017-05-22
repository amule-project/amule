//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "updownclient.h"		// Needed for CUpDownClient

#include <protocol/Protocols.h>
#include <protocol/ed2k/Client2Client/TCP.h>
#include <protocol/ed2k/Client2Client/UDP.h>
#include <common/EventIDs.h>
#include <common/Macros.h>
#include <common/Constants.h>

#include <zlib.h>
#include <cmath>		// Needed for std:exp

#include "ClientCredits.h"	// Needed for CClientCredits
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "Preferences.h"	// Needed for thePrefs
#include "Packet.h"		// Needed for CPacket
#include "MemFile.h"		// Needed for CMemFile
#include "ClientTCPSocket.h"// Needed for CClientTCPSocket
#include "ListenSocket.h"	// Needed for CListenSocket
#include "amule.h"		// Needed for theApp
#include "PartFile.h"		// Needed for CPartFile
#include "SharedFileList.h"
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"
#include "GuiEvents.h"		// Needed for Notify_*
#include "UploadQueue.h"	// Needed for CUploadQueue


#ifdef __MULE_UNUSED_CODE__
// This function is left as a reminder.
// Changes here _must_ be reflected in CClientList::FindMatchingClient.
bool CUpDownClient::Compare(const CUpDownClient* tocomp, bool bIgnoreUserhash) const
{
	if (!tocomp) {
		// should we wxASSERT here?
		return false;
	}

	//Compare only the user hash..
	if(!bIgnoreUserhash && HasValidHash() && tocomp->HasValidHash()) {
	    return GetUserHash() == tocomp->GetUserHash();
	}

	if (HasLowID()) {
		//User is firewalled.. Must do two checks..
		if (GetIP()!=0	&& GetIP() == tocomp->GetIP()) {
			//The IP of both match
			if (GetUserPort()!=0 && GetUserPort() == tocomp->GetUserPort()) {
				//IP-UserPort matches
				return true;
			}
			if (GetKadPort()!=0	&& GetKadPort() == tocomp->GetKadPort()) {
				//IP-KadPort Matches
				return true;
			}
		}

		if (GetUserIDHybrid()!=0
						&& GetUserIDHybrid() == tocomp->GetUserIDHybrid()
						&& GetServerIP()!=0
						&& GetServerIP() == tocomp->GetServerIP()
						&& GetServerPort()!=0
						&& GetServerPort() == tocomp->GetServerPort()) {
			//Both have the same lowID, Same serverIP and Port..
			return true;
		}

		//Both IP, and Server do not match..
		return false;
    }

	//User is not firewalled.
	if (GetUserPort()!=0) {
		//User has a Port, lets check the rest.
		if (GetIP() != 0 && tocomp->GetIP() != 0) {
			//Both clients have a verified IP..
			if(GetIP() == tocomp->GetIP() && GetUserPort() == tocomp->GetUserPort()) {
				//IP and UserPort match..
				return true;
			}
		} else {
			//One of the two clients do not have a verified IP
			if (GetUserIDHybrid() == tocomp->GetUserIDHybrid() && GetUserPort() == tocomp->GetUserPort()) {
				//ID and Port Match..
				return true;
			}
		}
    }

	if(GetKadPort()!=0) {
		//User has a Kad Port.
		if(GetIP() != 0 && tocomp->GetIP() != 0) {
			//Both clients have a verified IP.
			if(GetIP() == tocomp->GetIP() && GetKadPort() == tocomp->GetKadPort()) {
				//IP and KadPort Match..
				return true;
			}
		} else {
			//One of the users do not have a verified IP.
			if (GetUserIDHybrid() == tocomp->GetUserIDHybrid() && GetKadPort() == tocomp->GetKadPort()) {
				//ID and KadProt Match..
				return true;
			}
		}
	}

	//No Matches..
	return false;
}
#endif


bool CUpDownClient::AskForDownload()
{
	// 0.42e
	if (theApp->listensocket->TooManySockets()) {
		if (!m_socket) {
			if (GetDownloadState() != DS_TOOMANYCONNS) {
				SetDownloadState(DS_TOOMANYCONNS);
			}
			return true;
		} else if (!m_socket->IsConnected()) {
			if (GetDownloadState() != DS_TOOMANYCONNS) {
				SetDownloadState(DS_TOOMANYCONNS);
			}
			return true;
		}
	}
	m_bUDPPending = false;
	m_dwLastAskedTime = ::GetTickCount();
	SetDownloadState(DS_CONNECTING);
	SetSentCancelTransfer(0);
	return TryToConnect();
}


void CUpDownClient::SendStartupLoadReq()
{
	// 0.42e
	if (m_socket==NULL || m_reqfile==NULL) {
		return;
	}
	SetDownloadState(DS_ONQUEUE);
	CMemFile dataStartupLoadReq(16);
	dataStartupLoadReq.WriteHash(m_reqfile->GetFileHash());
	CPacket* packet = new CPacket(dataStartupLoadReq, OP_EDONKEYPROT, OP_STARTUPLOADREQ);
	theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
	AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_STARTUPLOADREQ to ") + GetFullIP());
	SendPacket(packet, true, true);
}


bool CUpDownClient::IsSourceRequestAllowed()
{
	//#warning REWRITE - Source swapping from eMule.
	// 0.42e
	uint32 dwTickCount = ::GetTickCount() + CONNECTION_LATENCY;
	uint32 nTimePassedClient = dwTickCount - GetLastSrcAnswerTime();
	uint32 nTimePassedFile   = dwTickCount - m_reqfile->GetLastAnsweredTime();
	bool bNeverAskedBefore = (GetLastAskedForSources() == 0);

	uint32 uSources = m_reqfile->GetSourceCount();
	return (
		// if client has the correct extended protocol
		ExtProtocolAvailable() && (SupportsSourceExchange2() || GetSourceExchange1Version() > 1) &&
		// AND if we need more sources
		thePrefs::GetMaxSourcePerFileSoft() > uSources &&
		// AND if...
		(
		//source is not complete and file is very rare
			( !m_bCompleteSource
			&& (bNeverAskedBefore || nTimePassedClient > SOURCECLIENTREASKS)
			&& (uSources <= RARE_FILE/5)
			) ||
			//source is not complete and file is rare
			( !m_bCompleteSource
			&& (bNeverAskedBefore || nTimePassedClient > SOURCECLIENTREASKS)
			&& (uSources <= RARE_FILE || uSources - m_reqfile->GetValidSourcesCount() <= RARE_FILE / 2)
			&& (nTimePassedFile > SOURCECLIENTREASKF)
			) ||
			// OR if file is not rare
			( (bNeverAskedBefore || nTimePassedClient > (unsigned)(SOURCECLIENTREASKS * MINCOMMONPENALTY))
			&& (nTimePassedFile > (unsigned)(SOURCECLIENTREASKF * MINCOMMONPENALTY))
			)
		)
	);
}


void CUpDownClient::SendFileRequest()
{
	wxCHECK_RET(m_reqfile, wxT("Cannot request file when no reqfile is set"));

	CMemFile dataFileReq(16+16);
	dataFileReq.WriteHash(m_reqfile->GetFileHash());

	if (SupportMultiPacket()) {
		DEBUG_ONLY( wxString sent_opcodes; )

		if (SupportExtMultiPacket()) {
			dataFileReq.WriteUInt64(m_reqfile->GetFileSize());
		}

		AddDebugLogLineN(logClient, wxT("Sending file request to client"));

		dataFileReq.WriteUInt8(OP_REQUESTFILENAME);
		DEBUG_ONLY( sent_opcodes += wxT("|RFNM|"); )
		// Extended information
		if (GetExtendedRequestsVersion() > 0) {
			m_reqfile->WritePartStatus(&dataFileReq);
		}
		if (GetExtendedRequestsVersion() > 1) {
			m_reqfile->WriteCompleteSourcesCount(&dataFileReq);
		}
		if (m_reqfile->GetPartCount() > 1) {
			DEBUG_ONLY( sent_opcodes += wxT("|RFID|"); )
			dataFileReq.WriteUInt8(OP_SETREQFILEID);
		}
		if (IsEmuleClient()) {
			SetRemoteQueueFull( true );
			SetRemoteQueueRank(0);
		}
		if (IsSourceRequestAllowed()) {
			if (SupportsSourceExchange2()){
				DEBUG_ONLY( sent_opcodes += wxT("|RSRC2|"); )
				dataFileReq.WriteUInt8(OP_REQUESTSOURCES2);
				dataFileReq.WriteUInt8(SOURCEEXCHANGE2_VERSION);
				const uint16 nOptions = 0; // 16 ... Reserved
				dataFileReq.WriteUInt16(nOptions);
			} else {
				DEBUG_ONLY( sent_opcodes += wxT("|RSRC|"); )
				dataFileReq.WriteUInt8(OP_REQUESTSOURCES);
			}
			m_reqfile->SetLastAnsweredTimeTimeout();
			SetLastAskedForSources();
		}
		if (IsSupportingAICH()) {
			DEBUG_ONLY( sent_opcodes += wxT("|AFHR|"); )
			dataFileReq.WriteUInt8(OP_AICHFILEHASHREQ);
		}
		CPacket* packet = new CPacket(dataFileReq, OP_EMULEPROT, (SupportExtMultiPacket() ? OP_MULTIPACKET_EXT : OP_MULTIPACKET));
		theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
		AddDebugLogLineN(logLocalClient, CFormat(wxT("Local Client: %s (%s) to %s"))
			% (SupportExtMultiPacket() ? wxT("OP_MULTIPACKET_EXT") : wxT("OP_MULTIPACKET")) % sent_opcodes % GetFullIP());
		SendPacket(packet, true);
	} else {
		//This is extended information
		if (GetExtendedRequestsVersion() > 0 ) {
			m_reqfile->WritePartStatus(&dataFileReq);
		}
		if (GetExtendedRequestsVersion() > 1 ) {
			m_reqfile->WriteCompleteSourcesCount(&dataFileReq);
		}
		CPacket* packet = new CPacket(dataFileReq, OP_EDONKEYPROT, OP_REQUESTFILENAME);
		theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
		AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_REQUESTFILENAME to ") + GetFullIP() );
		SendPacket(packet, true);

		// 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
		// if the remote client answers the OP_REQUESTFILENAME with OP_REQFILENAMEANSWER the file is shared by the remote client. if we
		// know that the file is shared, we know also that the file is complete and don't need to request the file status.

		// Sending the packet could have deleted the client, check m_reqfile
		if (m_reqfile && (m_reqfile->GetPartCount() > 1)) {
			CMemFile dataSetReqFileID(16);
			dataSetReqFileID.WriteHash(m_reqfile->GetFileHash());
			packet = new CPacket(dataSetReqFileID, OP_EDONKEYPROT, OP_SETREQFILEID);
			theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
			AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_SETREQFILEID to ") + GetFullIP());
			SendPacket(packet, true);
		}

		if (IsEmuleClient()) {
			SetRemoteQueueFull( true );
			SetRemoteQueueRank(0);
		}

		// Sending the packet could have deleted the client, check m_reqfile
		if (m_reqfile && IsSourceRequestAllowed()) {
			m_reqfile->SetLastAnsweredTimeTimeout();

			CMemFile packetdata;

			if (SupportsSourceExchange2()) {
				packetdata.WriteUInt8(SOURCEEXCHANGE2_VERSION);
				packetdata.WriteUInt16(0 /* Reserved */);
			}

			packetdata.WriteHash(m_reqfile->GetFileHash());

			packet = new CPacket(packetdata, OP_EMULEPROT, SupportsSourceExchange2() ? OP_REQUESTSOURCES2 : OP_REQUESTSOURCES);

			theStats::AddUpOverheadSourceExchange(packet->GetPacketSize());
			AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_REQUESTSOURCES to ") + GetFullIP() );
			SendPacket(packet,true,true);
			SetLastAskedForSources();
		}

		// Sending the packet could have deleted the client, check m_reqfile
		if (m_reqfile && IsSupportingAICH()) {
			packet = new CPacket(OP_AICHFILEHASHREQ,16,OP_EMULEPROT);
			packet->Copy16ToDataBuffer((const char *)m_reqfile->GetFileHash().GetHash());
			theStats::AddUpOverheadOther(packet->GetPacketSize());
			AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_AICHFILEHASHREQ to ") + GetFullIP());
			SendPacket(packet,true,true);
		}
	}
}


void CUpDownClient::ProcessFileInfo(const CMemFile* data, const CPartFile* file)
{
	// 0.42e
	if (file==NULL) {
		throw wxString(wxT("ERROR: Wrong file ID (ProcessFileInfo; file==NULL)"));
	}
	if (m_reqfile==NULL) {
		throw wxString(wxT("ERROR: Wrong file ID (ProcessFileInfo; m_reqfile==NULL)"));
	}
	if (file != m_reqfile) {
		throw wxString(wxT("ERROR: Wrong file ID (ProcessFileInfo; m_reqfile!=file)"));
	}

	m_clientFilename = data->ReadString((GetUnicodeSupport() != utf8strNone));

	// 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
	// if the remote client answers the OP_REQUESTFILENAME with OP_REQFILENAMEANSWER the file is shared by the remote client. if we
	// know that the file is shared, we know also that the file is complete and don't need to request the file status.
	if (m_reqfile->GetPartCount() == 1) {
		m_nPartCount = m_reqfile->GetPartCount();

		m_reqfile->UpdatePartsFrequency( this, false );	// Decrement
		m_downPartStatus.setsize( m_nPartCount, 1 );
		m_reqfile->UpdatePartsFrequency( this, true );	// Increment

		m_bCompleteSource = true;

		UpdateDisplayedInfo();
		// even if the file is <= PARTSIZE, we _may_ need the hashset for that file (if the file size == PARTSIZE)
		if (m_reqfile->IsHashSetNeeded()) {
			if (m_socket) {
				CPacket* packet = new CPacket(OP_HASHSETREQUEST,16, OP_EDONKEYPROT);
				packet->Copy16ToDataBuffer((const char *)m_reqfile->GetFileHash().GetHash());
				theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
				AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_HASHSETREQUEST to ") + GetFullIP());
				SendPacket(packet,true,true);
				SetDownloadState(DS_REQHASHSET);
				m_fHashsetRequesting = 1;
				m_reqfile->SetHashSetNeeded(false);
			} else {
				wxFAIL;
			}
		} else {
			SendStartupLoadReq();
		}
		m_reqfile->UpdatePartsInfo();
	}
}

void CUpDownClient::ProcessFileStatus(bool bUdpPacket, const CMemFile* data, const CPartFile* file)
{
	// 0.42e
	wxString strReqFileNull(wxT("ERROR: Wrong file ID (ProcessFileStatus; m_reqfile==NULL)"));

	if ( !m_reqfile || file != m_reqfile ){
		if (!m_reqfile) {
			throw strReqFileNull;
		}
		throw wxString(wxT("ERROR: Wrong file ID (ProcessFileStatus; m_reqfile!=file)"));
	}

	uint16 nED2KPartCount = data->ReadUInt16();

	m_reqfile->UpdatePartsFrequency( this, false );	// Decrement
	m_downPartStatus.clear();

	bool bPartsNeeded = false;
	if (!nED2KPartCount)
	{
		m_nPartCount = m_reqfile->GetPartCount();
		m_downPartStatus.setsize( m_nPartCount, 1);
		bPartsNeeded = true;
		m_bCompleteSource = true;
	}
	else
	{
		// Somehow this happened.
		if (!m_reqfile) {
			throw strReqFileNull;
		}
		if (m_reqfile->GetED2KPartCount() != nED2KPartCount)
		{
			wxString strError;
			strError << wxT("ProcessFileStatus - wrong part number recv=") << nED2KPartCount <<
				wxT("  expected=") << m_reqfile->GetED2KPartCount() << wxT(" ") <<
				m_reqfile->GetFileHash().Encode();
			m_nPartCount = 0;
			throw strError;
		}
		m_nPartCount = m_reqfile->GetPartCount();

		m_bCompleteSource = false;
		m_downPartStatus.setsize( m_nPartCount, 0 );
		uint16 done = 0;

		try {
			while (done != m_nPartCount) {
				uint8 toread = data->ReadUInt8();

				for ( uint8 i = 0;i < 8; i++ ) {
					bool status = ((toread>>i)&1)? 1:0;
					m_downPartStatus.set(done, status);

					if (status) {
						if (!m_reqfile->IsComplete(done)){
							bPartsNeeded = true;
						}
					}
					done++;
					if (done == m_nPartCount) {
						break;
					}
				}
			}
		} catch( ... ) {
			// We want the counts to be updated, even if we fail to read everything
			m_reqfile->UpdatePartsFrequency( this, true );	// Increment

			throw;
		}
	}

	m_reqfile->UpdatePartsFrequency( this, true );	// Increment

	UpdateDisplayedInfo();

	// NOTE: This function is invoked from TCP and UDP socket!
	if (!bUdpPacket) {
		if (!bPartsNeeded) {
			SetDownloadState(DS_NONEEDEDPARTS);
		} else if (m_reqfile->IsHashSetNeeded()) {
			//If we are using the eMule filerequest packets, this is taken care of in the Multipacket!
			if (m_socket) {
				CPacket* packet = new CPacket(OP_HASHSETREQUEST,16, OP_EDONKEYPROT);
				packet->Copy16ToDataBuffer((const char *)m_reqfile->GetFileHash().GetHash());
				theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
				AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_HASHSETREQUEST to ") + GetFullIP());
				SendPacket(packet, true, true);
				SetDownloadState(DS_REQHASHSET);
				m_fHashsetRequesting = 1;
				m_reqfile->SetHashSetNeeded(false);
			} else {
				wxFAIL;
			}
		}
		else {
			SendStartupLoadReq();
		}
	}
	else {
		if (!bPartsNeeded) {
			SetDownloadState(DS_NONEEDEDPARTS);
		} else {
			SetDownloadState(DS_ONQUEUE);
		}
	}
	m_reqfile->UpdatePartsInfo();
}

bool CUpDownClient::AddRequestForAnotherFile(CPartFile* file)
{
	if ( m_A4AF_list.find( file ) == m_A4AF_list.end() ) {
		// When we access a non-existing entry entry, it will be zeroed by default,
		// so we have to set NeededParts. All in one go.
		m_A4AF_list[file].NeededParts = true;
		file->AddA4AFSource( this );
		return true;
	} else {
		return false;
	}
}

bool CUpDownClient::DeleteFileRequest(CPartFile* file)
{
	return (m_A4AF_list.erase( file ) > 0);
}

void CUpDownClient::DeleteAllFileRequests()
{
	m_A4AF_list.clear();
}


/* eMule 0.30c implementation, i give it a try (Creteil) BEGIN ... */
void CUpDownClient::SetDownloadState(uint8 byNewState)
{
	if (m_nDownloadState != byNewState) {
		if (m_reqfile) {
			// Notify the client that this source has changed its state
			m_reqfile->ClientStateChanged( m_nDownloadState, byNewState );

			if (byNewState == DS_DOWNLOADING) {
				m_reqfile->AddDownloadingSource(this);
			} else if (m_nDownloadState == DS_DOWNLOADING) {
				m_reqfile->RemoveDownloadingSource(this);
			}
		}
		if (byNewState == DS_DOWNLOADING) {
			msReceivedPrev = GetTickCount();
			theStats::AddDownloadingSource();
		} else if (m_nDownloadState == DS_DOWNLOADING) {
			theStats::RemoveDownloadingSource();
		}

		if (m_nDownloadState == DS_DOWNLOADING) {
			m_nDownloadState = byNewState;
			ClearDownloadBlockRequests();

			kBpsDown = 0.0;
			bytesReceivedCycle = 0;
			msReceivedPrev = 0;
			if (byNewState == DS_NONE) {
				if (m_reqfile) {
					m_reqfile->UpdatePartsFrequency( this, false );	// Decrement
				}
				m_downPartStatus.clear();
				m_nPartCount = 0;
			}
			if (m_socket && byNewState != DS_ERROR) {
				m_socket->DisableDownloadLimit();
			}
		}
		m_nDownloadState = byNewState;
		if(GetDownloadState() == DS_DOWNLOADING) {
			if (IsEmuleClient()) {
				SetRemoteQueueFull(false);
			}
			SetRemoteQueueRank(0); // eMule 0.30c set like this ...
		}
		UpdateDisplayedInfo(true);
	}
}
/* eMule 0.30c implementation, i give it a try (Creteil) END ... */

void CUpDownClient::ProcessHashSet(const byte* packet, uint32 size)
{
	if ((!m_reqfile) || md4cmp(packet,m_reqfile->GetFileHash().GetHash())) {
		throw wxString(wxT("Wrong fileid sent (ProcessHashSet)"));
	}
	if (!m_fHashsetRequesting) {
		throw wxString(wxT("Received unsolicited hashset, ignoring it."));
	}
	CMemFile data(packet,size);
	if (m_reqfile->LoadHashsetFromFile(&data,true)) {
		m_fHashsetRequesting = 0;
	} else {
		m_reqfile->SetHashSetNeeded(true);
		throw wxString(wxT("Corrupted or invalid hashset received"));
	}
	SendStartupLoadReq();
}

void CUpDownClient::SendBlockRequests()
{
	uint32 current_time = ::GetTickCount();
	if (GetVBTTags()) {

		// Ask new blocks only when all completed
		if (!m_PendingBlocks_list.empty()) {
			return;
		}

		if ((m_dwLastBlockReceived + SEC2MS(5)) > current_time) {
			// We received last block in less than 5 secs? Let's request faster.
			m_MaxBlockRequests = m_MaxBlockRequests << 1;
			if ( m_MaxBlockRequests > 0x20) {
				m_MaxBlockRequests = 0x20;
			}
		} else {
			m_MaxBlockRequests = m_MaxBlockRequests >> 1;
			if ( m_MaxBlockRequests < STANDARD_BLOCKS_REQUEST) {
				m_MaxBlockRequests = STANDARD_BLOCKS_REQUEST;
			}
		}
	}

	m_dwLastBlockReceived = current_time;

	if (!m_reqfile) {
		return;
	}

	if (m_DownloadBlocks_list.empty()) {
		// Barry - instead of getting 3, just get how many is needed
		uint16 count = m_MaxBlockRequests - m_PendingBlocks_list.size();
		std::vector<Requested_Block_Struct*> toadd;
		if (m_reqfile->GetNextRequestedBlock(this, toadd, count)) {
			for (int i = 0; i != count; i++) {
				m_DownloadBlocks_list.push_back(toadd[i]);
			}
		}
	}

	// Barry - Why are unfinished blocks requested again, not just new ones?

	while (m_PendingBlocks_list.size() < m_MaxBlockRequests && !m_DownloadBlocks_list.empty()) {
		Pending_Block_Struct* pblock = new Pending_Block_Struct;
		pblock->block = m_DownloadBlocks_list.front();
		pblock->zStream = NULL;
		pblock->totalUnzipped = 0;
		pblock->fZStreamError = 0;
		pblock->fRecovered = 0;
		m_PendingBlocks_list.push_back(pblock);
		m_DownloadBlocks_list.pop_front();
	}


	if (m_PendingBlocks_list.empty()) {

		CUpDownClient* slower_client = NULL;

		if (thePrefs::GetDropSlowSources()) {
			slower_client = m_reqfile->GetSlowerDownloadingClient(m_lastaverage, this);
		}

		if (slower_client == NULL) {
			slower_client = this;
		}

		if (!slower_client->GetSentCancelTransfer()) {
			CPacket* packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
			theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
//			if (slower_client != this) {
//				printf("Dropped client %p to allow client %p to download\n",slower_client, this);
//			}
			slower_client->ClearDownloadBlockRequests();
			slower_client->SendPacket(packet,true,true);
			slower_client->SetSentCancelTransfer(1);
		}

		slower_client->SetDownloadState(DS_NONEEDEDPARTS);

		if (slower_client != this) {
			// Re-request freed blocks.
			AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_CANCELTRANSFER (faster source eager to transfer) to ") + slower_client->GetFullIP() );
			wxASSERT(m_DownloadBlocks_list.empty());
			wxASSERT(m_PendingBlocks_list.empty());
			uint16 count = m_MaxBlockRequests;
			std::vector<Requested_Block_Struct*> toadd;
			if (m_reqfile->GetNextRequestedBlock(this, toadd, count)) {
				for (int i = 0; i != count; i++) {
					Pending_Block_Struct* pblock = new Pending_Block_Struct;
					pblock->block = toadd[i];
					pblock->zStream = NULL;
					pblock->totalUnzipped = 0;
					pblock->fZStreamError = 0;
					pblock->fRecovered = 0;
					m_PendingBlocks_list.push_back(pblock);
				}
			} else {
				// WTF, we just freed blocks.
				wxFAIL_MSG(wxT("No free blocks to request after freeing some blocks"));
				return;
			}
		} else {
			// Drop this one.
			AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_CANCELTRANSFER (no free blocks) to ") + GetFullIP() );
			//#warning Kry - Would be nice to swap A4AF here.
			return;
		}
	}

	CPacket* packet = NULL;

	if (GetVBTTags()) {
		// ED2Kv2 packet...
		// Most common scenario: hash + blocks to request + every one
		// having 2 uint32 tags

		uint8 nBlocks = m_PendingBlocks_list.size();
		if (nBlocks > m_MaxBlockRequests) {
			nBlocks = m_MaxBlockRequests;
		}

		CMemFile data(16 + 1 + nBlocks*((2+4)*2));

		data.WriteHash(m_reqfile->GetFileHash());

		data.WriteUInt8(nBlocks);

		std::list<Pending_Block_Struct*>::iterator it = m_PendingBlocks_list.begin();
		while (nBlocks) {
			wxASSERT(it != m_PendingBlocks_list.end());
			wxASSERT( (*it)->block->StartOffset <= (*it)->block->EndOffset );
			(*it)->fZStreamError = 0;
			(*it)->fRecovered = 0;
			CTagVarInt(/*Noname*/0,(*it)->block->StartOffset).WriteTagToFile(&data);
			CTagVarInt(/*Noname*/0,(*it)->block->EndOffset).WriteTagToFile(&data);
			++it;
			nBlocks--;
		}

		packet = new CPacket(data, OP_ED2KV2HEADER, OP_REQUESTPARTS);
		AddDebugLogLineN( logLocalClient, CFormat(wxT("Local Client ED2Kv2: OP_REQUESTPARTS(%i) to %s"))
				  % (m_PendingBlocks_list.size()<m_MaxBlockRequests ? m_PendingBlocks_list.size() : m_MaxBlockRequests) % GetFullIP() );

	} else {
		wxASSERT(m_MaxBlockRequests == STANDARD_BLOCKS_REQUEST);

		//#warning Kry - I dont specially like this approach, we iterate one time too many

		bool bHasLongBlocks =  false;

		std::list<Pending_Block_Struct*>::iterator it = m_PendingBlocks_list.begin();
		for (uint32 i = 0; i != m_MaxBlockRequests; i++){
			if (it != m_PendingBlocks_list.end()) {
				Pending_Block_Struct* pending = *it++;
				wxASSERT( pending->block->StartOffset <= pending->block->EndOffset );
				if (pending->block->StartOffset > 0xFFFFFFFF || pending->block->EndOffset > 0xFFFFFFFF){
					bHasLongBlocks = true;
					if (!SupportsLargeFiles()){
						// Requesting a large block from a client that doesn't support large files?
						if (!GetSentCancelTransfer()){
							CPacket* cancel_packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
							theStats::AddUpOverheadFileRequest(cancel_packet->GetPacketSize());
							AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + GetFullIP() );
							SendPacket(cancel_packet,true,true);
							SetSentCancelTransfer(1);
						}
						SetDownloadState(DS_ERROR);
						return;
					}
					break;
				}
			}
		}

		CMemFile data(16 /*Hash*/ + (m_MaxBlockRequests*(bHasLongBlocks ? 8 : 4) /* uint32/64 start*/) + (3*(bHasLongBlocks ? 8 : 4)/* uint32/64 end*/));
		data.WriteHash(m_reqfile->GetFileHash());

		it = m_PendingBlocks_list.begin();
		for (uint32 i = 0; i != m_MaxBlockRequests; i++) {
			if (it != m_PendingBlocks_list.end()) {
				Pending_Block_Struct* pending = *it++;
				wxASSERT( pending->block->StartOffset <= pending->block->EndOffset );
				pending->fZStreamError = 0;
				pending->fRecovered = 0;
				if (bHasLongBlocks) {
					data.WriteUInt64(pending->block->StartOffset);
				} else {
					data.WriteUInt32(pending->block->StartOffset);
				}
			} else {
				if (bHasLongBlocks) {
					data.WriteUInt64(0);
				} else {
					data.WriteUInt32(0);
				}
			}
		}

		it = m_PendingBlocks_list.begin();
		for (uint32 i = 0; i != m_MaxBlockRequests; i++) {
			if (it != m_PendingBlocks_list.end()) {
				Requested_Block_Struct* block = (*it++)->block;
				if (bHasLongBlocks) {
					data.WriteUInt64(block->EndOffset+1);
				} else {
					data.WriteUInt32(block->EndOffset+1);
				}
			} else {
				if (bHasLongBlocks) {
					data.WriteUInt64(0);
				} else {
					data.WriteUInt32(0);
				}
			}
		}
		packet = new CPacket(data, (bHasLongBlocks ? OP_EMULEPROT : OP_EDONKEYPROT), (bHasLongBlocks ? (uint8)OP_REQUESTPARTS_I64 : (uint8)OP_REQUESTPARTS));
		AddDebugLogLineN(logLocalClient, CFormat(wxT("Local Client: %s to %s")) % (bHasLongBlocks ? wxT("OP_REQUESTPARTS_I64") : wxT("OP_REQUESTPARTS")) % GetFullIP());
	}

	if (packet) {
		theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
		SendPacket(packet, true, true);
	}
}

/*
Barry - Originally this only wrote to disk when a full 180k block
had been received from a client, and only asked for data in
180k blocks.

This meant that on average 90k was lost for every connection
to a client data source. That is a lot of wasted data.

To reduce the lost data, packets are now written to a buffer
and flushed to disk regularly regardless of size downloaded.

This includes compressed packets.

Data is also requested only where gaps are, not in 180k blocks.
The requests will still not exceed 180k, but may be smaller to
fill a gap.
*/

void CUpDownClient::ProcessBlockPacket(const byte* packet, uint32 size, bool packed, bool largeblocks)
{
	// Ignore if no data required
	if (!(GetDownloadState() == DS_DOWNLOADING || GetDownloadState() == DS_NONEEDEDPARTS)) {
		return;
	}

	// This vars are defined here to be able to use them on the catch
	int header_size = 16;
	uint64 nStartPos = 0;
	uint64 nEndPos = 0;
	uint32 nBlockSize = 0;
	uint32 lenUnzipped = 0;

	// Update stats
	m_dwLastBlockReceived = ::GetTickCount();

	try {

		// Read data from packet
		const CMemFile data(packet, size);

		// Check that this data is for the correct file
		if ((!m_reqfile) || data.ReadHash() != m_reqfile->GetFileHash()) {
			throw wxString(wxT("Wrong fileid sent (ProcessBlockPacket)"));
		}

		// Find the start & end positions, and size of this chunk of data

		if (largeblocks) {
			nStartPos = data.ReadUInt64();
			header_size += 8;
		} else {
			nStartPos = data.ReadUInt32();
			header_size += 4;
		}

		if (packed) {
			nBlockSize = data.ReadUInt32();
			header_size += 4;
			nEndPos = nStartPos + (size - header_size);
		} else {
			if (largeblocks) {
				nEndPos = data.ReadUInt64();
				header_size += 8;
			} else {
				nEndPos = data.ReadUInt32();
				header_size += 4;
			}
		}

		// Check that packet size matches the declared data size + header size
		if ( nEndPos == nStartPos || size != ((nEndPos - nStartPos) + header_size)) {
			throw wxString(wxT("Corrupted or invalid DataBlock received (ProcessBlockPacket)"));
		}
		theStats::AddDownloadFromSoft(GetClientSoft(),size - header_size);
		bytesReceivedCycle += size - header_size;

		credits->AddDownloaded(size - header_size, GetIP(), theApp->CryptoAvailable());

		// Move end back one, should be inclusive
		nEndPos--;

		// Loop through to find the reserved block that this is within
		std::list<Pending_Block_Struct*>::iterator it = m_PendingBlocks_list.begin();
		for (; it != m_PendingBlocks_list.end(); ++it) {
			Pending_Block_Struct* cur_block = *it;

			if ((cur_block->block->StartOffset <= nStartPos) && (cur_block->block->EndOffset >= nStartPos)) {
				// Found reserved block

				if (cur_block->block->StartOffset == nStartPos) {
					// This block just started transfering. Set the start time.
					m_last_block_start = ::GetTickCountFullRes();
				}

				if (cur_block->fZStreamError){
					AddDebugLogLineN(logZLib,
						CFormat(wxT("Ignoring %u bytes of block %u-%u because of erroneous zstream state for file: %s"))
							% (size - header_size) % nStartPos % nEndPos % m_reqfile->GetFileName());
					m_reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
					return;
				}

				// Remember this start pos, used to draw part downloading in list
				m_lastDownloadingPart = nStartPos / PARTSIZE;

				// Occasionally packets are duplicated, no point writing it twice
				// This will be 0 in these cases, or the length written otherwise
				uint32 lenWritten = 0;

				// Handle differently depending on whether packed or not
				if (!packed) {
					// security sanitize check
					if (nEndPos > cur_block->block->EndOffset) {
						AddDebugLogLineN(logRemoteClient, CFormat(wxT("Received Blockpacket exceeds requested boundaries (requested end: %u, Part: %u, received end: %u, Part: %u), file: %s remote IP: %s")) % cur_block->block->EndOffset % (uint32)(cur_block->block->EndOffset / PARTSIZE) % nEndPos % (uint32)(nEndPos / PARTSIZE) % m_reqfile->GetFileName() % Uint32toStringIP(GetIP()));
						m_reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
						return;
					}
					// Write to disk (will be buffered in part file class)
					lenWritten = m_reqfile->WriteToBuffer( size - header_size, (byte*)(packet + header_size), nStartPos, nEndPos, cur_block->block, this);
				} else {
					// Packed
					wxASSERT( (long int)size > 0 );
					// Create space to store unzipped data, the size is
					// only an initial guess, will be resized in unzip()
					// if not big enough
					lenUnzipped = (size * 2);
					// Don't get too big
					if (lenUnzipped > (BLOCKSIZE + 300)) {
						lenUnzipped = (BLOCKSIZE + 300);
					}
					byte *unzipped = new byte[lenUnzipped];

					// Try to unzip the packet
					int result = unzip(cur_block, (byte*)(packet + header_size), (size - header_size), &unzipped, &lenUnzipped);

					// no block can be uncompressed to >2GB, 'lenUnzipped' is obviously erroneous.
					if (result == Z_OK && ((int)lenUnzipped >= 0)) {

						// Write any unzipped data to disk
						if (lenUnzipped > 0) {
							wxASSERT( (int)lenUnzipped > 0 );

							// Use the current start and end positions for the uncompressed data
							nStartPos = cur_block->block->StartOffset + cur_block->totalUnzipped - lenUnzipped;
							nEndPos = cur_block->block->StartOffset + cur_block->totalUnzipped - 1;

							if (nStartPos > cur_block->block->EndOffset || nEndPos > cur_block->block->EndOffset) {
								AddDebugLogLineN(logZLib,
									CFormat(wxT("Corrupted compressed packet for '%s' received (error 666)")) % m_reqfile->GetFileName());
								m_reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
							} else {
								// Write uncompressed data to file
								lenWritten = m_reqfile->WriteToBuffer( size - header_size,
																	   unzipped,
																	   nStartPos,
																	   nEndPos,
																	   cur_block->block,
																	   this);
							}
						}
					} else {
						wxString strZipError;
						if (cur_block->zStream && cur_block->zStream->msg) {
							strZipError = wxT(" - ") + wxString::FromAscii(cur_block->zStream->msg);
						}

						AddDebugLogLineN(logZLib,
							CFormat(wxT("Corrupted compressed packet for '%s' received (error %i): %s"))
								% m_reqfile->GetFileName() % result % strZipError);

						m_reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);

						// If we had an zstream error, there is no chance that we could recover from it nor that we
						// could use the current zstream (which is in error state) any longer.
						if (cur_block->zStream){
							inflateEnd(cur_block->zStream);
							delete cur_block->zStream;
							cur_block->zStream = NULL;
						}

						// Although we can't further use the current zstream, there is no need to disconnect the sending
						// client because the next zstream (a series of 10K-blocks which build a 180K-block) could be
						// valid again. Just ignore all further blocks for the current zstream.
						cur_block->fZStreamError = 1;
						cur_block->totalUnzipped = 0; // bluecow's fix
					}
					delete [] unzipped;
				}
				// These checks only need to be done if any data was written
				if (lenWritten > 0) {
					m_nTransferredDown += lenWritten;

					// If finished reserved block
					if (nEndPos == cur_block->block->EndOffset) {

						// Save last average speed based on data and time.
						// This should do bytes/sec.
						uint32 average_time = (::GetTickCountFullRes() - m_last_block_start);

						// Avoid divide by 0.
						if (average_time == 0) {
							average_time++;
						}

						m_lastaverage = ((cur_block->block->EndOffset - cur_block->block->StartOffset) * 1000) / average_time;

						m_reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
						delete cur_block->block;
						// Not always allocated
						if (cur_block->zStream) {
							inflateEnd(cur_block->zStream);
							delete cur_block->zStream;
						}
						delete cur_block;
						m_PendingBlocks_list.erase(it);

						// Request next block
						SendBlockRequests();
					}
				}
				// Stop looping and exit method
				return;
			}
		}
	} catch (const CEOFException& e) {
		wxString error = wxString(wxT("Error reading "));
		if (packed) error += CFormat(wxT("packed (LU: %i) largeblocks ")) % lenUnzipped;
		error += CFormat(wxT("data packet: RS: %i HS: %i SP: %i EP: %i BS: %i -> "))
					% size % header_size % nStartPos % nEndPos % nBlockSize;
		AddDebugLogLineC(logRemoteClient, error + e.what());
		return;
	}
}

int CUpDownClient::unzip(Pending_Block_Struct *block, byte *zipped, uint32 lenZipped, byte **unzipped, uint32 *lenUnzipped, int iRecursion)
{
	int err = Z_DATA_ERROR;

	// Save some typing
	z_stream *zS = block->zStream;

	// Is this the first time this block has been unzipped
	if (zS == NULL) {
		// Create stream
		block->zStream = new z_stream;
		zS = block->zStream;

		// Initialise stream values
		zS->zalloc = (alloc_func)0;
		zS->zfree = (free_func)0;
		zS->opaque = (voidpf)0;

		// Set output data streams, do this here to avoid overwriting on recursive calls
		zS->next_out = (*unzipped);
		zS->avail_out = (*lenUnzipped);

		// Initialise the z_stream
		err = inflateInit(zS);
		if (err != Z_OK) {
			return err;
		}
	}

	// Use whatever input is provided
	zS->next_in  = zipped;
	zS->avail_in = lenZipped;

	// Only set the output if not being called recursively
	if (iRecursion == 0) {
		zS->next_out = (*unzipped);
		zS->avail_out = (*lenUnzipped);
	}

	// Try to unzip the data
	err = inflate(zS, Z_SYNC_FLUSH);

	// Is zip finished reading all currently available input and writing
	// all generated output
	if (err == Z_STREAM_END) {
		// Finish up
		err = inflateEnd(zS);
		if (err != Z_OK) {
			return err;
		}

		// Got a good result, set the size to the amount unzipped in this call
		//  (including all recursive calls)
		(*lenUnzipped) = (zS->total_out - block->totalUnzipped);
		block->totalUnzipped = zS->total_out;
	} else if ((err == Z_OK) && (zS->avail_out == 0) && (zS->avail_in != 0)) {

		// Output array was not big enough,
		// call recursively until there is enough space

		// What size should we try next
		uint32 newLength = (*lenUnzipped) *= 2;
		if (newLength == 0) {
			newLength = lenZipped * 2;
		}
		// Copy any data that was successfully unzipped to new array
		byte *temp = new byte[newLength];
		wxASSERT( zS->total_out - block->totalUnzipped <= newLength );
		memcpy(temp, (*unzipped), (zS->total_out - block->totalUnzipped));
		delete [] (*unzipped);
		(*unzipped) = temp;
		(*lenUnzipped) = newLength;

		// Position stream output to correct place in new array
		zS->next_out = (*unzipped) + (zS->total_out - block->totalUnzipped);
		zS->avail_out = (*lenUnzipped) - (zS->total_out - block->totalUnzipped);

		// Try again
		err = unzip(block, zS->next_in, zS->avail_in, unzipped, lenUnzipped, iRecursion + 1);
	} else if ((err == Z_OK) && (zS->avail_in == 0)) {
		// All available input has been processed, everything ok.
		// Set the size to the amount unzipped in this call
		// (including all recursive calls)
		(*lenUnzipped) = (zS->total_out - block->totalUnzipped);
		block->totalUnzipped = zS->total_out;
	} else {
		// Should not get here unless input data is corrupt
		wxString strZipError;

		if ( zS->msg ) {
			strZipError = CFormat(wxT(" %d '%s'")) % err % wxString::FromAscii(zS->msg);
		} else if (err != Z_OK) {
			strZipError = CFormat(wxT(" %d")) % err;
		}

		AddDebugLogLineN(logZLib,
			CFormat(wxT("Unexpected zip error %s in file '%s'"))
				% strZipError % (m_reqfile ? m_reqfile->GetFileName() : CPath(wxT("?"))));
	}

	if (err != Z_OK) {
		(*lenUnzipped) = 0;
	}

	return err;
}


// Speed is now updated only when data was received, calculated as
// (data received) / (time since last receiption)
// and slightly filtered (10s average).
// Result is quite precise now and makes the DownloadRateAdjust workaround obsolete.

float CUpDownClient::CalculateKBpsDown()
{
	const	float tAverage = 10.0;
	uint32	msCur = GetTickCount();

	if (bytesReceivedCycle) {
		float dt = (msCur - msReceivedPrev) / 1000.0; // time since last reception
		if (dt < 0.01) {	// (safeguard against divide-by-zero)
			dt = 0.01f;		//  diff should be 100ms actually
		}
		float kBpsDownCur = bytesReceivedCycle / 1024.0 / dt;
		if (dt >= tAverage) {
			kBpsDown = kBpsDownCur;
		} else {
			kBpsDown = (kBpsDown * (tAverage - dt) + kBpsDownCur * dt) / tAverage;
		}
		//AddDebugLogLineN(logLocalClient, CFormat(wxT("CalculateKBpsDown %p kbps %.1f kbpsCur %.1f dt %.3f rcv %d "))
		//			% this % kBpsDown  % kBpsDownCur % dt % bytesReceivedCycle);
		bytesReceivedCycle = 0;
		msReceivedPrev = msCur;
	}

	m_cShowDR++;
	if (m_cShowDR == 30){
		m_cShowDR = 0;
		UpdateDisplayedInfo();
	}
	if (msCur - m_dwLastBlockReceived > DOWNLOADTIMEOUT) {
		if (!GetSentCancelTransfer()){
			CPacket* packet = new CPacket(OP_CANCELTRANSFER, 0, OP_EDONKEYPROT);
			theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
			AddDebugLogLineN( logLocalClient, wxT("Local Client: OP_CANCELTRANSFER to ") + GetFullIP() );
			SendPacket(packet,true,true);
			SetSentCancelTransfer(1);
		}
		SetDownloadState(DS_ONQUEUE);
	}

	return kBpsDown;
}

uint16 CUpDownClient::GetAvailablePartCount() const
{
	uint16 result = 0;
	for (int i = 0;i != m_nPartCount;i++){
		if (IsPartAvailable(i))
			result++;
	}
	return result;
}

void CUpDownClient::SetRemoteQueueRank(uint16 nr)
{
	m_nOldRemoteQueueRank = m_nRemoteQueueRank;
	m_nRemoteQueueRank = nr;
	UpdateDisplayedInfo();
}

void CUpDownClient::UDPReaskACK(uint16 nNewQR)
{
	// 0.42e
	m_bUDPPending = false;
	SetRemoteQueueRank(nNewQR);
	m_dwLastAskedTime = ::GetTickCount();
}

void CUpDownClient::UDPReaskFNF()
{
	m_bUDPPending = false;

	// avoid premature deletion of 'this' client
	if (GetDownloadState() != DS_DOWNLOADING){
		if (m_reqfile) {
			m_reqfile->AddDeadSource(this);
		}

		theApp->downloadqueue->RemoveSource(this);
		if (!m_socket) {
			if (Disconnected(wxT("UDPReaskFNF m_socket=NULL"))) {
				Safe_Delete();
			}
		}
	} else {
		AddDebugLogLineN( logRemoteClient, wxT("UDP ANSWER FNF : ") + GetUserName() + wxT(" - did not remove client because of current download state") );
	}
}

void CUpDownClient::UDPReaskForDownload()
{

	wxASSERT(m_reqfile);

	if(!m_reqfile || m_bUDPPending ) {
		return;
	}

	//#warning We should implement the quality tests for udp reliability
	/*
	if( m_nTotalUDPPackets > 3 && ((float)(m_nFailedUDPPackets/m_nTotalUDPPackets) > .3)) {
		return;
	}
	*/

	if (thePrefs::GetEffectiveUDPPort() == 0) {
		return;
	}

	if (m_nUDPPort != 0 && !theApp->IsFirewalled() && !IsConnected()) {
		//don't use udp to ask for sources
		if(IsSourceRequestAllowed()) {
			return;
		}

		m_bUDPPending = true;

		CMemFile data(128);
		data.WriteHash(m_reqfile->GetFileHash());

		if (GetUDPVersion() > 3) {
			if (m_reqfile->IsPartFile()) {
				static_cast<CPartFile*>(m_reqfile)->WritePartStatus(&data);
			}
			else {
				data.WriteUInt16(0);
			}
		}

		if (GetUDPVersion() > 2) {
			data.WriteUInt16(m_reqfile->m_nCompleteSourcesCount);
		}

		CPacket* response = new CPacket(data, OP_EMULEPROT, OP_REASKFILEPING);
		AddDebugLogLineN( logClientUDP, wxT("Client UDP socket: send OP_REASKFILEPING") );
		theStats::AddUpOverheadFileRequest(response->GetPacketSize());
		theApp->clientudp->SendPacket(response,GetConnectIP(),GetUDPPort(), ShouldReceiveCryptUDPPackets(), GetUserHash().GetHash(), false, 0);
	} else  if (HasLowID() && GetBuddyIP() && GetBuddyPort() && HasValidBuddyID()) {

		m_bUDPPending = true;

		CMemFile data(128);

		data.WriteHash(CMD4Hash(GetBuddyID()));
		data.WriteHash(m_reqfile->GetFileHash());

		if (GetUDPVersion() > 3) {
			if (m_reqfile->IsPartFile()) {
				static_cast<CPartFile*>(m_reqfile)->WritePartStatus(&data);
			} else {
				data.WriteUInt16(0);
			}
		}

		if (GetUDPVersion() > 2) {
			data.WriteUInt16(m_reqfile->m_nCompleteSourcesCount);
		}

		CPacket* response = new CPacket(data, OP_EMULEPROT, OP_REASKCALLBACKUDP);
		AddDebugLogLineN( logClientUDP, wxT("Client UDP socket: send OP_REASKCALLBACKUDP") );
		theStats::AddUpOverheadFileRequest(response->GetPacketSize());
		theApp->clientudp->SendPacket(response, GetBuddyIP(), GetBuddyPort(), false, NULL, true, 0 );
	}
}


// Get the next part that is requested
uint16 CUpDownClient::GetNextRequestedPart() const
{
	uint16 part = 0xffff;

	std::list<Pending_Block_Struct*>::const_iterator it = m_PendingBlocks_list.begin();
	for (; it != m_PendingBlocks_list.end(); ++it) {
		part = (*it)->block->StartOffset / PARTSIZE;
		if (part != m_lastDownloadingPart) {
			break;
		}
	}

	return part;
}


void CUpDownClient::UpdateDisplayedInfo(bool force)
{
	uint32 curTick = ::GetTickCount();
	if (force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE) {
		// Check if we actually need to notify of changes
		bool update = m_reqfile && m_reqfile->ShowSources();

		// Check A4AF files only if needed
		if ( !update ) {
			A4AFList::iterator it = m_A4AF_list.begin();
			for ( ; it != m_A4AF_list.end(); ++it ) {
				if ( it->first->ShowSources() ) {
					update = true;
					break;
				}
			}
		}

		// And finnaly trigger an event if there's any reason
		if ( update ) {
			SourceItemType type;
			switch (GetDownloadState()) {
				case DS_DOWNLOADING:
				case DS_ONQUEUE:
					// We will send A4AF, which will be checked.
					type = A4AF_SOURCE;
					break;
				default:
					type = UNAVAILABLE_SOURCE;
					break;
			}

			Notify_SourceCtrlUpdateSource(ECID(), type );
		}

		// Shared files view
		if (m_uploadingfile && m_uploadingfile->ShowPeers()) {
			Notify_SharedCtrlRefreshClient(ECID(), AVAILABLE_SOURCE);
		}

		m_lastRefreshedDLDisplay = curTick;
	}
}

uint8 CUpDownClient::GetObfuscationStatus() const
{
	uint8 ret = OBST_UNDEFINED;
	if (thePrefs::IsClientCryptLayerSupported()) {
		if (SupportsCryptLayer()) {
			if ((RequestsCryptLayer() || thePrefs::IsClientCryptLayerRequested()) && HasObfuscatedConnectionBeenEstablished()) {
				ret = OBST_ENABLED;
			} else {
				ret = OBST_SUPPORTED;
			}
		} else {
			ret = OBST_NOT_SUPPORTED;
		}
	} else {
		ret = OBST_DISABLED;
	}
	return ret;
}

// IgnoreNoNeeded = will switch to files of which this source has no needed parts (if no better fiels found)
// ignoreSuspensions = ignore timelimit for A4Af jumping
// bRemoveCompletely = do not readd the file which the source is swapped from to the A4AF lists (needed if deleting or stopping a file)
// toFile = Try to swap to this partfile only

bool CUpDownClient::SwapToAnotherFile(bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile)
{
	// Fail if m_reqfile is invalid
	if ( m_reqfile == NULL ) {
		return false;
	}

	// It would be stupid to swap away a downloading source
	if (GetDownloadState() == DS_DOWNLOADING) {
		return false;
	}

	// The iterator of the final target
	A4AFList::iterator target = m_A4AF_list.end();

	// Do we want to swap to a specific file?
	if ( toFile != NULL ) {
		A4AFList::iterator it = m_A4AF_list.find( toFile );
		if ( it != m_A4AF_list.end() ) {

			// We force ignoring of timestamps
			if ( IsValidSwapTarget( it, bIgnoreNoNeeded, true ) ) {
				// Set the target
				target = it;
			}
		}
	} else {
		// We want highest priority possible, but need to start with
		// a value less than any other priority
		char priority = -1;

		A4AFList::iterator it = m_A4AF_list.begin();
		for ( ; it != m_A4AF_list.end(); ++it ) {
			if ( IsValidSwapTarget( it, bIgnoreNoNeeded, ignoreSuspensions ) ) {
				char cur_priority = it->first->GetDownPriority();

				// We would prefer to get files with needed parts, thus rate them higher.
				// However, this really only matters if bIgnoreNoNeeded is true.
				if ( it->second.NeededParts )
						cur_priority += 10;

				// Change target if the current file has a higher rate than the previous
				if ( cur_priority > priority ) {
					priority = cur_priority;

					// Set the new target
					target = it;

					// Break on the first High-priority file with needed parts
					if ( priority == PR_HIGH + 10 ) {
						break;
					}
				}
			}
		}
	}

	// Try to swap if we found a valid target
	if ( target != m_A4AF_list.end() ) {

		// Sanity check, if reqfile doesn't own the source, then something
		// is wrong and the swap cannot proceed.
		if ( m_reqfile->DelSource( this ) ) {
			CPartFile* SwapTo = target->first;

			// remove this client from the A4AF list of our new m_reqfile
			if ( SwapTo->RemoveA4AFSource( this ) ) {
				Notify_SourceCtrlRemoveSource(ECID(), SwapTo);
			}

			m_reqfile->RemoveDownloadingSource( this );

			// Do we want to remove it completly? Say if the old file is getting deleted
			if ( !bRemoveCompletely ) {
				m_reqfile->AddA4AFSource( this );

				// Set the status of the old file
				m_A4AF_list[m_reqfile].NeededParts = (GetDownloadState() != DS_NONEEDEDPARTS);

				// Avoid swapping to this file for a while
				m_A4AF_list[m_reqfile].timestamp = ::GetTickCount();

				Notify_SourceCtrlAddSource(m_reqfile, CCLIENTREF(this, wxT("CUpDownClient::SwapToAnotherFile Notify_SourceCtrlAddSource 1")), A4AF_SOURCE);
			} else {
				Notify_SourceCtrlRemoveSource(ECID(), m_reqfile);
			}

			SetDownloadState(DS_NONE);
			ResetFileStatusInfo();

			m_nRemoteQueueRank = 0;
			m_nOldRemoteQueueRank = 0;

			m_reqfile->UpdatePartsInfo();

			SetRequestFile( SwapTo );

			SwapTo->AddSource( this );

			Notify_SourceCtrlAddSource(SwapTo, CCLIENTREF(this, wxT("CUpDownClient::SwapToAnotherFile Notify_SourceCtrlAddSource 2")), UNAVAILABLE_SOURCE);

			// Remove the new reqfile from the list of other files
			m_A4AF_list.erase( target );

			return true;
		}
	}

	return false;
}


bool CUpDownClient::IsValidSwapTarget( A4AFList::iterator it, bool ignorenoneeded, bool ignoresuspended )
{
	wxASSERT( it != m_A4AF_list.end() && it->first );

	// Check if this file has been suspended
	if ( !ignoresuspended ) {
		if ( ::GetTickCount() - it->second.timestamp >= PURGESOURCESWAPSTOP ) {
			// The wait-time has been exceeded and the file is now a valid target
			it->second.timestamp = 0;
		} else {
			// The file was still suspended and we are not ignoring suspensions
			return false;
		}
	}

	// Check if the client has needed parts
	if ( !ignorenoneeded ) {
		if ( !it->second.NeededParts ) {
			return false;
		}
	}

	// Final checks to see if the client is a valid target
	CPartFile* cur_file = it->first;
	if ( ( cur_file != m_reqfile && !cur_file->IsStopped() ) &&
	     ( cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY ) &&
		 ( cur_file->IsPartFile() ) )
	{
		return true;
	} else {
		return false;
	}
}


void CUpDownClient::SetRequestFile(CPartFile* reqfile)
{
	if ( m_reqfile != reqfile ) {
		// Decrement the source-count of the old request-file
		if ( m_reqfile ) {
			m_reqfile->ClientStateChanged( GetDownloadState(), -1 );
			m_reqfile->UpdatePartsFrequency( this, false );
		}

		m_nPartCount = 0;
		m_downPartStatus.clear();

		m_reqfile = reqfile;

		if ( reqfile ) {
			// Increment the source-count of the new request-file
			m_reqfile->ClientStateChanged( -1, GetDownloadState() );

			m_nPartCount = reqfile->GetPartCount();
		}
	}
}

void CUpDownClient::SetReqFileAICHHash(CAICHHash* val){
	if(m_pReqFileAICHHash != NULL && m_pReqFileAICHHash != val)
		delete m_pReqFileAICHHash;
	m_pReqFileAICHHash = val;
}

void CUpDownClient::SendAICHRequest(CPartFile* pForFile, uint16 nPart){
	CAICHRequestedData request;
	request.m_nPart = nPart;
	request.m_pClient.Link(this CLIENT_DEBUGSTRING("CUpDownClient::SendAICHRequest"));
	request.m_pPartFile = pForFile;
	CAICHHashSet::m_liRequestedData.push_back(request);
	m_fAICHRequested = TRUE;
	CMemFile data;
	data.WriteHash(pForFile->GetFileHash());
	data.WriteUInt16(nPart);
	pForFile->GetAICHHashset()->GetMasterHash().Write(&data);
	CPacket* packet = new CPacket(data, OP_EMULEPROT, OP_AICHREQUEST);
	theStats::AddUpOverheadOther(packet->GetPacketSize());
	AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_AICHREQUEST to") + GetFullIP());
	SafeSendPacket(packet);
}

void CUpDownClient::ProcessAICHAnswer(const byte* packet, uint32 size)
{
	if (m_fAICHRequested == FALSE){
		throw wxString(wxT("Received unrequested AICH Packet"));
	}
	m_fAICHRequested = FALSE;

	CMemFile data(packet, size);
	if (size <= 16){
		CAICHHashSet::ClientAICHRequestFailed(this);
		return;
	}

	CMD4Hash hash = data.ReadHash();
	CPartFile* pPartFile = theApp->downloadqueue->GetFileByID(hash);
	CAICHRequestedData request = CAICHHashSet::GetAICHReqDetails(this);
	uint16 nPart = data.ReadUInt16();
	if (pPartFile != NULL && request.m_pPartFile == pPartFile && request.m_pClient.GetClient() == this && nPart == request.m_nPart){
		CAICHHash ahMasterHash(&data);
		if ( (pPartFile->GetAICHHashset()->GetStatus() == AICH_TRUSTED || pPartFile->GetAICHHashset()->GetStatus() == AICH_VERIFIED)
			 && ahMasterHash == pPartFile->GetAICHHashset()->GetMasterHash())
		{
			if(pPartFile->GetAICHHashset()->ReadRecoveryData(request.m_nPart*PARTSIZE, &data)){
				// finally all checks passed, everythings seem to be fine
				AddDebugLogLineN(logAICHTransfer, wxT("AICH Packet Answer: Succeeded to read and validate received recoverydata"));
				CAICHHashSet::RemoveClientAICHRequest(this);
				pPartFile->AICHRecoveryDataAvailable(request.m_nPart);
				return;
			} else {
				AddDebugLogLineN(logAICHTransfer, wxT("AICH Packet Answer: Succeeded to read and validate received recoverydata"));
			}
		} else {
			AddDebugLogLineN( logAICHTransfer, wxT("AICH Packet Answer: Masterhash differs from packethash or hashset has no trusted Masterhash") );
		}
	} else {
		AddDebugLogLineN( logAICHTransfer, wxT("AICH Packet Answer: requested values differ from values in packet") );
	}

	CAICHHashSet::ClientAICHRequestFailed(this);
}


void CUpDownClient::ProcessAICHRequest(const byte* packet, uint32 size)
{
	if (size != 16 + 2 + CAICHHash::GetHashSize()) {
		throw wxString(wxT("Received AICH Request Packet with wrong size"));
	}

	CMemFile data(packet, size);

	CMD4Hash hash = data.ReadHash();
	uint16 nPart = data.ReadUInt16();
	CAICHHash ahMasterHash(&data);
	CKnownFile* pKnownFile = theApp->sharedfiles->GetFileByID(hash);
	if (pKnownFile != NULL){
		if (pKnownFile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE && pKnownFile->GetAICHHashset()->HasValidMasterHash()
			&& pKnownFile->GetAICHHashset()->GetMasterHash() == ahMasterHash && pKnownFile->GetPartCount() > nPart
			&& pKnownFile->GetFileSize() > EMBLOCKSIZE && pKnownFile->GetFileSize() - PARTSIZE*nPart > EMBLOCKSIZE)
		{
			CMemFile fileResponse;
			fileResponse.WriteHash(pKnownFile->GetFileHash());
			fileResponse.WriteUInt16(nPart);
			pKnownFile->GetAICHHashset()->GetMasterHash().Write(&fileResponse);
			if (pKnownFile->GetAICHHashset()->CreatePartRecoveryData(nPart*PARTSIZE, &fileResponse)){
				AddDebugLogLineN(logAICHTransfer,
					CFormat(wxT("AICH Packet Request: Sucessfully created and send recoverydata for '%s' to %s"))
						% pKnownFile->GetFileName() % GetClientFullInfo());

				CPacket* packAnswer = new CPacket(fileResponse, OP_EMULEPROT, OP_AICHANSWER);
				theStats::AddUpOverheadOther(packAnswer->GetPacketSize());
				AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_AICHANSWER to") + GetFullIP());
				SafeSendPacket(packAnswer);
				return;
			} else {
				AddDebugLogLineN(logAICHTransfer,
					CFormat(wxT("AICH Packet Request: Failed to create recoverydata for '%s' to %s"))
						% pKnownFile->GetFileName() % GetClientFullInfo());
			}
		} else {
			AddDebugLogLineN(logAICHTransfer,
				CFormat(wxT("AICH Packet Request: Failed to create recoverydata - Hashset not ready or requested Hash differs from Masterhash for '%s' to %s"))
					% pKnownFile->GetFileName() % GetClientFullInfo());
		}
	} else {
		AddDebugLogLineN( logAICHTransfer, wxT("AICH Packet Request: Failed to find requested shared file - ") + GetClientFullInfo() );
	}

	CPacket* packAnswer = new CPacket(OP_AICHANSWER, 16, OP_EMULEPROT);
	packAnswer->Copy16ToDataBuffer(hash.GetHash());
	theStats::AddUpOverheadOther(packAnswer->GetPacketSize());
	AddDebugLogLineN(logLocalClient, wxT("Local Client: OP_AICHANSWER to") + GetFullIP());
	SafeSendPacket(packAnswer);
}

void CUpDownClient::ProcessAICHFileHash(CMemFile* data, const CPartFile* file){
	CPartFile* pPartFile;
	if (file == NULL){
		pPartFile = theApp->downloadqueue->GetFileByID(data->ReadHash());
	} else {
		pPartFile = const_cast<CPartFile*>(file);
	}
	CAICHHash ahMasterHash(data);

	if(pPartFile != NULL && pPartFile == GetRequestFile()){
		SetReqFileAICHHash(new CAICHHash(ahMasterHash));
		pPartFile->GetAICHHashset()->UntrustedHashReceived(ahMasterHash, GetConnectIP());
	} else {
		AddDebugLogLineN( logAICHTransfer, wxT("ProcessAICHFileHash(): PartFile not found or Partfile differs from requested file, ") + GetClientFullInfo() );
	}
}
// File_checked_for_headers
