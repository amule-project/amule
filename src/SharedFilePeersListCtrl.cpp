//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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
#include "SharedFilePeersListCtrl.h"
#include "KnownFile.h"		// Do_not_auto_remove

static CGenericClientListCtrlColumn s_sources_column_info[] = { 
	{ ColumnUserName,		wxTRANSLATE("User Name"),	260 },
	{ ColumnUserDownloaded,		wxTRANSLATE("Downloaded"),	65 },
	{ ColumnUserSpeedDown,		wxTRANSLATE("Download Speed"),	65 },
	{ ColumnUserUploaded,		wxTRANSLATE("Uploaded"),	65 },
	{ ColumnUserSpeedUp,		wxTRANSLATE("Upload Speed"),	65 },
	{ ColumnUserAvailable,		wxTRANSLATE("Available Parts"),	170 },
	{ ColumnUserVersion,		wxTRANSLATE("Version"),		50 },
	{ ColumnUserQueueRankLocal,	wxTRANSLATE("Upload status"),	70 },
	{ ColumnUserQueueRankRemote,	wxTRANSLATE("Download status"),	70 },
	{ ColumnUserOrigin,		wxTRANSLATE("Origin"),		110 },
	{ ColumnUserFileNameUpload,	wxTRANSLATE("Local File Name"),	200 }
};

BEGIN_EVENT_TABLE(CSharedFilePeersListCtrl, CGenericClientListCtrl)
END_EVENT_TABLE()

CSharedFilePeersListCtrl::CSharedFilePeersListCtrl(
	wxWindow *parent, wxWindowID winid, const wxPoint& pos, const wxSize& size,
	long style, const wxValidator& validator, const wxString& name )
:
CGenericClientListCtrl( wxT("Peers"), parent, winid, pos, size, style | wxLC_OWNERDRAW, validator, name )
{
	// Setting the sorter function.
	SetSortFunc( SourceSortProc );

	m_columndata.n_columns = sizeof(s_sources_column_info) / sizeof(CGenericClientListCtrlColumn);
	m_columndata.columns = s_sources_column_info;

	InitColumnData();
}

CSharedFilePeersListCtrl::~CSharedFilePeersListCtrl()
{
}

int CSharedFilePeersListCtrl::SourceSortProc(wxUIntPtr param1, wxUIntPtr param2, long sortData)
{
	return CGenericClientListCtrl::SortProc(param1, param2, s_sources_column_info[sortData & CMuleListCtrl::COLUMN_MASK].cid | (sortData & CMuleListCtrl::SORT_DES));
}

void CSharedFilePeersListCtrl::SetShowSources(CKnownFile * f, bool b) const
{
	f->SetShowPeers(b);
}

// File_checked_for_headers
