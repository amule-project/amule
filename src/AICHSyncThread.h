//This file is part of aMule Project
//
// Copyright (c) 2003-2004 Angel Vidal (Kry) ( kry@amule.org )
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
// Copyright (c) 2002-2004 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef __AICHSYNCTHREAD_H__
#define __AICHSYNCTHREAD_H__

#include <wx/thread.h>
#include <wx/object.h>
#include <list>

class CKnownFile;
	
using namespace std;
	
typedef std::list<CKnownFile*> KnownFilePtrList;

/////////////////////////////////////////////////////////////////////////////////////////
///CAICHSyncThread
class CAICHSyncThread : public wxThread
{
	
protected:
	CAICHSyncThread();
public:
	virtual bool InitInstance();
	virtual void*	Entry();

private:
	KnownFilePtrList m_liToHash;
};

#endif // __AICHSYNCTHREAD_H__
