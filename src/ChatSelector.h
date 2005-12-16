//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2005 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef CHATSELECTOR_H
#define CHATSELECTOR_H

#include <wx/defs.h>			// Needed before any other wx/*.h
#include <wx/imaglist.h>		// Needed for wxImageList (at least on wx2.5)
#include "MuleTextCtrl.h" 
#include "MuleNotebook.h"
#include "Types.h"				// Needed for uint16
#include "Color.h"				// Needed for COLORREF

class CUpDownClient;
class CFriend;
class CDlgFriend;


/**
 * This class is used to display chat sessions.
 */
class CChatSession : public CMuleTextCtrl
{
public:
	CChatSession(wxWindow *parent, wxWindowID id = -1, const wxString& value = wxT(""), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxValidator& validator = wxDefaultValidator, const wxString& name = wxTextCtrlNameStr );
	~CChatSession();

	uint64	m_client_id;
	bool	m_active;
	
	/**
	 * Appends the specified text.
	 *
	 * @param text The text to add.
	 * @param style The style of the new text.
	 * @param newline If a newline should be added to the end of the line.
	 *
	 * If newline is false, then no newlines will be at the end of added text,
	 * even if the passed string ends with newlines. Multiline strings are
	 * broken into indivudual lines and each are timestamped with the same date.
	 */
	void AddText( const wxString& text, const wxTextAttr& style, bool newline = true );
};


class CChatSelector : public CMuleNotebook
{
public:
	CChatSelector(wxWindow* parent, wxWindowID id, const wxPoint& pos, wxSize siz, long style);
	virtual			~CChatSelector() {};
	CChatSession*	StartSession(uint64 client_id, const wxString& client_name, bool show = true);
	void			EndSession(uint64 client_id = 0);
	CChatSession*	GetPageByClientID(uint64 client_id);
	int				GetTabByClientID(uint64 client_id);
	bool			ProcessMessage(uint64 sender_id, const wxString& message);
	bool			SendMessage(const wxString& message, const wxString& client_name = wxEmptyString, uint64 to_id = 0);
	void			ConnectionResult(bool success, const wxString& message, uint64 id);
	void			RefreshFriend(uint64 toupdate_id, const wxString& new_name);
};

#endif
