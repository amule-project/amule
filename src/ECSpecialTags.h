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

#include "types.h"	// Needed for uint* types
#include <wx/string.h>	// Needed for wxString
#include "ECcodes.h"	// Needed for EC types
#include "ECPacket.h"	// Needed for CECTag

#pragma interface

/*
 * Specific tags for specific requests
 *
 * \note EC remote end does not need to create these packets,
 * only using the getter functions.
 *
 * Regarding this, thos classe are removed from remote build,
 * that have only a constructor.
 */

#ifndef EC_REMOTE
class CServer;
class CKnownFile;

#include "PartFile.h"	// Needed for CPartFile::GetProgressString()
#endif /* ! EC_REMOTE */


#ifndef EC_REMOTE
class CEC_Server_Tag : public CECTag {
 	public:
 		CEC_Server_Tag(CServer *, EC_DETAIL_LEVEL);
};
#endif /* ! EC_REMOTE */


#ifndef EC_REMOTE
class CEC_ConnState_Tag : public CECTag {
 	public:
 		CEC_ConnState_Tag(EC_DETAIL_LEVEL);
};
#endif /* ! EC_REMOTE */


class CEC_PartFile_Tag : public CECTag {
 	public:
		#ifndef EC_REMOTE
 		CEC_PartFile_Tag(CPartFile *file, EC_DETAIL_LEVEL detail_level);
		#else // dummy constructor, to satisfy gcc
		CEC_PartFile_Tag() : CECTag(0, 0, NULL, false) {}	// 0 = invalid tagname
		#endif /* ! EC_REMOTE */
 		
 		uint32		FileID()	{ return GetInt32Data(); }
 		wxString	FileName()	{ return GetTagByName(EC_TAG_PARTFILE_NAME)->GetStringData(); }
 		uint32		SizeFull()	{ return GetTagByName(EC_TAG_PARTFILE_SIZE_FULL)->GetInt32Data(); }
 		uint32		SizeXfer()	{ return GetTagByName(EC_TAG_PARTFILE_SIZE_XFER)->GetInt32Data(); }
  		uint32		SizeDone()	{ return GetTagByName(EC_TAG_PARTFILE_SIZE_DONE)->GetInt32Data(); }
 		wxString	FileEd2kLink()	{ return GetTagByName(EC_TAG_PARTFILE_ED2K_LINK)->GetStringData(); }
 		uint8		FileStatus()	{ return GetTagByName(EC_TAG_PARTFILE_STATUS)->GetInt8Data(); }
  		uint32		SourceCount()	{ return GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT)->GetInt32Data(); }
  		uint32		SourceNotCurrCount()	{ return GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT_NOT_CURRENT)->GetInt32Data(); }
  		uint32		SourceXferCount()	{ return GetTagByName(EC_TAG_PARTFILE_SOURCE_COUNT_XFER)->GetInt32Data(); }
  		uint32		Speed()		{ return GetTagByName(EC_TAG_PARTFILE_SPEED)->GetInt32Data(); }
  		uint32		Prio()		{ return GetTagByName(EC_TAG_PARTFILE_PRIO)->GetInt32Data(); }
  		wxString	PartStatus()	{ return GetTagByName(EC_TAG_PARTFILE_PART_STATUS)->GetStringData(); }
};


#ifndef EC_REMOTE
class CEC_PartStatus_Tag : public CECTag {
 	public:
 		CEC_PartStatus_Tag(CPartFile *file, int statussize) : CECTag(EC_TAG_PARTFILE_PART_STATUS, file->GetProgressString(statussize)) {};
};
#endif /* ! EC_REMOTE */


class CEC_SharedFile_Tag : public CECTag {
	public:
		#ifndef EC_REMOTE
		CEC_SharedFile_Tag(CKnownFile *file, EC_DETAIL_LEVEL detail_level);
		#else // dummy constructor, to satisfy gcc
		CEC_SharedFile_Tag() : CECTag(0, 0, NULL, false) {}	// 0 = invalid tagname
		#endif /* ! EC_REMOTE */
		
 		uint32		FileID()	{ return GetInt32Data(); }
 		wxString	FileName()	{ return GetTagByName(EC_TAG_PARTFILE_NAME)->GetStringData(); }
 		uint32		SizeFull()	{ return GetTagByName(EC_TAG_PARTFILE_SIZE_FULL)->GetInt32Data(); }
  		uint32		Prio()		{ return GetTagByName(EC_TAG_PARTFILE_PRIO)->GetInt32Data(); }
 		wxString	FileEd2kLink()	{ return GetTagByName(EC_TAG_PARTFILE_ED2K_LINK)->GetStringData(); }

 		uint32		GetRequests()	{ return GetTagByName(EC_TAG_KNOWNFILE_REQ_COUNT)->GetInt32Data(); }
 		uint32		GetAllRequests()	{ return GetTagByName(EC_TAG_KNOWNFILE_REQ_COUNT_ALL)->GetInt32Data(); }

 		uint32		GetAccepts()	{ return GetTagByName(EC_TAG_KNOWNFILE_ACCEPT_COUNT)->GetInt32Data(); }
 		uint32		GetAllAccepts()	{ return GetTagByName(EC_TAG_KNOWNFILE_ACCEPT_COUNT_ALL)->GetInt32Data(); }
};

#endif /* ECSPEACIALTAGS_H */
