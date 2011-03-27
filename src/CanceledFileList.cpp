//
// This file is part of the aMule Project.
//
// Copyright (c) 2010-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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


#include "CanceledFileList.h"	// Interface declarations

#include <common/DataFileVersion.h>
#include "amule.h"
#include "CFile.h"
#include "Logger.h"
#include <common/Format.h>


CCanceledFileList::CCanceledFileList()
{
	m_filename = wxT("canceled.met");
	Init();
}


bool CCanceledFileList::Init()
{
	CFile file;
	
	CPath fullpath = CPath(theApp->ConfigDir + m_filename);
	if (!fullpath.FileExists()) {
		// This is perfectly normal. The file was probably either
		// deleted, or this is the first time running aMule.
		return false;
	}

	if (!file.Open(fullpath)) {
		AddLogLineC(CFormat(_("WARNING: %s cannot be opened.")) % m_filename);
		return false;
	}
	
	try {
		uint8 version = file.ReadUInt8();
		if (version != CANCELEDFILE_VERSION) {
			AddLogLineC(_("WARNING: Canceled file list corrupted, contains invalid header."));
			return false;
		}
		
		uint32 RecordsNumber = file.ReadUInt32();
		AddDebugLogLineN(logKnownFiles,
			CFormat(wxT("Reading %i canceled files from file format 0x%02x."))
			% RecordsNumber % version);
		for (uint32 i = 0; i < RecordsNumber; i++) {
			CMD4Hash hash;
			file.Read(hash.GetHash(), 16);
			AddDebugLogLineN(logKnownFiles, CFormat(wxT("Canceled file read: %s")) % hash.Encode());
			if (!hash.IsEmpty()) {
				m_canceledFileList.insert(hash);
			}
		}
		AddDebugLogLineN(logKnownFiles, wxT("Finished reading canceled files"));
	
		return true;
	} catch (const CSafeIOException& e) {
		AddLogLineC(CFormat(_("IO error while reading %s file: %s")) % m_filename % e.what());
	}	
	
	return false;
}


void CCanceledFileList::Save()
{
	CFile file(theApp->ConfigDir + m_filename, CFile::write);
	if (!file.IsOpened()) {
		return;
	}

	try {
		file.WriteUInt8(CANCELEDFILE_VERSION);
		file.WriteUInt32(m_canceledFileList.size());

		CanceledFileList::iterator it = m_canceledFileList.begin();
		for (; it != m_canceledFileList.end(); ++it) {
			file.Write(it->GetHash(), 16);
		}
	} catch (const CIOFailureException& e) {
		AddLogLineC(CFormat(_("Error while saving %s file: %s")) % m_filename % e.what());
	}
}


bool CCanceledFileList::IsCanceledFile(const CMD4Hash& hash) const
{
	return !hash.IsEmpty() && m_canceledFileList.find(hash) != m_canceledFileList.end();
}


bool CCanceledFileList::Add(const CMD4Hash& hash)
{
	return m_canceledFileList.insert(hash).second;
}


bool CCanceledFileList::Remove(const CMD4Hash& hash)
{
	return m_canceledFileList.erase(hash) > 0;
}


// File_checked_for_headers
