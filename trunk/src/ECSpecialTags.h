/*
 * This file is part of the aMule Project
 *
 * Copyright (C) 2004 aMule Team (http://www.amule.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#ifndef ECSPECIALTAGS_H
#define ECSPECIALTAGS_H

#include "Types.h"	// Needed for uint* types
#include <wx/string.h>	// Needed for wxString
#include "ECcodes.h"	// Needed for EC types
#include "ECPacket.h"	// Needed for CECTag
#include "CMD4Hash.h"	// Needed for CMD4Hash

#include <vector>

#ifndef EC_REMOTE
#include "Statistics.h"	// Needed for StatsTree
#endif

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "ECSpecialTags.h"
#endif

/*
 * Specific tags for specific requests
 *
 * \note EC remote end does not need to create these packets,
 * only using the getter functions.
 *
 * Regarding this, those classes are removed from remote build,
 * that have only a constructor.
 */

class CServer;
class CKnownFile;
class CPartFile;
class CSearchFile;
class CUpDownClient;


class CEC_Server_Tag : public CECTag {
 	public:
 		CEC_Server_Tag(CServer *, EC_DETAIL_LEVEL);
};


class CEC_ConnState_Tag : public CECTag {
 	public:
 		CEC_ConnState_Tag(EC_DETAIL_LEVEL);
};


class CEC_PartFile_Tag : public CECTag {
 	public:
 		CEC_PartFile_Tag(CPartFile *file, EC_DETAIL_LEVEL detail_level);
 		
		// template needs it
		CMD4Hash		ID()	{ return GetMD4Data(); }

 		CMD4Hash	FileHash()	{ return GetMD4Data(); }
		wxString	FileHashString() { return GetMD4Data().Encode(); }

 		wxString	FileName()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_NAME);
			return tag ? tag->GetStringData() : wxT("");
		}
 		uint32		SizeFull()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SIZE_FULL);
			return tag ? tag->GetInt32Data() : 0;
		}
 		uint32		SizeXfer()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SIZE_XFER);
			return tag ? tag->GetInt32Data() : 0;
		}
  		uint32		SizeDone()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SIZE_DONE);
			return tag ? tag->GetInt32Data() : 0;
		}
 		wxString	FileEd2kLink()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_ED2K_LINK);
			return tag ? tag->GetStringData() : wxT("");
		}
 		uint8		FileStatus()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_STATUS);
			return tag ? tag->GetInt8Data() : 0;
		}
  		uint32		SourceCount()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT);
			return tag ? tag->GetInt32Data() : 0;
		}
  		uint32		SourceNotCurrCount()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT);
			return tag ? tag->GetInt32Data() : 0;
		}
  		uint32		SourceXferCount()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT_XFER);
			return tag ? tag->GetInt32Data() : 0;
		}
  		uint32		SourceCountA4AF()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT_A4AF);
			return tag ? tag->GetInt32Data() : 0;
		}
  		uint32		Speed()	
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SPEED);
			return tag ? tag->GetInt32Data() : 0;
		}
  		uint32		Prio()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_PRIO);
			return tag ? tag->GetInt32Data() : 0;
		}
  		wxString	PartStatus()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_PART_STATUS);
			return tag ? tag->GetStringData() : wxT("");
		}

		#ifdef EC_REMOTE
		wxString	GetFileStatusString();
		#endif /* EC_REMOTE */
};

class CEC_SharedFile_Tag : public CECTag {
	public:
		CEC_SharedFile_Tag(const CKnownFile *file, EC_DETAIL_LEVEL detail_level);

		// template needs it
 		CMD4Hash	ID()		{ return GetMD4Data(); }
		
 		CMD4Hash	FileHash()	{ return GetMD4Data(); }
		wxString	FileHashString(){ return GetMD4Data().Encode(); }

 		wxString	FileName()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_NAME);
			return tag ? tag->GetStringData() : wxT("");
		}
 		uint32		SizeFull()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SIZE_FULL);
			return tag ? tag->GetInt32Data() : 0;
		}
  		uint32		Prio()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_PRIO);
			return tag ? tag->GetInt32Data() : 0;
		}
 		wxString	FileEd2kLink()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_ED2K_LINK);
			return tag ? tag->GetStringData() : wxT("");
		}

 		uint32		GetRequests()
		{
			CECTag *tag =  GetTagByName(EC_TAG_KNOWNFILE_REQ_COUNT);
			return tag ? tag->GetInt32Data() : 0;
		}
 		uint32		GetAllRequests()
		{
			CECTag *tag =  GetTagByName(EC_TAG_KNOWNFILE_REQ_COUNT_ALL);
			return tag ? tag->GetInt32Data() : 0;
		}

 		uint32		GetAccepts()
		{
			CECTag *tag =  GetTagByName(EC_TAG_KNOWNFILE_ACCEPT_COUNT);
			return tag ? tag->GetInt32Data() : 0;
		}
 		uint32		GetAllAccepts()
		{
			CECTag *tag =  GetTagByName(EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL);
			return tag ? tag->GetInt32Data() : 0;
		}

 		uint32		GetXferred()
		{
			CECTag *tag =  GetTagByName(EC_TAG_KNOWNFILE_XFERRED);
			return tag ? tag->GetInt32Data() : 0;
		}
 		uint32		GetAllXferred()
		{
			CECTag *tag =  GetTagByName(EC_TAG_KNOWNFILE_XFERRED_ALL);
			return tag ? tag->GetInt32Data() : 0;
		}
};

class CEC_UpDownClient_Tag : public CECTag {
	public:
		CEC_UpDownClient_Tag(const CUpDownClient* client, EC_DETAIL_LEVEL detail_level);

 		CMD4Hash FileID()
		{
			CECTag *tag =  GetTagByName(EC_TAG_KNOWNFILE);
			return tag ? tag->GetMD4Data() : CMD4Hash();
		}
 		bool HaveFile() { return GetTagByName(EC_TAG_KNOWNFILE) != NULL; }

 		wxString ClientName()
		{
			CECTag *tag =  GetTagByName(EC_TAG_CLIENT_NAME);
			return tag ? tag->GetStringData() : wxT("");
		}
 		uint32 Speed()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SPEED);
			return tag ? tag->GetInt32Data() : 0;
		}
 		uint32 XferUp()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SIZE_XFER_UP);
			return tag ? tag->GetInt32Data() : 0;
		}
 		uint32 XferDown()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SIZE_XFER);
			return tag ? tag->GetInt32Data() : 0;
		}
};

class CEC_SearchFile_Tag : public CECTag {
	public:
		CEC_SearchFile_Tag(CSearchFile *file, EC_DETAIL_LEVEL detail_level);

		// template needs it
 		CMD4Hash	ID()	{ return GetMD4Data(); }

 		CMD4Hash	FileHash()	{ return GetMD4Data(); }
		wxString	FileHashString() { return GetMD4Data().Encode(); }

 		wxString	FileName()
 		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_NAME);
			return tag ? tag->GetStringData() : wxT("");
		}
		uint32		SizeFull()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SIZE_FULL);
			return tag ? tag->GetInt32Data() : 0;
		}
  		uint32		SourceCount()
		{
			CECTag *tag =  GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT);
			return tag ? tag->GetInt32Data() : 0;
		}
  		bool		AlreadyHave()	{ return GetTagByName(EC_TAG_KNOWNFILE) != 0; }
};

class CEC_Search_Tag : public CECTag {
	public:
		// search request
		CEC_Search_Tag(wxString &name, EC_SEARCH_TYPE search_type, wxString &file_type,
			wxString &extension, uint32 avail, uint32 min_size, uint32 max_size);
			
		wxString SearchText() { return GetTagByName(EC_TAG_SEARCH_NAME)->GetStringData(); }
		EC_SEARCH_TYPE SearchType() { return (EC_SEARCH_TYPE)GetInt32Data(); }
		uint32 MinSize()
		{
			CECTag *tag =  GetTagByName(EC_TAG_SEARCH_MIN_SIZE);
			return tag ? tag->GetInt32Data() : 0;
		}
		uint32 MaxSize()
		{
			CECTag *tag =  GetTagByName(EC_TAG_SEARCH_MAX_SIZE);
			return tag ? tag->GetInt32Data() : 0;
		}
		uint32 Avail()
		{
			CECTag *tag =  GetTagByName(EC_TAG_SEARCH_AVAILABILITY);
			return tag ? tag->GetInt32Data() : 0;
		}
		wxString SearchExt()
		{
			CECTag *tag =  GetTagByName(EC_TAG_SEARCH_EXTENSION);
			return tag ? tag->GetStringData() : wxT("");
		}
		wxString SearchFileType()
		{
			CECTag *tag =  GetTagByName(EC_TAG_SEARCH_FILE_TYPE);
			return tag ? tag->GetStringData() : wxT("");
		}
};

#ifndef EC_REMOTE
class CEC_Tree_Tag : public CECTag {
	public:
		CEC_Tree_Tag(const StatsTreeSiblingIterator& tr);
};
#endif

#endif /* ECSPEACIALTAGS_H */
