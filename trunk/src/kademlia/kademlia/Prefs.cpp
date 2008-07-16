//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2008 Angel Vidal ( kry@amule.org )
// Copyright (c) 2004-2008 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2003 Barry Dunne (http://www.emule-project.net)
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

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#include "Prefs.h"

#include <common/MD5Sum.h>

#include "Kademlia.h"
#include "Indexed.h"
#include "UDPFirewallTester.h"
#include "../../amule.h"
#include "../../CFile.h"
#include "../../ServerList.h"
#include "../../Logger.h"
#include "../../ArchSpecific.h"


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CPrefs::CPrefs()
{
	Init(theApp->ConfigDir + wxT("preferencesKad.dat"));
}

CPrefs::~CPrefs()
{
	if (!m_filename.IsEmpty()) {
		WriteFile();
	}
}

void CPrefs::Init(const wxString& filename)
{
	m_clientID.SetValueRandom();
	m_lastContact = 0;
	m_recheckip = 0;
	m_firewalled = 0;
	m_totalFile = 0;
	m_totalStoreSrc = 0;
	m_totalStoreKey = 0;
	m_totalSource = 0;
	m_totalNotes = 0;
	m_totalStoreNotes = 0;
	m_Publish = false;
	m_clientHash.SetValueBE(thePrefs::GetUserHash().GetHash());
	m_ip = 0;
	m_ipLast = 0;
	m_findBuddy = false;
	m_kademliaUsers	= 0;
	m_kademliaFiles	= 0;
	m_filename = filename;
	m_lastFirewallState = true;
	m_externKadPort = 0;
	m_useExternKadPort = true;
	ReadFile();
}

void CPrefs::ReadFile()
{
	const CPath path = CPath(m_filename);

	try {
		CFile file;
		if (path.FileExists() && file.Open(path, CFile::read)) {
			m_ip = file.ReadUInt32();
			file.ReadUInt16();
			m_clientID = file.ReadUInt128();
			// get rid of invalid kad IDs which may have been stored by older versions
			if (m_clientID == 0)
				m_clientID.SetValueRandom();
			file.Close();
		}
	} catch (const CSafeIOException& e) {
		AddDebugLogLineM(true, logKadPrefs, wxT("IO error while reading prefs: ") + e.what());
	}
}

void CPrefs::WriteFile()
{
	try {
		CFile file;
		if (file.Open(m_filename, CFile::write)) {
			file.WriteUInt32(m_ip);
			file.WriteUInt16(0); //This is no longer used.
			file.WriteUInt128(m_clientID);
			file.WriteUInt8(0); //This is to tell older clients there are no tags..
			file.Close();
		}
	} catch (const CIOFailureException& e) {
		AddDebugLogLineM(true, logKadPrefs, wxT("IO failure while saving kad-prefs: ") + e.what());
	}
}

void CPrefs::SetIPAddress(uint32_t val) throw()
{
	//This is our first check on connect, init our IP..
	if ( !val || !m_ipLast ) {
		m_ip = val;
		m_ipLast = val;
	}
	//If the last check matches this one, reset our current IP.
	//If the last check does not match, wait for our next incoming IP.
	//This happens for two reasons.. We just changed our IP, or a client responsed with a bad IP.
	if ( val == m_ipLast ) {
		m_ip = val;
	} else {
		m_ipLast = val;
	}
}


bool CPrefs::GetFirewalled() const throw()
{
	if (m_firewalled < 2) {
		//Not enough people have told us we are open but we may be doing a recheck
		//at the moment which will give a false lowID.. Therefore we check to see
		//if we are still rechecking and will report our last known state..
		if (GetRecheckIP()) {
			return m_lastFirewallState;
		}
		return true;
	}
	//We had enough tell us we are not firewalled..
	return false;
}

void CPrefs::SetFirewalled()
{
	//Are are checking our firewall state.. Let keep a snapshot of our
	//current state to prevent false reports during the recheck..
	m_lastFirewallState = (m_firewalled < 2);
	m_firewalled = 0;
	theApp->ShowConnectionState();
}

void CPrefs::IncFirewalled()
{
	m_firewalled++;
	theApp->ShowConnectionState();
}

void CPrefs::SetKademliaFiles()
{
	//There is no real way to know how many files are in the Kad network..
	//So we first try to see how many files per user are in the ED2K network..
	//If that fails, we use a set value based on previous tests..
	uint32_t nServerAverage = theApp->serverlist->GetAvgFile();
	uint32_t nKadAverage = Kademlia::CKademlia::GetIndexed()->GetFileKeyCount();

#ifdef __DEBUG__
	wxString method;
#endif
	if (nServerAverage > nKadAverage) {
#ifdef __DEBUG__
		method = wxString::Format(wxT("Kad file estimate used Server avg(%u)"), nServerAverage);
#endif
		nKadAverage = nServerAverage;
	}
#ifdef __DEBUG__
	   else {
		method = wxString::Format(wxT("Kad file estimate used Kad avg(%u)"), nKadAverage);
	}
#endif
	if( nKadAverage < 108 ) {
#ifdef __DEBUG__
		method = wxString(wxT("Kad file estimate used default avg(108, min value)"));
#endif
		nKadAverage = 108;
	}
#ifdef ___DEBUG__
	AddDebugLogLineM(false, logKadPrefs, method);
#endif
	m_kademliaFiles = nKadAverage * m_kademliaUsers;
}

uint8_t CPrefs::GetMyConnectOptions(bool encryption, bool callback)
{
       // Connect options Tag
       // 4 Reserved (!)
       // 1 Direct Callback
       // 1 CryptLayer Required
       // 1 CryptLayer Requested
       // 1 CryptLayer Supported

       // direct callback is only possible if connected to kad, tcp firewalled and verified UDP open (for example on a full cone NAT)

       return    (callback && theApp->IsFirewalled() && CKademlia::IsRunning() && !CUDPFirewallTester::IsFirewalledUDP(true) && CUDPFirewallTester::IsVerified()) ? 0x08 : 0
	       | (thePrefs::IsClientCryptLayerRequired() && encryption) ? 0x04 : 0
	       | (thePrefs::IsClientCryptLayerRequested() && encryption) ? 0x02 : 0
	       | (thePrefs::IsClientCryptLayerSupported() && encryption) ? 0x01 : 0;
}

uint32_t CPrefs::GetUDPVerifyKey(uint32_t targetIP) throw()
{
	uint64_t buffer = (uint64_t)thePrefs::GetKadUDPKey() << 32 | targetIP;
	MD5Sum md5((const uint8_t *)&buffer, 8);
	return (uint32_t)(PeekUInt32(md5.GetRawHash()) ^ PeekUInt32(md5.GetRawHash() + 4) ^ PeekUInt32(md5.GetRawHash() + 8) ^ PeekUInt32(md5.GetRawHash() + 12)) % 0xFFFFFFFE + 1;
}
// File_checked_for_headers