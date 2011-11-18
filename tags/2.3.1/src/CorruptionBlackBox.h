//
// This file is part of the aMule Project.
//
// Copyright (c) 2008-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2008-2011 Stu Redman ( sturedman@amule.org )
// Copyright (C) 2002-2011 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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

#ifndef CORRUPTIONBLACKBOX_H
#define CORRUPTIONBLACKBOX_H

#include "Types.h"
#include <list>
#include <map>

class CCorruptionBlackBox
{
public:
	void Free();
	void TransferredData(uint64 nStartPos, uint64 nEndPos, uint32 senderIP);
	void VerifiedData(bool ok, uint16 nPart, uint32 nRelStartPos, uint32 nRelEndPos);
	void EvaluateData();
	void SetPartFileInfo(const wxString& name, const wxString& nr) { m_fileName = name; m_partNumber = nr; }
	void DumpAll();

private:
	// downloaded data for each part
	class CCBBRecord
	{
	public:
		CCBBRecord(uint32 nStartPos, uint32 nEndPos, uint32 dwIP);
		bool Merge(uint32 nStartPos, uint32 nEndPos, uint32 dwIP);

		// Startpos / Endpos relative to part
		uint32	m_nStartPos;
		uint32	m_nEndPos;
		// IP of client
		uint32	m_dwIP;
	};
	typedef std::list<CCBBRecord> CRecordList;
	std::map<uint16, CRecordList> m_Records;

	// good/bad data for each client
	class CCBBClient
	{
	public:
		CCBBClient() { m_downloaded = 0; }
		uint64 m_downloaded;
	};
	typedef std::map<uint32, CCBBClient> CCBBClientMap;
	CCBBClientMap m_goodClients, m_badClients;

	// for debug prints
	wxString m_fileName;
	wxString m_partNumber;
};

#endif
