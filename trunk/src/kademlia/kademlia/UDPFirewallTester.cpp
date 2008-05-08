//
// This file is part of the aMule Project.
//
// Copyright (c) 2008 Dévai Tamás ( gonosztopi@amule.org )
// Copyright (c) 2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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

#define need_UDP_FIREWALLTEST_CLIENTSTOASK

#include "UDPFirewallTester.h"
#include "../utils/UInt128.h"
#include "../utils/KadUDPKey.h"
#include "../routing/RoutingZone.h"
#include <common/Macros.h>
#include "Kademlia.h"
#include "Prefs.h"
#include "SearchManager.h"
#include "../../Logger.h"
#include "../../amule.h"
#include "../../ClientList.h"
#include "../../GetTickCount.h"
#include "../../NetworkFunctions.h"


using namespace Kademlia;

bool	CUDPFirewallTester::m_firewalledUDP		= false;
bool	CUDPFirewallTester::m_firewalledLastStateUDP	= false;
bool	CUDPFirewallTester::m_isFWVerifiedUDP		= false;
bool	CUDPFirewallTester::m_nodeSearchStarted		= false;
bool	CUDPFirewallTester::m_timedOut			= false;
uint8_t	CUDPFirewallTester::m_fwChecksRunningUDP	= 0;
uint8_t	CUDPFirewallTester::m_fwChecksFinishedUDP	= 0;
uint32_t CUDPFirewallTester::m_testStart		= 0;
CUDPFirewallTester::PossibleClientList	CUDPFirewallTester::m_possibleTestClients;
CUDPFirewallTester::UsedClientList	CUDPFirewallTester::m_usedTestClients;


bool CUDPFirewallTester::IsFirewalledUDP(bool lastStateIfTesting)
{ 
	if (!m_timedOut && IsFWCheckUDPRunning()) {
		if (!m_firewalledUDP && CKademlia::IsFirewalled() && m_testStart != 0 && ::GetTickCount() - m_testStart > MIN2MS(6)
			&& !m_isFWVerifiedUDP /*For now we don't allow to get firewalled by timeouts if we have succeded a test before, might be changed later*/)
		{
			AddDebugLogLineM(false, logKadUdpFwTester, wxT("Timeout: Setting UDP status to firewalled after being unable to get results for 6 minutes"));
			m_timedOut = true;
			theApp->ShowConnectionState();
		}
	}
	else if (m_timedOut && IsFWCheckUDPRunning()) {
		return true; // firewallstate by timeout
	}
	else if (m_timedOut) {
		wxFAIL;
	}

	if (lastStateIfTesting && IsFWCheckUDPRunning()) {
		return m_firewalledLastStateUDP;
	} else {
		return m_firewalledUDP;
	}
}

void CUDPFirewallTester::SetUDPFWCheckResult(bool succeeded, bool testCancelled, uint32_t fromIP, uint16_t incomingPort)
{
	if (!IsFWCheckUDPRunning()) {
		return;
	}

	// check if we actually requested a firewallcheck from this client
	bool requested = false;
	for (UsedClientList::iterator it = m_usedTestClients.begin(); it != m_usedTestClients.end(); ++it) {
		if (it->contact.GetIPAddress() == fromIP) {
			if (it->answered) {
				// we already received an answer. This may happen since all tests contain of two answer pakcets,
				// but the answer could also be too late and we already counted it as failure. In any way, we ignore
				// multiple answers
				return;
			} else {
				it->answered = true;
			}
			requested = true;
			break;
		}
	}

	if (!requested){
		AddDebugLogLineM(false, logKadUdpFwTester, wxT("Unrequested UDPFWCheckResult from ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(fromIP)));
		return;
	}

	if (m_fwChecksRunningUDP == 0) {
		wxFAIL;
	} else {
		m_fwChecksRunningUDP--;
	}

	if (!testCancelled){
		m_fwChecksFinishedUDP++;
		if (succeeded) {	//one positive result is enough
			AddDebugLogLineM(false, logKadUdpFwTester, wxT("New KAD Firewallstate (UDP): Open"));
			m_testStart = 0;
			m_firewalledUDP = false;
			m_isFWVerifiedUDP = true;
			m_timedOut = false;
			m_fwChecksFinishedUDP = UDP_FIREWALLTEST_CLIENTSTOASK; // dont do any more tests			
			theApp->ShowConnectionState();
			m_possibleTestClients.clear(); // clear list, keep used clients list though
			CSearchManager::CancelNodeFWCheckUDPSearch(); // cancel firewallnode searches if any are still active
			// if this packet came to our internal port, explict set the interal port as used port from now on
			if (incomingPort == CKademlia::GetPrefs()->GetExternalKadPort()) {
				CKademlia::GetPrefs()->SetUseExternKadPort(true);
			} else if (incomingPort == CKademlia::GetPrefs()->GetInternKadPort()) {
				CKademlia::GetPrefs()->SetUseExternKadPort(false);
			}
			return;
		} else if (m_fwChecksFinishedUDP >= UDP_FIREWALLTEST_CLIENTSTOASK) {
			// seems we are firewalled
			m_testStart = 0;
			AddDebugLogLineM(false, logKadUdpFwTester, wxT("New KAD Firewallstate (UDP): Firewalled"));
			m_firewalledUDP = true;
			m_isFWVerifiedUDP = true;
			m_timedOut = false;
			theApp->ShowConnectionState();
			m_possibleTestClients.clear(); // clear list, keep used clients list though
			CSearchManager::CancelNodeFWCheckUDPSearch(); // cancel firewallnode searches if any are still active
			return;
		} else
			AddDebugLogLineM(false, logKadUdpFwTester, wxT("Kad UDP firewalltest from ") + Uint32toStringIP(wxUINT32_SWAP_ALWAYS(fromIP)) + wxT(" result: Firewalled, continue testing"));
	}
	QueryNextClient();
}

void CUDPFirewallTester::ReCheckFirewallUDP(bool setUnverified)
{
	wxASSERT(m_fwChecksRunningUDP == 0);
	m_fwChecksRunningUDP = 0;
	m_fwChecksFinishedUDP = 0;
	m_testStart = ::GetTickCount();
	m_timedOut = false;
	m_firewalledLastStateUDP = m_firewalledUDP;
	m_isFWVerifiedUDP = (m_isFWVerifiedUDP && !setUnverified);
	CSearchManager::FindNodeFWCheckUDP(); // start a lookup for a random node to find suitable IPs
	m_nodeSearchStarted = true;
	CKademlia::GetPrefs()->SetExternKadPort(0);
}

void CUDPFirewallTester::Connected()
{
	if (!m_nodeSearchStarted && IsFWCheckUDPRunning()) {
		CSearchManager::FindNodeFWCheckUDP(); // start a lookup for a random node to find suitable IPs
		m_nodeSearchStarted = true;
		m_testStart = ::GetTickCount();
		m_timedOut = false;
	}
}

void CUDPFirewallTester::Reset()
{
	m_firewalledUDP = false;
	m_firewalledLastStateUDP = false;
	m_isFWVerifiedUDP = false;
	m_nodeSearchStarted = false;
	m_timedOut = false;
	m_fwChecksRunningUDP = 0;
	m_fwChecksFinishedUDP = 0;
	m_testStart = 0;
	CSearchManager::CancelNodeFWCheckUDPSearch(); // cancel firewallnode searches if any are still active
	m_possibleTestClients.clear();
	// keep the list of used clients
}

void CUDPFirewallTester::QueryNextClient()
{	// try the next available client for the firewallcheck
	if (!IsFWCheckUDPRunning() || !GetUDPCheckClientsNeeded() || CKademlia::GetPrefs()->GetExternalKadPort() == 0) {
		return; // check if more tests are needed and wait till we know our extern port
	}

	if (!CKademlia::IsRunning() || CKademlia::GetRoutingZone() == NULL) {
		wxFAIL;
		return;
	}

	while (!m_possibleTestClients.empty()) {
		CContact curContact = m_possibleTestClients.front();
		m_possibleTestClients.pop_front();
		// udp firewallchecks are not supported by clients with kadversion < 6
		if (curContact.GetVersion() <= 5) {
			continue;
		}

		// sanity - do not test ourself
		if (wxUINT32_SWAP_ALWAYS(curContact.GetIPAddress()) == theApp->GetPublicIP() || curContact.GetClientID() == CKademlia::GetPrefs()->GetKadID()) {
			continue;
		}

		// check if we actually requested a firewallcheck from this client at some point
		bool alreadyRequested = false;
		for (UsedClientList::const_iterator it = m_usedTestClients.begin(); it != m_usedTestClients.end(); ++it) {
			if (it->contact.GetIPAddress() == curContact.GetIPAddress()) {
				alreadyRequested = true;
				break;
			}
		}

		// check if we know its IP already from kademlia - we need an IP which was never used for UDP yet
		if (!alreadyRequested && CKademlia::GetRoutingZone()->GetContact(curContact.GetIPAddress(), 0, false) == NULL) {
			// ok, tell the clientlist to do the same search and start the check if ok
			if (theApp->clientlist->DoRequestFirewallCheckUDP(curContact)) {
				UsedClient_Struct sAdd = { curContact, false };
				m_usedTestClients.push_front(sAdd);
				m_fwChecksRunningUDP++;
				break;
			}
		}
	}
}
