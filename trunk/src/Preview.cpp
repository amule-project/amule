// This file is part of the aMule Project
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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

// Preview.cpp : implementation file
//

#include <stdio.h>
#include "Preview.h"		// Interface declarations.


// CPreview

//IMPLEMENT_DYNCREATE(CPreviewThread, CWinThread)

CPreviewThread::CPreviewThread()
{
}

CPreviewThread::~CPreviewThread()
{
}

wxThread::ExitCode CPreviewThread::Entry(){
#if 0
	assert (m_pPartfile) ;
	CFile* srcFile = 0;
	CFile destFile;
	/*try*/{
		srcFile = m_pPartfile->m_hpartfile.Duplicate();
		uint32 nSize = m_pPartfile->GetFileSize();
		wxString strExtension = CString(strrchr(m_pPartfile->GetFileName(), '.'));
		wxString strPreviewName = CString(theApp.glob_prefs->GetTempDir())+ CString("\\") + CString(m_pPartfile->GetFileName()).Mid(0,5) + CString("_preview") + strExtension;
		bool bFullSized = true;
		if (!strExtension.CompareNoCase(".mpg") || !strExtension.CompareNoCase(".mpeg"))
			bFullSized = false;
		destFile.Open(strPreviewName, CFile::modeWrite | CFile::shareExclusive | CFile::modeCreate);
		srcFile->SeekToBegin();
		if (bFullSized)
			destFile.SetLength(nSize);
		destFile.SeekToBegin();
		BYTE abyBuffer[4096];
		uint32 nRead;
		while (destFile.GetPosition()+4096 < PARTSIZE*2){
			nRead = srcFile->Read(abyBuffer,4096);
			destFile.Write(abyBuffer,nRead);
		}
		srcFile->Seek(-(PARTSIZE*2),CFile::end);
		uint32 nToGo =PARTSIZE*2;
		if (bFullSized)
			destFile.Seek(-(PARTSIZE*2),CFile::end);
		do{
			nRead = (nToGo - 4096 < 1)? nToGo:4096;
			nToGo -= nRead;
			nRead = srcFile->Read(abyBuffer,4096);
			destFile.Write(abyBuffer,nRead);
		}
		while (nToGo);
		destFile.Close();
		srcFile->Close();
		m_pPartfile->m_bPreviewing = false;

		SHELLEXECUTEINFO SE;
		memset(&SE,0,sizeof(SE));
		SE.fMask = SEE_MASK_NOCLOSEPROCESS ;
		SE.lpVerb = "open";
		SE.lpFile = strPreviewName.GetBuffer();
		SE.nShow = SW_SHOW;
		SE.cbSize = sizeof(SE);
		ShellExecuteEx(&SE);
		if (SE.hProcess){
			WaitForSingleObject(SE.hProcess, INFINITE);
			CloseHandle(SE.hProcess);
		}
		CFile::Remove(strPreviewName.GetBuffer());
	}	
	catch(CFileException* error){
		m_pPartfile->m_bPreviewing = false;
		if (srcFile->m_hFile != INVALID_HANDLE_VALUE)
			srcFile->Close();
		if (destFile.m_hFile != INVALID_HANDLE_VALUE)
			destFile.Close();
		error->Delete();	//mf
	}
	if (srcFile)
		delete srcFile;
	AfxEndThread(0,true);
#endif
	printf("todo. previewthread\n");
	return 0;
}

void CPreviewThread::SetValues(CPartFile* pPartFile){
	m_pPartfile = pPartFile;
}

#if 0
BEGIN_MESSAGE_MAP(CPreviewThread, CWinThread)
END_MESSAGE_MAP()
#endif

// CPreview message handlers
