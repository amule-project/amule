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

#ifndef PARTFILECONVERT_H
#if !(defined __need_convertinfo || defined __need_convstatus)
#	define PARTFILECONVERT_H
#endif

#ifndef __have_convstatus
#define __have_convstatus

enum ConvStatus {
	CONV_OK			= 0,
	CONV_QUEUE,
	CONV_INPROGRESS,
	CONV_OUTOFDISKSPACE,
	CONV_PARTMETNOTFOUND,
	CONV_IOERROR,
	CONV_FAILED,
	CONV_BADFORMAT,
	CONV_ALREADYEXISTS
};

#endif /* convstatus */
#undef __need_convstatus

#if !defined __have_convertinfo && (defined __need_convertinfo || defined PARTFILECONVERT_H)
#define __have_convertinfo

#include "Types.h"
#include <common/Path.h>

struct ConvertJob;

struct ConvertInfo {
	unsigned	id;
	CPath		folder;
	CPath		filename;
	wxString	filehash;
	ConvStatus	state;
	uint32_t	size;
	uint32_t	spaceneeded;
	ConvertInfo(ConvertJob *);
};

#endif /* convertinfo */
#undef __need_convertinfo

#ifdef PARTFILECONVERT_H

#include <wx/thread.h>

class CPartFileConvert : private wxThread
{
public:
	static int	ScanFolderToAdd(const CPath& folder, bool deletesource = false);
	static void	ConvertToeMule(const CPath& file, bool deletesource = false);
	static void	StartThread();
	static void	StopThread();

	static void	RemoveJob(unsigned id);
	static void	RetryJob(unsigned id);
	static void	ReaddAllJobs();

private:
	CPartFileConvert() : wxThread(wxTHREAD_DETACHED) {}

	static ConvStatus	performConvertToeMule(const CPath& file);
	virtual ExitCode	Entry();

	static wxMutex			s_mutex;
	static wxThread*		s_convertPfThread;
	static std::list<ConvertJob*>	s_jobs;
	static ConvertJob*		s_pfconverting;
};

#endif

#endif /* PARTFILECONVERT_H */
