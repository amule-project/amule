//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#include "DataToText.h"

#include "KnownFile.h"		// Needed by PriorityToStr
#include "updownclient.h"	// Needed by DownloadStateToStr and GetSoftName

#include <wx/string.h>
#include <wx/intl.h>


wxString PriorityToStr( int priority, bool isAuto )
{
	if ( isAuto ) {
		switch ( priority ) {
			case PR_LOW:		return _("Auto [Lo]");
			case PR_NORMAL:		return _("Auto [No]");
			case PR_HIGH:		return _("Auto [Hi]");
		}
	} else {
		switch ( priority ) {
			case PR_VERYLOW:	return _("Very low");
			case PR_LOW:		return _("Low");
			case PR_NORMAL:		return _("Normal");
			case PR_HIGH:		return _("High");
			case PR_VERYHIGH:	return _("Very High");
			case PR_POWERSHARE:	return _("Release");
		}
	}

	wxASSERT( false );

	return _("Unknown");
}


wxString DownloadStateToStr( int state, bool queueFull )
{
	switch ( state ) {
		case DS_CONNECTING:		return  _("Connecting");
		case DS_CONNECTED:		return _("Asking");
		case DS_WAITCALLBACK:		return _("Connecting via server");
		case DS_ONQUEUE:		return ( queueFull ? _("Queue Full") : _("On Queue") );
		case DS_DOWNLOADING:		return _("Transferring");
		case DS_REQHASHSET:		return _("Receiving hashset");
		case DS_NONEEDEDPARTS:		return _("No needed parts");
		case DS_LOWTOLOWIP:		return _("Cannot connect LowID to LowID");
		case DS_TOOMANYCONNS:		return _("Too many connections");
		case DS_NONE:			return _("Unknown");
		case DS_WAITCALLBACKKAD: 	return _("Connecting via Kad");
		case DS_TOOMANYCONNSKAD:	return _("Too many Kad connections");
		case DS_BANNED:			return _("Banned");
		case DS_ERROR:			return _("Connection Error");
		case DS_REMOTEQUEUEFULL:	return _("Remote Queue Full");
	}
	
	wxASSERT( false );

	return _("Unknown");
}


const wxString GetSoftName(unsigned int software_ident)
{
	switch (software_ident) {
		case SO_OLDEMULE:
		case SO_EMULE:
			return wxT("eMule");
		case SO_CDONKEY:
			return wxT("cDonkey");
		case SO_LXMULE:
			return wxT("(l/x)Mule");
		case SO_AMULE:
			return wxT("aMule");
		case SO_SHAREAZA:
		case SO_NEW_SHAREAZA:
			return wxT("Shareaza");
		case SO_EMULEPLUS:
			return wxT("eMule+");
		case SO_HYDRANODE:
			return wxT("HydraNode");
		case SO_MLDONKEY:
			return wxTRANSLATE("Old MLDonkey");
		case SO_NEW_MLDONKEY:
		case SO_NEW2_MLDONKEY:
			return wxTRANSLATE("New MLDonkey");
		case SO_LPHANT:
			return wxT("lphant");
		case SO_EDONKEYHYBRID:
			return wxT("eDonkeyHybrid");
		case SO_EDONKEY:
			return wxT("eDonkey");
		case SO_UNKNOWN:
			return wxTRANSLATE("Unknown");
		case SO_COMPAT_UNK:
			return wxTRANSLATE("eMule Compatible");
		default:
			return wxEmptyString;
	}
}


#ifndef EC_REMOTE

const wxString& GetClientDetails(const CUpDownClient *pClient, wxString *clientName, wxString *clientVersion, wxString *clientModName)
{
	wxString l_clientName;
	wxString l_clientVersion;
	wxString l_clientModName;

	if (!(clientName || clientVersion || clientModName)) {
		return pClient->GetClientOSInfo();
	}

	if (!pClient || pClient->GetUserName().IsEmpty()) {
		l_clientName = wxTRANSLATE("Unknown");
	} else {
		uint32 nClientVersion = pClient->GetVersion();
		// Split client version.
		uint32 nClientMajVersion = nClientVersion / 100000;
		uint32 nClientMinVersion = (nClientVersion / 1000) % 100;
		uint32 nClientUpVersion = (nClientVersion / 100) % 10;

		int iHashType = pClient->GetHashType();
		if (iHashType == SO_EMULE) {
			uint32 clientSoft = pClient->GetClientSoft();
			l_clientName = GetSoftName(clientSoft);
			// Special issues:
			if((pClient->GetClientModString().IsEmpty() == false) && (clientSoft != SO_EMULE)) {
				l_clientName = pClient->GetClientModString();
			}
			// Isn't xMule annoying?
			if ((clientSoft == SO_LXMULE) && (pClient->GetMuleVersion() > 0x26) && (pClient->GetMuleVersion() != 0x99)) {
				l_clientName += wxString::Format(_(" (Fake eMule version %#x)"), pClient->GetMuleVersion());
			}
			if ((clientSoft == SO_EMULE) && 
			    (
			     wxString(pClient->GetClientModString()).MakeLower().Find(wxT("xmule")) != -1 
			     || pClient->GetUserName().Find(wxT("xmule.")) != -1
			     )
			    ) {
				// FAKE eMule -a newer xMule faking its ident.
				if (pClient->GetClientModString().IsEmpty() == false) {
					l_clientName = pClient->GetClientModString() + _(" (Fake eMule)");
				} else {
					l_clientName = wxTRANSLATE("xMule (Fake eMule)"); // don't use GetSoftName, it's not lmule.
				}
			}		
			// Now, what if we don't know this SO_ID?
			if (l_clientName.IsEmpty()) {
				if (pClient->IsML() || pClient->IsHybrid()) {
					l_clientName = GetSoftName(clientSoft);
				} else if (pClient->GetCompatibleClient() != 0) {
					l_clientName = GetSoftName(clientSoft) + wxString::Format(wxT("(%#x)"), pClient->GetCompatibleClient());
				} else {
					// If we step here, it might mean 2 things:
					// a eMule
					// a Compat Client that has sent no MuleInfo packet yet.
					l_clientName = wxT("eMule");
				}
			}

			if (pClient->GetMuleVersion() == 0) {
				// just do nothing here
			} else if (pClient->GetMuleVersion() != 0x99) {
				switch (clientSoft) {
				 case SO_AMULE:
					 l_clientVersion += wxString::Format(wxTRANSLATE("1.x (based on eMule v0.%u)"), nClientMinVersion);
					 break;
				 case SO_LPHANT:
					 l_clientVersion = wxT("< v0.05 ");
					 break;
				 default:
					 l_clientVersion = wxString::Format(wxT("v0.%u"), nClientMinVersion);
					 l_clientModName = pClient->GetClientModString();
					 break;
				}
			} else {
				switch (clientSoft) {
				 case SO_AMULE:
				 case SO_LXMULE:
				 case SO_HYDRANODE:
					 // Kry - xMule started sending correct version tags on 1.9.1b.
					 // It only took them 4 months, and being told by me and the
					 // eMule+ developers, so I think they're slowly getting smarter.
					 // They are based on our implementation, so we use the same format
					 // for the version string.
					 l_clientVersion =  wxString::Format(wxT("v%u.%u.%u"), nClientMajVersion, nClientMinVersion, nClientUpVersion);
					 break;
				 case SO_LPHANT:
					 l_clientVersion =  wxString::Format(wxT("v%u.%.2u%c"), nClientMajVersion-1, nClientMinVersion, 'a' + nClientUpVersion);
					 break;
				 case SO_EMULEPLUS:
					 l_clientVersion =  wxString::Format(wxT("v%u"), nClientMajVersion);
					 if(nClientMinVersion != 0) {
						 l_clientVersion +=  wxString::Format(wxT(".%u"), nClientMinVersion);
					 }
					 if(nClientUpVersion != 0) {
						 l_clientVersion +=  wxString::Format(wxT("%c"), 'a' + nClientUpVersion - 1);
					 }
					 break;
				 default:
					 l_clientVersion = wxString::Format(wxT("v%u.%u%c"), nClientMajVersion, nClientMinVersion, 'a' + nClientUpVersion);
					 l_clientModName = pClient->GetClientModString();
					 break;
				}
			}
		} else if (pClient->IsHybrid()) {
			l_clientName = GetSoftName(pClient->GetClientSoft());
			if (nClientUpVersion) {
				l_clientVersion = wxString::Format(wxT("v%u.%u.%u"), nClientMajVersion, nClientMinVersion, nClientUpVersion);
			} else {
				l_clientVersion = wxString::Format(wxT("v%u.%u"), nClientMajVersion, nClientMinVersion);
			}
		} else {
			l_clientName = GetSoftName(pClient->GetClientSoft());
			l_clientVersion = wxString::Format(wxT("v%u.%u"), nClientMajVersion, nClientMinVersion);
		}
	}

	// give results
	if (clientName) {
		*clientName = l_clientName;
	}
	if (clientVersion) {
		*clientVersion = l_clientVersion;
	}
	if (clientModName) {
		*clientModName = l_clientModName;
	}

	return pClient->GetClientOSInfo();
}

#endif /* !EC_REMOTE */

wxString OriginToText(unsigned int source_from)
{
	switch ((ESourceFrom)source_from) {
		case SF_SERVER:		return wxTRANSLATE("Server");
		case SF_KADEMLIA:	return wxTRANSLATE("Kad");
		case SF_SOURCE_EXCHANGE: return wxTRANSLATE("Source Exchange");
		case SF_PASSIVE:	return wxTRANSLATE("Passive");
		case SF_LINK:		return wxTRANSLATE("Link");
		case SF_SOURCE_SEEDS:	return wxTRANSLATE("Source Seeds");
		case SF_NONE:
		default:		return wxTRANSLATE("Unknown");
	}
}
