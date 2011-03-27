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
#include "SourceListCtrl.h"
#include "KnownFile.h"

static CGenericClientListCtrlColumn s_sources_column_info[] = { 
	{ ColumnUserName,		wxTRANSLATE("User Name"),	260 },
	{ ColumnUserDownloaded,		wxTRANSLATE("Downloaded"),	65 },
	{ ColumnUserSpeedDown,		wxTRANSLATE("Speed"),		65 },
	{ ColumnUserUploaded,		wxTRANSLATE("Uploaded"),	65 },
	{ ColumnUserProgress,		wxTRANSLATE("Available Parts"),	170 },
	{ ColumnUserVersion,		wxTRANSLATE("Version"),		50 },
	{ ColumnUserQueueRankRemote,	wxTRANSLATE("Download Status"),	55 },
	{ ColumnUserOrigin,		wxTRANSLATE("Origin"),		110 },
	{ ColumnUserFileNameDownload,	wxTRANSLATE("Local File Name"),	200 },
	{ ColumnUserFileNameDownloadRemote, wxTRANSLATE("Remote File Name"), 200 }
};

BEGIN_EVENT_TABLE(CSourceListCtrl, CGenericClientListCtrl)
END_EVENT_TABLE()

CSourceListCtrl::CSourceListCtrl(
	wxWindow *parent, wxWindowID winid, const wxPoint& pos, const wxSize& size,
	long style, const wxValidator& validator, const wxString& name )
:
CGenericClientListCtrl( wxT("Sources"), parent, winid, pos, size, style | wxLC_OWNERDRAW, validator, name )
{
	// Setting the sorter function.
	SetSortFunc( SourceSortProc );

	m_columndata.n_columns = sizeof(s_sources_column_info) / sizeof(CGenericClientListCtrlColumn);
	m_columndata.columns = s_sources_column_info;

	InitColumnData();
}

CSourceListCtrl::~CSourceListCtrl()
{
}

int CSourceListCtrl::SourceSortProc(wxUIntPtr param1, wxUIntPtr param2, long sortData)
{
	return CGenericClientListCtrl::SortProc(param1, param2, s_sources_column_info[sortData & CMuleListCtrl::COLUMN_MASK].cid | (sortData & CMuleListCtrl::SORT_DES));
}

void CSourceListCtrl::SetShowSources(CKnownFile * f, bool b) const
{
	f->SetShowSources(b);
}

// File_checked_for_headers
