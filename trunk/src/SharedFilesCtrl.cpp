//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "SharedFilesCtrl.h"	// Interface declarations

#include <common/MenuIDs.h>

#include "muuli_wdr.h"			// Needed for ID_SHFILELIST
#include "SharedFilesWnd.h"		// Needed for CSharedFilesWnd
#include "amuleDlg.h"			// Needed for CamuleDlg
#include "CommentDialog.h"		// Needed for CCommentDialog
#include "PartFile.h"			// Needed for CPartFile
#include "SharedFileList.h"		// Needed for CKnownFileMap
#include "amule.h"				// Needed for theApp
#include "ServerConnect.h"		// Needed for CServerConnect
#include "Preferences.h"		// Needed for thePrefs
#include "BarShader.h"			// Needed for CBarShader
#include "DataToText.h"			// Needed for PriorityToStr
#include "GuiEvents.h"			// Needed for CoreNotify_*
#include "MuleCollection.h"		// Needed for CMuleCollection
#include "DownloadQueue.h"		// Needed for CDownloadQueue
#include "TransferWnd.h"		// Needed for CTransferWnd


BEGIN_EVENT_TABLE(CSharedFilesCtrl,CMuleListCtrl)
	EVT_LIST_ITEM_RIGHT_CLICK(-1, CSharedFilesCtrl::OnRightClick)

	EVT_MENU( MP_PRIOVERYLOW,	CSharedFilesCtrl::OnSetPriority )
	EVT_MENU( MP_PRIOLOW,		CSharedFilesCtrl::OnSetPriority )
	EVT_MENU( MP_PRIONORMAL,	CSharedFilesCtrl::OnSetPriority )
	EVT_MENU( MP_PRIOHIGH,		CSharedFilesCtrl::OnSetPriority )
	EVT_MENU( MP_PRIOVERYHIGH,	CSharedFilesCtrl::OnSetPriority )
	EVT_MENU( MP_POWERSHARE,	CSharedFilesCtrl::OnSetPriority )
	EVT_MENU( MP_PRIOAUTO,		CSharedFilesCtrl::OnSetPriorityAuto )

	EVT_MENU( MP_CMT,			CSharedFilesCtrl::OnEditComment )
	EVT_MENU( MP_ADDCOLLECTION,		CSharedFilesCtrl::OnAddCollection )
	EVT_MENU( MP_GETMAGNETLINK,		CSharedFilesCtrl::OnCreateURI )
	EVT_MENU( MP_GETED2KLINK,				CSharedFilesCtrl::OnCreateURI )
	EVT_MENU( MP_GETSOURCEED2KLINK,			CSharedFilesCtrl::OnCreateURI )
	EVT_MENU( MP_GETCRYPTSOURCEDED2KLINK,			CSharedFilesCtrl::OnCreateURI )
	EVT_MENU( MP_GETHOSTNAMESOURCEED2KLINK,	CSharedFilesCtrl::OnCreateURI )
	EVT_MENU( MP_GETHOSTNAMECRYPTSOURCEED2KLINK,			CSharedFilesCtrl::OnCreateURI )	
	EVT_MENU( MP_GETAICHED2KLINK,	CSharedFilesCtrl::OnCreateURI )
	EVT_MENU( MP_RENAME,		CSharedFilesCtrl::OnRename )
	EVT_MENU( MP_WS,		CSharedFilesCtrl::OnGetFeedback )


	EVT_CHAR( CSharedFilesCtrl::OnKeyPressed )
END_EVENT_TABLE()

enum SharedFilesListColumns {
	ID_SHARED_COL_NAME = 0,
	ID_SHARED_COL_SIZE,
	ID_SHARED_COL_TYPE,
	ID_SHARED_COL_PRIO,
	ID_SHARED_COL_ID,
	ID_SHARED_COL_REQ,
	ID_SHARED_COL_AREQ,
	ID_SHARED_COL_TRA,
	ID_SHARED_COL_RTIO,
	ID_SHARED_COL_PART,
	ID_SHARED_COL_CMPL,
	ID_SHARED_COL_PATH
};


CSharedFilesCtrl::CSharedFilesCtrl(wxWindow* parent, int id, const wxPoint& pos, wxSize size, int flags)
	: CMuleListCtrl(parent, id, pos, size, flags | wxLC_OWNERDRAW )
{
	// Setting the sorter function.
	SetSortFunc( SortProc );

	// Set the table-name (for loading and saving preferences).
	SetTableName( wxT("Shared") );

	m_menu=NULL;

	InsertColumn(ID_SHARED_COL_NAME, _("File Name"),		wxLIST_FORMAT_LEFT, 250, wxT("N") );
	InsertColumn(ID_SHARED_COL_SIZE, _("Size"),			wxLIST_FORMAT_LEFT, 100, wxT("Z") );
	InsertColumn(ID_SHARED_COL_TYPE, _("Type"),			wxLIST_FORMAT_LEFT,  50, wxT("Y") );
	InsertColumn(ID_SHARED_COL_PRIO, _("Priority"),			wxLIST_FORMAT_LEFT,  70, wxT("p") );
	InsertColumn(ID_SHARED_COL_ID,   _("FileID"),			wxLIST_FORMAT_LEFT, 220, wxT("I") );
	InsertColumn(ID_SHARED_COL_REQ,  _("Requests"),			wxLIST_FORMAT_LEFT, 100, wxT("Q") );
	InsertColumn(ID_SHARED_COL_AREQ, _("Accepted Requests"),	wxLIST_FORMAT_LEFT, 100, wxT("A") );
	InsertColumn(ID_SHARED_COL_TRA,  _("Transferred Data"),		wxLIST_FORMAT_LEFT, 120, wxT("T") );
	InsertColumn(ID_SHARED_COL_RTIO, _("Share Ratio"),		wxLIST_FORMAT_LEFT, 100, wxT("R") );
	InsertColumn(ID_SHARED_COL_PART, _("Obtained Parts"),		wxLIST_FORMAT_LEFT, 120, wxT("P") );
	InsertColumn(ID_SHARED_COL_CMPL, _("Complete Sources"),		wxLIST_FORMAT_LEFT, 120, wxT("C") );
	InsertColumn(ID_SHARED_COL_PATH, _("Directory Path"),		wxLIST_FORMAT_LEFT, 220, wxT("D") );

	LoadSettings();
}


wxString CSharedFilesCtrl::GetOldColumnOrder() const
{
	return wxT("N,Z,Y,p,I,Q,A,T,R,P,C,D");
}


CSharedFilesCtrl::~CSharedFilesCtrl()
{
}


void CSharedFilesCtrl::OnRightClick(wxListEvent& event)
{
	long item_hit = CheckSelection(event);

	if ( (m_menu == NULL) && (item_hit != -1)) {
		m_menu = new wxMenu(_("Shared Files"));
		wxMenu* prioMenu = new wxMenu();
		prioMenu->AppendCheckItem(MP_PRIOVERYLOW, _("Very low"));
		prioMenu->AppendCheckItem(MP_PRIOLOW, _("Low"));
		prioMenu->AppendCheckItem(MP_PRIONORMAL, _("Normal"));
		prioMenu->AppendCheckItem(MP_PRIOHIGH, _("High"));
		prioMenu->AppendCheckItem(MP_PRIOVERYHIGH, _("Very High"));
		prioMenu->AppendCheckItem(MP_POWERSHARE, _("Release"));
		prioMenu->AppendCheckItem(MP_PRIOAUTO, _("Auto"));

		m_menu->Append(0,_("Priority"),prioMenu);
		m_menu->AppendSeparator();

		CKnownFile* file = (CKnownFile*)GetItemData(item_hit);
		if (file->GetFileComment().IsEmpty() && !file->GetFileRating()) {
			m_menu->Append(MP_CMT, _("Add Comment/Rating"));
		} else {
			m_menu->Append(MP_CMT, _("Edit Comment/Rating"));
		}
		
		m_menu->AppendSeparator();
		m_menu->Append(MP_RENAME, _("Rename"));
		m_menu->AppendSeparator();

		if (file->GetFileName().GetExt() == wxT("emulecollection")) {
			m_menu->Append( MP_ADDCOLLECTION, _("Add files in collection to transfer list"));
			m_menu->AppendSeparator();
		}
		m_menu->Append(MP_GETMAGNETLINK,_("Copy magnet &URI to clipboard"));
		m_menu->Append(MP_GETED2KLINK,_("Copy eD2k &link to clipboard"));
		m_menu->Append(MP_GETSOURCEED2KLINK,_("Copy eD2k link to clipboard (&Source)"));
		m_menu->Append(MP_GETCRYPTSOURCEDED2KLINK,_("Copy eD2k link to clipboard (Source) (&With Crypt options)"));
		m_menu->Append(MP_GETHOSTNAMESOURCEED2KLINK,_("Copy eD2k link to clipboard (&Hostname)"));
		m_menu->Append(MP_GETHOSTNAMECRYPTSOURCEED2KLINK,_("Copy eD2k link to clipboard (Hostname) (With &Crypt options)"));		
		m_menu->Append(MP_GETAICHED2KLINK,_("Copy eD2k link to clipboard (&AICH info)"));
		m_menu->Append(MP_WS,_("Copy feedback to clipboard"));
		
		m_menu->Enable(MP_GETAICHED2KLINK, file->HasProperAICHHashSet());
		m_menu->Enable(MP_GETHOSTNAMESOURCEED2KLINK, !thePrefs::GetYourHostname().IsEmpty());
		m_menu->Enable(MP_GETHOSTNAMECRYPTSOURCEED2KLINK, !thePrefs::GetYourHostname().IsEmpty());

		int priority = file->IsAutoUpPriority() ? PR_AUTO : file->GetUpPriority();

		prioMenu->Check(MP_PRIOVERYLOW,	priority == PR_VERYLOW);
		prioMenu->Check(MP_PRIOLOW,	priority == PR_LOW);
		prioMenu->Check(MP_PRIONORMAL,	priority == PR_NORMAL);
		prioMenu->Check(MP_PRIOHIGH,	priority == PR_HIGH);
		prioMenu->Check(MP_PRIOVERYHIGH,priority == PR_VERYHIGH);
		prioMenu->Check(MP_POWERSHARE,	priority == PR_POWERSHARE);
		prioMenu->Check(MP_PRIOAUTO,	priority == PR_AUTO);

		PopupMenu( m_menu, event.GetPoint() );

		delete m_menu;

		m_menu = NULL;
	}
}


void CSharedFilesCtrl::OnGetFeedback(wxCommandEvent& WXUNUSED(event))
{
	wxString feed;
	long index = GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	while (index != -1) {
		if (feed.IsEmpty()) {
			feed = CFormat(_("Feedback from: %s (%s)\n\n")) % thePrefs::GetUserNick() % theApp->GetFullMuleVersion();
		} else {
			feed += wxT("\n");
		}
		feed += ((CKnownFile*)GetItemData(index))->GetFeedback();
		index = GetNextItem(index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	}

	if (!feed.IsEmpty()) {
		theApp->CopyTextToClipboard(feed);
	}
}


#ifndef CLIENT_GUI
void CSharedFilesCtrl::ShowFileList()
{
	Freeze();
	DeleteAllItems();

	std::vector<CKnownFile*> files;
	theApp->sharedfiles->CopyFileList(files);
	for (unsigned i = 0; i < files.size(); ++i) {
		DoShowFile(files[i], true);
	}

	SortList();
	ShowFilesCount();
	
	Thaw();
}
#endif


void CSharedFilesCtrl::RemoveFile(CKnownFile *toRemove)
{
	long index = FindItem( -1, reinterpret_cast<wxUIntPtr>(toRemove) );
	
	if ( index != -1 ) {
		DeleteItem( index );
		
		ShowFilesCount();
	}
}


void CSharedFilesCtrl::ShowFile(CKnownFile* file)
{
	DoShowFile(file, false);
}


void CSharedFilesCtrl::DoShowFile(CKnownFile* file, bool batch)
{
	wxUIntPtr ptr = reinterpret_cast<wxUIntPtr>(file);
	if ((!batch) && (FindItem(-1, ptr) > -1)) {
		return;
	}
	
	const long insertPos = (batch ? GetItemCount() : GetInsertPos(ptr));

	long newitem = InsertItem(insertPos, wxEmptyString);
	SetItemPtrData( newitem, ptr );

	if (!batch) {	
		ShowFilesCount();
	}
}

void CSharedFilesCtrl::OnSetPriority( wxCommandEvent& event )
{
	int priority = 0;

	switch ( event.GetId() ) {
		case MP_PRIOVERYLOW:	priority = PR_VERYLOW;	break;
		case MP_PRIOLOW:		priority = PR_LOW;		break;
		case MP_PRIONORMAL:		priority = PR_NORMAL;	break;
		case MP_PRIOHIGH:		priority = PR_HIGH;		break;
		case MP_PRIOVERYHIGH:	priority = PR_VERYHIGH;	break;
		case MP_POWERSHARE:		priority = PR_POWERSHARE; break;
	}

	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	while( index != -1 ) {
		CKnownFile* file = (CKnownFile*)GetItemData( index );
		CoreNotify_KnownFile_Up_Prio_Set( file, priority );

		RefreshItem( index );

		index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}
}


void CSharedFilesCtrl::OnSetPriorityAuto( wxCommandEvent& WXUNUSED(event) )
{
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	while( index != -1 ) {
		CKnownFile* file = (CKnownFile*)GetItemData( index );
		CoreNotify_KnownFile_Up_Prio_Auto(file);

		RefreshItem( index );

		index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}
}


void CSharedFilesCtrl::OnCreateURI( wxCommandEvent& event )
{
	wxString URIs;

	if ( event.GetId() == MP_GETSOURCEED2KLINK || event.GetId() == MP_GETCRYPTSOURCEDED2KLINK) {
		if ( !(	(theApp->IsConnectedED2K() && !theApp->serverconnect->IsLowID())
				|| (theApp->IsConnectedKad() && !theApp->IsFirewalledKad() ))) {
			wxMessageBox(_("You need a HighID to create a valid sourcelink"), _("WARNING"), wxOK | wxICON_ERROR, this);
			return;
		}
	}

	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

	while( index != -1 ) {
		CKnownFile* file = (CKnownFile*)GetItemData( index );

		switch ( event.GetId() ) {
			case MP_GETMAGNETLINK:				URIs += theApp->CreateMagnetLink( file ) + wxT("\n");				break;
			case MP_GETED2KLINK:				URIs += theApp->CreateED2kLink( file ) + wxT("\n");					break;
			case MP_GETSOURCEED2KLINK:			URIs += theApp->CreateED2kLink( file , true) + wxT("\n");			break;
			case MP_GETCRYPTSOURCEDED2KLINK:			URIs += theApp->CreateED2kLink( file , true, false, true) + wxT("\n");			break;
			case MP_GETHOSTNAMESOURCEED2KLINK:	URIs += theApp->CreateED2kLink( file , true, true) + wxT("\n");	break;
			case MP_GETHOSTNAMECRYPTSOURCEED2KLINK:			URIs += theApp->CreateED2kLink( file, true, true, true ) + wxT("\n");			break;				
			case MP_GETAICHED2KLINK:			URIs += theApp->CreateED2kAICHLink( file ) + wxT("\n");				break;
		}

		index = GetNextItem( index, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	}

	if ( !URIs.IsEmpty() ) {	
		theApp->CopyTextToClipboard( URIs.RemoveLast() );
	}
}


void CSharedFilesCtrl::OnEditComment( wxCommandEvent& WXUNUSED(event) )
{
	long index = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	
	if ( index != -1 ) {
		CKnownFile* file = (CKnownFile*)GetItemData( index );

		CCommentDialog dialog( this, file );
	
		dialog.ShowModal();
	}
}


int CSharedFilesCtrl::SortProc(wxUIntPtr item1, wxUIntPtr item2, long sortData)
{
	CKnownFile* file1 = (CKnownFile*)item1;
	CKnownFile* file2 = (CKnownFile*)item2;

	int mod = (sortData & CMuleListCtrl::SORT_DES) ? -1 : 1;
	bool altSorting = (sortData & CMuleListCtrl::SORT_ALT) > 0;

	switch (sortData & CMuleListCtrl::COLUMN_MASK) {
		// Sort by filename.
		case  ID_SHARED_COL_NAME:
			return mod * CmpAny(file1->GetFileName(), file2->GetFileName());
		
		// Sort by filesize.
		case  ID_SHARED_COL_SIZE:
			return mod * CmpAny( file1->GetFileSize(), file2->GetFileSize() );

		// Sort by filetype.
		case  ID_SHARED_COL_TYPE:
			return mod * GetFiletypeByName(file1->GetFileName()).CmpNoCase(GetFiletypeByName( file2->GetFileName()) );

		// Sort by priority.
		case  ID_SHARED_COL_PRIO: {
			int8 prioA = file1->GetUpPriority();
			int8 prioB = file2->GetUpPriority();

			// Work-around for PR_VERYLOW which has value 4. See KnownFile.h for that stupidity ...
			return mod * CmpAny( ( prioB != PR_VERYLOW ? prioB : -1 ), ( prioA != PR_VERYLOW ? prioA : -1 ) );
		}

		// Sort by fileID.
		case  ID_SHARED_COL_ID:
			return mod * file1->GetFileHash().Encode().Cmp( file2->GetFileHash().Encode() );

		// Sort by Requests this session.
		case  ID_SHARED_COL_REQ:
			if (altSorting) {
				return mod * CmpAny( file1->statistic.GetAllTimeRequests(), file2->statistic.GetAllTimeRequests() );
			} else {
				return mod * CmpAny( file1->statistic.GetRequests(), file2->statistic.GetRequests() );
			}

		// Sort by accepted requests. Ascending.
		case  ID_SHARED_COL_AREQ:
			if (altSorting) {
				return mod * CmpAny( file1->statistic.GetAllTimeAccepts(), file2->statistic.GetAllTimeAccepts() );
			} else {
				return mod * CmpAny( file1->statistic.GetAccepts(), file2->statistic.GetAccepts() );
			}

		// Sort by transferred. Ascending.
		case  ID_SHARED_COL_TRA:
			if (altSorting) {
				return mod * CmpAny( file1->statistic.GetAllTimeTransferred(), file2->statistic.GetAllTimeTransferred() );
			} else {
				return mod * CmpAny( file1->statistic.GetTransferred(), file2->statistic.GetTransferred() );
			}

		// Sort by Share Ratio. Ascending.
		case  ID_SHARED_COL_RTIO:
			return mod * CmpAny( (double)file1->statistic.GetAllTimeTransferred() / file1->GetFileSize(),
					(double)file2->statistic.GetAllTimeTransferred() / file2->GetFileSize() );

		// Complete sources asc
		case ID_SHARED_COL_CMPL:
			return mod * CmpAny( file1->m_nCompleteSourcesCount, file2->m_nCompleteSourcesCount );

		// Folders ascending
		case ID_SHARED_COL_PATH: {
			if ( file1->IsPartFile() && file2->IsPartFile() )
				return mod *  0;
			if ( file1->IsPartFile() )
				return mod * -1;
			if ( file2->IsPartFile() )
				return mod *  1;

			return mod * CmpAny(file1->GetFilePath(), file2->GetFilePath());
		}
		
		default:
			return 0;
	}
}


void CSharedFilesCtrl::UpdateItem(CKnownFile* toupdate)
{
	long result = FindItem( -1, reinterpret_cast<wxUIntPtr>(toupdate) );
	
	if ( result > -1 ) {
		RefreshItem(result);

		if ( GetItemState( result, wxLIST_STATE_SELECTED ) ) {
			theApp->amuledlg->m_sharedfileswnd->SelectionUpdated();
		}
	}
}


void CSharedFilesCtrl::ShowFilesCount()
{
	wxStaticText* label = CastByName( wxT("sharedFilesLabel"), GetParent(), wxStaticText );
	
	label->SetLabel(CFormat(_("Shared Files (%i)")) % GetItemCount());
	label->GetParent()->Layout();
	// If file list was updated, the "selection" is involved too, if we chose to show clients for all files.
	// So update client list here too.
	theApp->amuledlg->m_sharedfileswnd->SelectionUpdated();
}


void CSharedFilesCtrl::OnDrawItem( int item, wxDC* dc, const wxRect& rect, const wxRect& rectHL, bool highlighted )
{
	CKnownFile *file = (CKnownFile*)GetItemData(item);
	wxASSERT( file );

	if ( highlighted ) {
		CMuleColour newcol(GetFocus() ? wxSYS_COLOUR_HIGHLIGHT : wxSYS_COLOUR_BTNSHADOW);	
		dc->SetBackground(newcol.Blend(125).GetBrush());
		dc->SetTextForeground( CMuleColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
		// The second blending goes over the first one.
		dc->SetPen(newcol.Blend(65).GetPen());
	} else {
		dc->SetBackground( CMuleColour(wxSYS_COLOUR_LISTBOX).GetBrush() );
		dc->SetTextForeground(CMuleColour(wxSYS_COLOUR_WINDOWTEXT));
		dc->SetPen(*wxTRANSPARENT_PEN);
	}
	
	dc->SetBrush(dc->GetBackground());
	dc->DrawRectangle(rectHL);
	dc->SetPen(*wxTRANSPARENT_PEN);

	// Offset based on the height of the fonts
	const int textVOffset = ( rect.GetHeight() - dc->GetCharHeight() ) / 2;
	// Empty space to each side of a column
	const int SPARE_PIXELS_HORZ	= 4;

	// The leftmost position of the current column
	int columnLeft = 0;
	
	for ( int i = 0; i < GetColumnCount(); ++i ) {
		const int columnWidth = GetColumnWidth(i);

		if (columnWidth > 2*SPARE_PIXELS_HORZ) {
			wxRect columnRect(
				columnLeft + SPARE_PIXELS_HORZ, rect.y,
				columnWidth - 2 * SPARE_PIXELS_HORZ, rect.height);
			
			wxDCClipper clipper(*dc, columnRect);
			
			wxString textBuffer;
			switch ( i ) {
				case ID_SHARED_COL_NAME:
					textBuffer = file->GetFileName().GetPrintable();

					if (file->GetFileRating() || file->GetFileComment().Length()) {
						int image = Client_CommentOnly_Smiley;
						if (file->GetFileRating()) {
							image = Client_InvalidRating_Smiley + file->GetFileRating() - 1;
						}	
							
						wxASSERT(image >= Client_InvalidRating_Smiley);
						wxASSERT(image <= Client_CommentOnly_Smiley);

						int imgWidth = 16;
						
						theApp->amuledlg->m_imagelist.Draw(image, *dc, columnRect.x,
								columnRect.y + 1, wxIMAGELIST_DRAW_TRANSPARENT);

						// Move the text to the right
						columnRect.x += (imgWidth + 4);
					}

					break;
				
				case ID_SHARED_COL_SIZE:
					textBuffer = CastItoXBytes(file->GetFileSize());
					break;

				case ID_SHARED_COL_TYPE:
					textBuffer = GetFiletypeByName(file->GetFileName());
					break;

				case ID_SHARED_COL_PRIO:
					textBuffer = PriorityToStr(file->GetUpPriority(), file->IsAutoUpPriority());
					break;

				case ID_SHARED_COL_ID:
					textBuffer = file->GetFileHash().Encode();
					break;
				
				case ID_SHARED_COL_REQ:
					textBuffer = CFormat(wxT("%u (%u)"))
							% file->statistic.GetRequests()
							% file->statistic.GetAllTimeRequests();
					break;

				case ID_SHARED_COL_AREQ:
					textBuffer = CFormat(wxT("%u (%u)"))
							% file->statistic.GetAccepts()
							% file->statistic.GetAllTimeAccepts();
					break;

				case ID_SHARED_COL_TRA:
					textBuffer = CastItoXBytes(file->statistic.GetTransferred())
						+ wxT(" (") + CastItoXBytes(file->statistic.GetAllTimeTransferred()) + wxT(")");
					break;
					
				case ID_SHARED_COL_RTIO:
					textBuffer = CFormat(wxT("%.2f")) %	((double)file->statistic.GetAllTimeTransferred() / file->GetFileSize());
					break;
				
				case ID_SHARED_COL_PART:
					if ( file->GetPartCount() ) {
						wxRect barRect(columnRect.x, columnRect. y + 1, 
							columnRect.width, columnRect.height - 2);
						
						DrawAvailabilityBar(file, dc, barRect);
					}
					break;
				
				case ID_SHARED_COL_CMPL:
					if ( file->m_nCompleteSourcesCountLo == 0 ) {
						if ( file->m_nCompleteSourcesCountHi ) {
							textBuffer = CFormat(wxT("< %u")) % file->m_nCompleteSourcesCountHi;
						} else {
							textBuffer = wxT("0");
						}
					} else if (file->m_nCompleteSourcesCountLo == file->m_nCompleteSourcesCountHi) {
						textBuffer = CFormat(wxT("%u")) % file->m_nCompleteSourcesCountLo;
					} else {
						textBuffer = CFormat(wxT("%u - %u")) % file->m_nCompleteSourcesCountLo % file->m_nCompleteSourcesCountHi;
					}
					
					break;				
				
				case ID_SHARED_COL_PATH:
					if ( file->IsPartFile() ) {
						textBuffer = _("[PartFile]");
					} else {
						textBuffer = file->GetFilePath().GetPrintable();
					}
			}

			if (!textBuffer.IsEmpty()) {
				dc->DrawText(textBuffer, columnRect.x, columnRect.y + textVOffset);
			}
		}

		// Move to the next column
		columnLeft += columnWidth;
	}
}


wxString CSharedFilesCtrl::GetTTSText(unsigned item) const
{
	return reinterpret_cast<CKnownFile*>(GetItemData(item))->GetFileName().GetPrintable();
}


bool CSharedFilesCtrl::AltSortAllowed(unsigned column) const
{
	switch ( column ) {
		case ID_SHARED_COL_REQ:
		case ID_SHARED_COL_AREQ:
		case ID_SHARED_COL_TRA:
			return true;

		default:
			return false;
	}
}


void CSharedFilesCtrl::DrawAvailabilityBar(CKnownFile* file, wxDC* dc, const wxRect& rect ) const
{
	// Reference to the availability list
	const ArrayOfUInts16& list = file->IsPartFile() ?
		((CPartFile*)file)->m_SrcpartFrequency :
		file->m_AvailPartFrequency;
	wxPen   old_pen   = dc->GetPen();
	wxBrush old_brush = dc->GetBrush();
	bool bFlat = thePrefs::UseFlatBar();

	wxRect barRect = rect;
	if (!bFlat) { // round bar has a black border, the bar itself is 1 pixel less on each border
		barRect.x ++;
		barRect.y ++;
		barRect.height -= 2;
		barRect.width -= 2;
	}
	static CBarShader s_ChunkBar;
	s_ChunkBar.SetFileSize( file->GetFileSize() );
	s_ChunkBar.SetHeight( barRect.GetHeight() );
	s_ChunkBar.SetWidth( barRect.GetWidth() );
	s_ChunkBar.Set3dDepth( CPreferences::Get3DDepth() );
	uint64 end = 0;
	for ( unsigned int i = 0; i < list.size(); ++i ) {
		uint64 start = PARTSIZE * static_cast<uint64>(i);
		end   = PARTSIZE * static_cast<uint64>(i + 1);
		s_ChunkBar.FillRange(start, end, CMuleColour(list[i] ? 0 : 255, list[i] ? ((210-(22*( list[i] - 1 ) ) < 0) ? 0 : (210-(22*( list[i] - 1 ) ))) : 0, list[i] ? 255 : 0));
	}
	s_ChunkBar.FillRange(end + 1, file->GetFileSize() - 1, CMuleColour(255, 0, 0));
	s_ChunkBar.Draw(dc, barRect.x, barRect.y, bFlat); 

	if (!bFlat) {
		// Draw black border
		dc->SetPen( *wxBLACK_PEN );
		dc->SetBrush( *wxTRANSPARENT_BRUSH );
		dc->DrawRectangle(rect);
	}

	dc->SetPen( old_pen );
	dc->SetBrush( old_brush );
}

void CSharedFilesCtrl::OnRename( wxCommandEvent& WXUNUSED(event) )
{
	int item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if ( item != -1 ) {
		CKnownFile* file = (CKnownFile*)GetItemData(item);

		wxString strNewName = ::wxGetTextFromUser(
			_("Enter new name for this file:"),
			_("File rename"), file->GetFileName().GetPrintable());

		CPath newName = CPath(strNewName);
		if (newName.IsOk() && (newName != file->GetFileName())) {
			theApp->sharedfiles->RenameFile(file, newName);
		}
	}
}


void CSharedFilesCtrl::OnKeyPressed( wxKeyEvent& event )
{
	if (event.GetKeyCode() == WXK_F2) {
		wxCommandEvent evt;
		OnRename(evt);
		
		return;
	}
	event.Skip();
}


void CSharedFilesCtrl::OnAddCollection( wxCommandEvent& WXUNUSED(evt) )
{
	int item = GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );
	if (item != -1) {
		CKnownFile *file = (CKnownFile*)GetItemData(item);
		wxString CollectionFile = file->GetFilePath().JoinPaths(file->GetFileName()).GetRaw();
		CMuleCollection my_collection;
		if (my_collection.Open( (std::string)CollectionFile.mb_str() )) {
//#warning This is probably not working on Unicode
			for (size_t e = 0; e < my_collection.GetFileCount(); ++e) {
				theApp->downloadqueue->AddLink(
					wxString(my_collection.GetEd2kLink(e).c_str(), wxConvUTF8));
			}
				
		}
	}
}

// File_checked_for_headers
