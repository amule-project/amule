// This file is part of the aMule Project
//
// Copyright (c) 2003-2005 aMule Project ( http://www.amule.org )
// Copyright (C) 2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef PREVIEW_H
#define PREVIEW_H

#if defined(__GNUG__) && !defined(NO_GCC_PRAGMA)
#pragma interface "Preview.h"
#endif

#include <wx/defs.h>		// Needed before any other wx/*.h
#include <wx/thread.h>		// Needed for wxThread

class CPartFile;

// CPreview

class CPreviewThread : public wxThread
{
  //	DECLARE_DYNCREATE(CPreviewThread)

 public:
	CPreviewThread();           // protected constructor used by dynamic creation
	virtual ~CPreviewThread();
	//DECLARE_MESSAGE_MAP()
public:
	virtual	bool	InitInstance() {return true;}
	//virtual int		Run();
	virtual void* Entry();
	void	SetValues(CPartFile* pPartFile);

private:
	CPartFile* m_pPartfile;

};

#endif // PREVIEW_H
