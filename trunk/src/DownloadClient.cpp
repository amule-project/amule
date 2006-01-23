//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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


#include <zlib.h>
#include <cmath>		// Needed for std:exp

#include "ClientCredits.h"	// Needed for CClientCredits
#include "OtherFunctions.h"	// Needed for md4cmp
#include "ClientUDPSocket.h"	// Needed for CClientUDPSocket
#include "ServerConnect.h"	// Needed for CServerConnect
#include "DownloadQueue.h"	// Needed for CDownloadQueue
#include "OtherStructs.h"	// Needed for Requested_Block_Struct
#include "Preferences.h"	// Needed for thePrefs
#include "UploadQueue.h"	// Needed for CUploadQueue
#include "Packet.h"		// Needed for CPacket
#include "MemFile.h"		// Needed for CMemFile
#include "ClientTCPSocket.h"// Needed for CClientTCPSocket
#include "ListenSocket.h"	// Needed for CListenSocket
#include "amule.h"		// Needed for theApp
#include "PartFile.h"		// Needed for CPartFile
#include "updownclient.h"	// Needed for CUpDownClient
#include "SHAHashSet.h"
#include "SharedFileList.h"
#include "Statistics.h"		// Needed for theStats
#include "Logger.h"



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

bool CUpDownClient::AskForDownload()
{
	// 0.42e
	if (theApp.listensocket->TooManySockets()) {
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
	CPacket* packet = new CPacket(&dataStartupLoadReq);
	packet->SetOpCode(OP_STARTUPLOADREQ);
	theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
	SendPacket(packet, true, true);
}


bool CUpDownClient::IsSourceRequestAllowed()
{
	// 0.42e
	uint32 dwTickCount = ::GetTickCount() + CONNECTION_LATENCY;
	uint32 nTimePassedClient = dwTickCount - GetLastSrcAnswerTime();
	uint32 nTimePassedFile   = dwTickCount - m_reqfile->GetLastAnsweredTime();
	bool bNeverAskedBefore = (GetLastAskedForSources() == 0);

	uint32 uSources = m_reqfile->GetSourceCount();
	return (
		// if client has the correct extended protocol
		ExtProtocolAvailable() && m_bySourceExchangeVer >= 1 &&
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
	// 0.42e
	wxCHECK_RET(m_reqfile, wxT("Cannot request file when no reqfile is set"));	
	
	CMemFile dataFileReq(16+16);
	dataFileReq.WriteHash(m_reqfile->GetFileHash());
	if (SupportMultiPacket()) {
		
		if (SupportExtMultiPacket()) {
			dataFileReq.WriteUInt64(m_reqfile->GetFileSize());
		}
		
		AddDebugLogLineM(false, logClient, wxT("Sending file request to client\n"));
		
		dataFileReq.WriteUInt8(OP_REQUESTFILENAME);
		// Extended information
		if (GetExtendedRequestsVersion() > 0) {
			m_reqfile->WritePartStatus(&dataFileReq);
		}
		if (GetExtendedRequestsVersion() > 1) {
			m_reqfile->WriteCompleteSourcesCount(&dataFileReq);
		}
		if (m_reqfile->GetPartCount() > 1) {
			dataFileReq.WriteUInt8(OP_SETREQFILEID);
		}
		if (IsEmuleClient()) {
			SetRemoteQueueFull( true );
			SetRemoteQueueRank(0);
		}
		if (IsSourceRequestAllowed()) {
			dataFileReq.WriteUInt8(OP_REQUESTSOURCES);
			m_reqfile->SetLastAnsweredTimeTimeout();
			SetLastAskedForSources();
		}
		if (IsSupportingAICH()) {
			dataFileReq.WriteUInt8(OP_AICHFILEHASHREQ);
		}		
		CPacket* packet = new CPacket(&dataFileReq, OP_EMULEPROT, (SupportExtMultiPacket() ? OP_MULTIPACKET_EXT : OP_MULTIPACKET));
		theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
		SendPacket(packet, true);
	} else {
		//This is extended information
		if (GetExtendedRequestsVersion() > 0 ) {
			m_reqfile->WritePartStatus(&dataFileReq);
		}
		if (GetExtendedRequestsVersion() > 1 ) {
			m_reqfile->WriteCompleteSourcesCount(&dataFileReq);
		}
		CPacket* packet = new CPacket(&dataFileReq);
		packet->SetOpCode(OP_REQUESTFILENAME);
		theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
		SendPacket(packet, true);
	
		// 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
		// if the remote client answers the OP_REQUESTFILENAME with OP_REQFILENAMEANSWER the file is shared by the remote client. if we
		// know that the file is shared, we know also that the file is complete and don't need to request the file status.
		
		// Sending the packet could have deleted the client, check m_reqfile		
		if (m_reqfile && (m_reqfile->GetPartCount() > 1)) {
			CMemFile dataSetReqFileID(16);
			dataSetReqFileID.WriteHash(m_reqfile->GetFileHash());
			packet = new CPacket(&dataSetReqFileID);
			packet->SetOpCode(OP_SETREQFILEID);
			theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
			SendPacket(packet, true);
		}
	
		if (IsEmuleClient()) {
			SetRemoteQueueFull( true );
			SetRemoteQueueRank(0);
		}	
		
		// Sending the packet could have deleted the client, check m_reqfile		
		if (m_reqfile && IsSourceRequestAllowed()) {
			m_reqfile->SetLastAnsweredTimeTimeout();
			packet = new CPacket(OP_REQUESTSOURCES,16,OP_EMULEPROT);
			packet->Copy16ToDataBuffer((const char *)m_reqfile->GetFileHash().GetHash());
			theStats::AddUpOverheadSourceExchange(packet->GetPacketSize());
			SendPacket(packet,true,true);
			SetLastAskedForSources();
		}
		
		// Sending the packet could have deleted the client, check m_reqfile		
		if (m_reqfile && IsSupportingAICH()) {
			packet = new CPacket(OP_AICHFILEHASHREQ,16,OP_EMULEPROT);
			packet->Copy16ToDataBuffer((const char *)m_reqfile->GetFileHash().GetHash());
			theStats::AddUpOverheadOther(packet->GetPacketSize());
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

	m_clientFilename = data->ReadString(GetUnicodeSupport());
			
	// 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
	// if the remote client answers the OP_REQUESTFILENAME with OP_REQFILENAMEANSWER the file is shared by the remote client. if we
	// know that the file is shared, we know also that the file is complete and don't need to request the file status.
	if (m_reqfile->GetPartCount() == 1) {
		m_nPartCount = m_reqfile->GetPartCount();
		
		m_reqfile->UpdatePartsFrequency( this, false );	// Decrement
		m_downPartStatus.clear();
		m_downPartStatus.resize( m_nPartCount, 1 );
		m_reqfile->UpdatePartsFrequency( this, true );	// Increment
		
		m_bCompleteSource = true;

		UpdateDisplayedInfo();
		// even if the file is <= PARTSIZE, we _may_ need the hashset for that file (if the file size == PARTSIZE)
		if (m_reqfile->IsHashSetNeeded()) {
			if (m_socket) {
				CPacket* packet = new CPacket(OP_HASHSETREQUEST,16);
				packet->Copy16ToDataBuffer((const char *)m_reqfile->GetFileHash().GetHash());
				theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
				SendPacket(packet,true,true);
				SetDownloadState(DS_REQHASHSET);
				m_fHashsetRequesting = 1;
				m_reqfile->SetHashSetNeeded(false);
			} else {
				wxASSERT(0);
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
	int iNeeded = 0;
	if (!nED2KPartCount)
	{
		m_nPartCount = m_reqfile->GetPartCount();
		m_downPartStatus.resize( m_nPartCount, 1);
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
		m_downPartStatus.resize( m_nPartCount, 0 );
		uint16 done = 0;
	
		try {
			while (done != m_nPartCount) {
				uint8 toread = data->ReadUInt8();
			
				for ( uint8 i = 0;i < 8; i++ ) {
					m_downPartStatus[done] = ((toread>>i)&1)? 1:0;
				
					if ( m_downPartStatus[done] ) {
						if (!m_reqfile->IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1)){
							bPartsNeeded = true;
							iNeeded++;
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
				CPacket* packet = new CPacket(OP_HASHSETREQUEST,16);
				packet->Copy16ToDataBuffer((const char *)m_reqfile->GetFileHash().GetHash());
				theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
				SendPacket(packet, true, true);
				SetDownloadState(DS_REQHASHSET);
				m_fHashsetRequesting = 1;
				m_reqfile->SetHashSetNeeded(false);
			} else {
				wxASSERT(0);
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
	return m_A4AF_list.erase( file );
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
			theStats::AddDownloadingSource();
		} else if (m_nDownloadState == DS_DOWNLOADING) {
			theStats::RemoveDownloadingSource();
		}
		if (m_nDownloadState == DS_DOWNLOADING) {
			m_nDownloadState = byNewState;
			for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition();pos != 0; ) {
				Requested_Block_Struct* cur_block = m_DownloadBlocks_list.GetNext(pos);
				if (m_reqfile) {
					m_reqfile->RemoveBlockFromList(cur_block->StartOffset,cur_block->EndOffset);
				}
				delete cur_block;
			}
			m_DownloadBlocks_list.RemoveAll();

			for (POSITION pos = m_PendingBlocks_list.GetHeadPosition();pos != 0; ) {
				Pending_Block_Struct *pending = m_PendingBlocks_list.GetNext(pos);
				if (m_reqfile) {
					m_reqfile->RemoveBlockFromList(pending->block->StartOffset, pending->block->EndOffset);
				}
				delete pending->block;
				// Not always allocated
				if (pending->zStream) {
					inflateEnd(pending->zStream);
					delete pending->zStream;
				}
				delete pending;
			}
			m_PendingBlocks_list.RemoveAll();
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
	m_dwLastBlockReceived = ::GetTickCount();
	if (!m_reqfile) {
		return;
	}
	if (m_DownloadBlocks_list.IsEmpty()) {
		// Barry - instead of getting 3, just get how many is needed
		uint16 count = 3 - m_PendingBlocks_list.GetCount();
		Requested_Block_Struct* toadd[count];
		if (m_reqfile->GetNextRequestedBlock(this,toadd,&count)) {
			for (int i = 0; i != count; i++) {
				m_DownloadBlocks_list.AddTail(toadd[i]);
			}
		}
	}

	// Barry - Why are unfinished blocks requested again, not just new ones?

	while (m_PendingBlocks_list.GetCount() < 3 && !m_DownloadBlocks_list.IsEmpty()) {
		Pending_Block_Struct* pblock = new Pending_Block_Struct;
		pblock->block = m_DownloadBlocks_list.RemoveHead();
		pblock->zStream = NULL;
		pblock->totalUnzipped = 0;
		pblock->fZStreamError = 0;
		pblock->fRecovered = 0;
		m_PendingBlocks_list.AddTail(pblock);
	}
	
	
	if (m_PendingBlocks_list.IsEmpty()) {
		if (!GetSentCancelTransfer()){
			CPacket* packet = new CPacket(OP_CANCELTRANSFER,0);
			theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
			SendPacket(packet,true,true);
			SetSentCancelTransfer(1);
		}
		SetDownloadState(DS_NONEEDEDPARTS);
		#warning Kry - Would be nice to swap A4AF here.
		return;
	}
	
	#warning Kry - I dont specially like this approach, we iterate one time too many
	
	bool bHasLongBlocks =  false;
	
	POSITION pos = m_PendingBlocks_list.GetHeadPosition();
	for (uint32 i = 0; i != 3; i++){
		if (pos){
			Pending_Block_Struct* pending = m_PendingBlocks_list.GetNext(pos);
			wxASSERT( pending->block->StartOffset <= pending->block->EndOffset );
			if (pending->block->StartOffset > 0xFFFFFFFF || pending->block->EndOffset > 0xFFFFFFFF){
				bHasLongBlocks = true;
				if (!SupportsLargeFiles()){
					// Requesting a large block from a client that doesn't support large files?
					wxASSERT( false );
					if (!GetSentCancelTransfer()){
						CPacket* packet = new CPacket(OP_CANCELTRANSFER,0);
						theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
						SendPacket(packet,true,true);
						SetSentCancelTransfer(1);
					}					
					SetDownloadState(DS_ERROR);
				}
				break;
			}
		}
	}
	
	CMemFile data(16 /*Hash*/ + (3*(bHasLongBlocks ? 8 : 4) /* uint32/64 start*/) + (3*(bHasLongBlocks ? 8 : 4)/* uint32/64 end*/));
	data.WriteHash(m_reqfile->GetFileHash());
	
	pos = m_PendingBlocks_list.GetHeadPosition();

	for (uint32 i = 0; i != 3; i++) {
		if (pos) {
			Pending_Block_Struct* pending = m_PendingBlocks_list.GetNext(pos);
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
	pos = m_PendingBlocks_list.GetHeadPosition();
	for (uint32 i = 0; i != 3; i++) {
		if (pos) {
			Requested_Block_Struct* block = m_PendingBlocks_list.GetNext(pos)->block;
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
	
	CPacket* packet = new CPacket(&data, (bHasLongBlocks ? OP_EMULEPROT : OP_EDONKEYPROT), (bHasLongBlocks ? (uint8)OP_REQUESTPARTS_I64 : (uint8)OP_REQUESTPARTS));
	theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
	SendPacket(packet, true, true);
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

	int header_size = 16;

	// Update stats
	m_dwLastBlockReceived = ::GetTickCount();

	// Read data from packet
	const CMemFile data(packet, size);

	// Check that this data is for the correct file
	if ((!m_reqfile) || data.ReadHash() != m_reqfile->GetFileHash()) {
		throw wxString(wxT("Wrong fileid sent (ProcessBlockPacket)"));
	}

	// Find the start & end positions, and size of this chunk of data
	uint64 nStartPos;
	uint64 nEndPos = 0;
	uint32 nBlockSize = 0;

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

	credits->AddDownloaded(size - header_size, GetIP(), theApp.CryptoAvailable());
	
	// Move end back one, should be inclusive
	nEndPos--;

	// Loop through to find the reserved block that this is within
	for ( POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != NULL; m_PendingBlocks_list.GetNext(pos) ) {

		Pending_Block_Struct* cur_block = m_PendingBlocks_list.GetAt(pos);
		
		if ((cur_block->block->StartOffset <= nStartPos) && (cur_block->block->EndOffset >= nStartPos)) {
			// Found reserved block

			if (cur_block->fZStreamError){
				AddDebugLogLineM( false, logZLib, wxString::Format(wxT("Ignoring %u bytes of block %u-%u because of erroneous zstream state for file : "), size - header_size, nStartPos, nEndPos) + m_reqfile->GetFileName());
				m_reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
				return;
			}
	   
			// Remember this start pos, used to draw part downloading in list
			m_nLastBlockOffset = nStartPos;

			// Occasionally packets are duplicated, no point writing it twice
			// This will be 0 in these cases, or the length written otherwise
			uint32 lenWritten = 0;

			// Handle differently depending on whether packed or not
			if (!packed) {
				// Write to disk (will be buffered in part file class)
				lenWritten = m_reqfile->WriteToBuffer( size - header_size, 
													   (byte*)(packet + header_size),
													   nStartPos,
													   nEndPos,
													   cur_block->block );
			} else {
				// Packed
				wxASSERT( (long int)size > 0 );
				// Create space to store unzipped data, the size is
				// only an initial guess, will be resized in unzip()
				// if not big enough
				uint32 lenUnzipped = (size * 2);
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
							AddDebugLogLineM( false, logZLib, wxT("Corrupted compressed packet for ") + m_reqfile->GetFileName() + wxT(" received (error 666)"));
							m_reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
						} else {
							// Write uncompressed data to file
							lenWritten = m_reqfile->WriteToBuffer( size - header_size,
																   unzipped,
																   nStartPos,
																   nEndPos,
																   cur_block->block );
						}
					}
				} else {
					wxString strZipError;
					if (cur_block->zStream && cur_block->zStream->msg) {
						strZipError = wxT(" - ") + wxString::FromAscii(cur_block->zStream->msg);
					} 
					
					AddDebugLogLineM( false, logZLib, wxString(wxT("Corrupted compressed packet for")) + m_reqfile->GetFileName() + wxString::Format(wxT("received (error %i) ") , result) + strZipError );
					
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
				m_nTransferedDown += lenWritten;

				// If finished reserved block
				if (nEndPos == cur_block->block->EndOffset) {
					m_reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
					delete cur_block->block;
					// Not always allocated
					if (cur_block->zStream) {
				  		inflateEnd(cur_block->zStream);
				 		delete cur_block->zStream;
					}
					delete cur_block;
					m_PendingBlocks_list.RemoveAt(pos);

					// Request next block
					SendBlockRequests();
				}
			}
			// Stop looping and exit method
			return;
		}
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
			strZipError = wxString::Format(wxT(" %d '"), err) + wxString::FromAscii(zS->msg) + wxT("'");
		} else if (err != Z_OK) {
			strZipError = wxString::Format(wxT(" %d"), err);
		}
		
		AddDebugLogLineM( false, logZLib, wxString(wxT("Unexpected zip error ")) +  strZipError + wxString(wxT("in file \"")) + (m_reqfile ? m_reqfile->GetFileName() : wxString(wxT("?"))) + wxT("\""));
	}

	if (err != Z_OK) {
		(*lenUnzipped) = 0;
	}
	
	return err;
}


// Emilio: rewrite of eMule code to eliminate use of lists for averaging and fix
// errors in calculation (32-bit rollover and time measurement)  This function 
// uses a first-order filter with variable time constant (initially very short 
// to quickly reach the right value without spiking, then gradually approaching 
// the value of 50 seconds which is equivalent to the original averaging period 
// used in eMule).  The download rate is measured using actual timestamps.  The 
// filter-based averaging however uses a simplified algorithm that assumes a 
// fixed loop time - this does not introduce any measurement error, it simply 
// makes the degree of smoothing slightly imprecise (the true TC of the filter 
// varies inversely with the true loop time), which is of no importance here.

float CUpDownClient::CalculateKBpsDown()
{
												// -- all timing values are in seconds --
	const	float tcLoop   =  0.1;				// _assumed_ Process() loop time = 0.1 sec
	const	float tcInit   =  0.4;				// initial filter time constant
	const	float tcFinal  = 50.0;				// final filter time constant
	const	float tcReduce =  5.0;				// transition from tcInit to tcFinal
	
	const	float fInit  = tcLoop/tcInit;		// initial averaging factor
	const	float fFinal = tcLoop/tcFinal;		// final averaging factor
	const	float fReduce = std::exp(std::log(fFinal/fInit) / (tcReduce/tcLoop)) * 0.99999;
	
	uint32	msCur = ::GetTickCount();

	if (msReceivedPrev == 0) {  // initialize the averaging filter
		fDownAvgFilter = fInit;
		// "kBpsDown =  bytesReceivedCycle/1024.0 / tcLoop"  would be technically correct,
		// but the first loop often receives a large chunk of data and then produces a spike
		kBpsDown = /* 0.0 * (1.0-fInit) + */ bytesReceivedCycle/1024.0 / tcLoop * fInit;
		bytesReceivedCycle = 0;
	} else if (msCur != msReceivedPrev) {	// (safeguard against divide-by-zero)
		if (fDownAvgFilter > fFinal) {		// reduce time constant during ramp-up phase
			fDownAvgFilter *= fReduce;		// this approximates averaging a lengthening list
		}
		kBpsDown = kBpsDown * (1.0 - fDownAvgFilter) 
					+ (bytesReceivedCycle/1.024)/((float)(msCur-msReceivedPrev)) * fDownAvgFilter;
		bytesReceivedCycle = 0;
	}
	msReceivedPrev = msCur;	

	m_cShowDR++;
	if (m_cShowDR == 30){
		m_cShowDR = 0;
		UpdateDisplayedInfo();
	}
	if ((::GetTickCount() - m_dwLastBlockReceived) > DOWNLOADTIMEOUT){
		if (!GetSentCancelTransfer()){
			CPacket* packet = new CPacket(OP_CANCELTRANSFER,0);
			theStats::AddUpOverheadFileRequest(packet->GetPacketSize());
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
		
		theApp.downloadqueue->RemoveSource(this);
		if (!m_socket) {
			if (Disconnected(wxT("UDPReaskFNF m_socket=NULL"))) {
				Safe_Delete();
			}
		}
	} else {
		AddDebugLogLineM( false, logRemoteClient, wxT("UDP ANSWER FNF : ") + GetUserName() + wxT(" - did not remove client because of current download state") );
	}
}

void CUpDownClient::UDPReaskForDownload()
{
	
	wxASSERT(m_reqfile);
	
	if(!m_reqfile || m_bUDPPending ) {
		return;
	}
	
	#warning We should implement the quality tests for udp reliability
	/*
	if( m_nTotalUDPPackets > 3 && ((float)(m_nFailedUDPPackets/m_nTotalUDPPackets) > .3)) {
		return;
	}
	*/

	if (thePrefs::GetEffectiveUDPPort() == 0) {
		return;
	}
		
	if (m_nUDPPort != 0 && !theApp.IsFirewalled() && !IsConnected()) {
		//don't use udp to ask for sources
		if(IsSourceRequestAllowed()) {
			return;
		}
		
		m_bUDPPending = true;
		
		CMemFile data(128);
		data.WriteHash(m_reqfile->GetFileHash());
		
		if (GetUDPVersion() > 3) {
			if (m_reqfile->IsPartFile()) {
				((CPartFile*)m_reqfile)->WritePartStatus(&data);
			}
			else {
				data.WriteUInt16(0);
			}
		}
		
		if (GetUDPVersion() > 2) {
			data.WriteUInt16(m_reqfile->m_nCompleteSourcesCount);
		}
		
		CPacket* response = new CPacket(&data, OP_EMULEPROT, OP_REASKFILEPING);
		AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket: send OP_REASKFILEPING") );
		theStats::AddUpOverheadFileRequest(response->GetPacketSize());
		theApp.clientudp->SendPacket(response,GetConnectIP(),GetUDPPort());
	} else  {
		if (HasLowID() && GetBuddyIP() && GetBuddyPort() && HasValidBuddyID()) {
			
			m_bUDPPending = true;
			
			CMemFile data(128);
			
			data.WriteHash(CMD4Hash(GetBuddyID()));
			data.WriteHash(m_reqfile->GetFileHash());
			
			if (GetUDPVersion() > 3) {
				if (m_reqfile->IsPartFile()) {
					((CPartFile*)m_reqfile)->WritePartStatus(&data);
				} else {
					data.WriteUInt16(0);
				}
			}
			
			if (GetUDPVersion() > 2) {
				data.WriteUInt16(m_reqfile->m_nCompleteSourcesCount);
			}
			
			CPacket* response = new CPacket(&data, OP_EMULEPROT, OP_REASKCALLBACKUDP);
			AddDebugLogLineM( false, logClientUDP, wxT("Client UDP socket: send OP_REASKCALLBACKUDP") );
			theStats::AddUpOverheadFileRequest(response->GetPacketSize());
			theApp.clientudp->SendPacket(response, GetBuddyIP(), GetBuddyPort() );
		}
	}
}

// Barry - Sets string to show parts downloading, eg NNNYNNNNYYNYN

wxString CUpDownClient::ShowDownloadingParts() const
{
	// Initialise to all N's
	wxString Parts(wxT('N'), m_nPartCount);
	
	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != 0; ) {
		Parts.SetChar((m_PendingBlocks_list.GetNext(pos)->block->StartOffset / PARTSIZE), 'Y');
	}
	
	return Parts;
}

void CUpDownClient::UpdateDisplayedInfo(bool force)
{
	uint32 curTick = ::GetTickCount();
	if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE) {
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
			Notify_DownloadCtrlUpdateItem(this);
		}
				
		m_lastRefreshedDLDisplay = curTick;
	}
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

			// We force ignoring of noneeded flag and timestamps
			if ( IsValidSwapTarget( it, true, true ) ) {
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
		
		// Sainity check, if reqfile doesn't own the source, then something
		// is wrong and the swap cannot proceed.
		if ( m_reqfile->DelSource( this ) ) {
			CPartFile* SwapTo = target->first;
			
			// remove this client from the A4AF list of our new m_reqfile
			if ( SwapTo->RemoveA4AFSource( this ) ) {
				Notify_DownloadCtrlRemoveSource(this, SwapTo);
			}

			m_reqfile->RemoveDownloadingSource( this );

			// Do we want to remove it completly? Say if the old file is getting deleted
			if ( !bRemoveCompletely ) {
				m_reqfile->AddA4AFSource( this );
				
				// Set the status of the old file
				m_A4AF_list[m_reqfile].NeededParts = (GetDownloadState() != DS_NONEEDEDPARTS);
				
				// Avoid swapping to this file for a while
				m_A4AF_list[m_reqfile].timestamp = ::GetTickCount(); 
							
				Notify_DownloadCtrlAddSource(m_reqfile, this, A4AF_SOURCE);
			} else {
				Notify_DownloadCtrlRemoveSource( this, m_reqfile );
			}
		
			SetDownloadState(DS_NONE);
			ResetFileStatusInfo();

			m_nRemoteQueueRank = 0;
			m_nOldRemoteQueueRank = 0;

			m_reqfile->UpdatePartsInfo();
	
			SetRequestFile( SwapTo );

			SwapTo->AddSource( this );
		
			Notify_DownloadCtrlAddSource(SwapTo, this, UNAVAILABLE_SOURCE);

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
	request.m_pClient = this;
	request.m_pPartFile = pForFile;
	CAICHHashSet::m_liRequestedData.push_back(request);
	m_fAICHRequested = TRUE;
	CMemFile data;
	data.WriteHash(pForFile->GetFileHash());
	data.WriteUInt16(nPart);
	pForFile->GetAICHHashset()->GetMasterHash().Write(&data);
	CPacket* packet = new CPacket(&data, OP_EMULEPROT, OP_AICHREQUEST);
	theStats::AddUpOverheadOther(packet->GetPacketSize());	
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
	CPartFile* pPartFile = theApp.downloadqueue->GetFileByID(hash);
	CAICHRequestedData request = CAICHHashSet::GetAICHReqDetails(this);
	uint16 nPart = data.ReadUInt16();
	if (pPartFile != NULL && request.m_pPartFile == pPartFile && request.m_pClient == this && nPart == request.m_nPart){
		CAICHHash ahMasterHash(&data);
		if ( (pPartFile->GetAICHHashset()->GetStatus() == AICH_TRUSTED || pPartFile->GetAICHHashset()->GetStatus() == AICH_VERIFIED)
			 && ahMasterHash == pPartFile->GetAICHHashset()->GetMasterHash())
		{
			if(pPartFile->GetAICHHashset()->ReadRecoveryData(request.m_nPart*PARTSIZE, &data)){
				// finally all checks passed, everythings seem to be fine
				AddDebugLogLineM( false, logAICHTransfer, wxT("AICH Packet Answer: Succeeded to read and validate received recoverydata"));
				CAICHHashSet::RemoveClientAICHRequest(this);
				pPartFile->AICHRecoveryDataAvailable(request.m_nPart);
				return;
			} else {
				AddDebugLogLineM( false, logAICHTransfer, wxT("AICH Packet Answer: Succeeded to read and validate received recoverydata"));
			}
		} else {
			AddDebugLogLineM( false, logAICHTransfer, wxT("AICH Packet Answer: Masterhash differs from packethash or hashset has no trusted Masterhash") );
		}
	} else {
		AddDebugLogLineM( false, logAICHTransfer, wxT("AICH Packet Answer: requested values differ from values in packet") );
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
	CKnownFile* pKnownFile = theApp.sharedfiles->GetFileByID(hash);
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
				AddDebugLogLineM( false, logAICHTransfer, wxT("AICH Packet Request: Sucessfully created and send recoverydata for ") + pKnownFile->GetFileName() + wxT(" to ") + GetClientFullInfo() );
				
				CPacket* packAnswer = new CPacket(&fileResponse, OP_EMULEPROT, OP_AICHANSWER);			
				theStats::AddUpOverheadOther(packAnswer->GetPacketSize());
				SafeSendPacket(packAnswer);
				return;
			} else {
				AddDebugLogLineM( false, logAICHTransfer, wxT("AICH Packet Request: Failed to create recoverydata for ") + pKnownFile->GetFileName() + wxT(" to ") + GetClientFullInfo() );
			}
		} else{
			AddDebugLogLineM( false, logAICHTransfer, wxT("AICH Packet Request: Failed to create ecoverydata - Hashset not ready or requested Hash differs from Masterhash for ") + pKnownFile->GetFileName() + wxT(" to ") + GetClientFullInfo() );
		}
	} else {
		AddDebugLogLineM( false, logAICHTransfer, wxT("AICH Packet Request: Failed to find requested shared file - ") + GetClientFullInfo() );
	}
		
	CPacket* packAnswer = new CPacket(OP_AICHANSWER, 16, OP_EMULEPROT);
	packAnswer->Copy16ToDataBuffer(hash.GetHash());
	theStats::AddUpOverheadOther(packAnswer->GetPacketSize());
	SafeSendPacket(packAnswer);
}

void CUpDownClient::ProcessAICHFileHash(CMemFile* data, const CPartFile* file){
	CPartFile* pPartFile;
	if (file == NULL){
		pPartFile = theApp.downloadqueue->GetFileByID(data->ReadHash());
	} else {
		pPartFile = (CPartFile*)file;
	}
	CAICHHash ahMasterHash(data);
	
	if(pPartFile != NULL && pPartFile == GetRequestFile()){
		SetReqFileAICHHash(new CAICHHash(ahMasterHash));
		pPartFile->GetAICHHashset()->UntrustedHashReceived(ahMasterHash, GetConnectIP());
	} else {
		AddDebugLogLineM( false, logAICHTransfer, wxT("ProcessAICHFileHash(): PartFile not found or Partfile differs from requested file, ") + GetClientFullInfo() );
	}
}
