#include "MuleTextCtrl.h"
#include <wx/menu.h>
#include <wx/intl.h>
#include <wx/clipbrd.h>

/**
 * These are the IDs used to identify the different menu-items.
 *
 * Please note that I make use of predefined wxIDs for the first two, but not 
 * for Paste. This is because wxMenu poses some restrictions on what can be 
 * done with items using those IDs, and by default, Paste is enabled even if 
 * there's nothing to paste!
 */ 
enum CMTC_Events
{
	//! Cut text, uses provided ID
	CMTCE_Cut	= wxID_CUT,
	//! Copy text, uses privided ID
	CMTCE_Copy 	= wxID_COPY,
	//! Paste text, uses custom ID
	CMTCE_Paste = wxID_HIGHEST + 666,	// Random satanic ID
	//! Clear text, uses custom ID
	CMTCE_Clear,
	//! Select All text, uses custom ID
	CMTCE_SelAll
};


BEGIN_EVENT_TABLE(CMuleTextCtrl, wxTextCtrl)
	EVT_RIGHT_DOWN	(CMuleTextCtrl::OnRightDown)

	EVT_MENU    	(CMTCE_Paste,	CMuleTextCtrl::OnPaste)
	EVT_MENU    	(CMTCE_Clear,	CMuleTextCtrl::OnClear)
	EVT_MENU    	(CMTCE_SelAll,	CMuleTextCtrl::OnSelAll)
END_EVENT_TABLE()


CMuleTextCtrl::CMuleTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name)
 :  wxTextCtrl( parent, id, value, pos, size, style, validator, name)
{
}


void CMuleTextCtrl::OnRightDown( wxMouseEvent& evt )
{
	// If this control doesn't have focus, then set it
	if ( FindFocus() != this )
		SetFocus();

	wxMenu popup_menu;
	
	popup_menu.Append( CMTCE_Cut, _("Cut") );
	popup_menu.Append( CMTCE_Copy, _("Copy") );
	popup_menu.Append( CMTCE_Paste, _("Paste") );
	popup_menu.Append( CMTCE_Clear, _("Clear") );
	
	popup_menu.AppendSeparator();
	
	popup_menu.Append( CMTCE_SelAll, _("Select All") );


	// wxMenu will automatically enable/disable the Cut and Copy items,
	// however, were are a little more pricky about the Paste item than they
	// are, so we enable/disable it on our own, depending on whenever or not
	// there's actually something to paste
	bool canpaste = false;
	if ( CanPaste() ) {
		if ( wxTheClipboard->IsSupported( wxDF_TEXT ) ) {
			wxTextDataObject data;
	 		wxTheClipboard->GetData( data );

			canpaste = data.GetTextLength();
		}
	}


	popup_menu.Enable( CMTCE_Paste,		canpaste );
	popup_menu.Enable( CMTCE_Clear,		IsEditable() && !GetValue().IsEmpty() );

	PopupMenu( &popup_menu, evt.GetX(), evt.GetY() );
}


void CMuleTextCtrl::OnPaste( wxMenuEvent& WXUNUSED(evt) )
{
	Paste();
}


void CMuleTextCtrl::OnSelAll( wxMenuEvent& WXUNUSED(evt) )
{
	// Move the pointer to the front
	SetInsertionPoint( 0 );

	// Selects everything
	SetSelection( -1, -1 );
}


void CMuleTextCtrl::OnClear( wxMenuEvent& WXUNUSED(evt) )
{
	Clear();
}

