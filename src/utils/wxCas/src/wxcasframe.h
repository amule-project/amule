/////////////////////////////////////////////////////////////////////////////
// Name:        wxcasframe.h
// Purpose:     wxCas main frame
// Author:      ThePolish <thepolish@vipmail.ru>
// Created:     2004/04/15
// Modified by:
// Copyright:   (c) ThePolish <thepolish@vipmail.ru>
// Licence:     GPL
// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
/////////////////////////////////////////////////////////////////////////////

#ifndef _WXCASFRAME_H
#define _WXCASFRAME_H

#ifdef __GNUG__
#pragma interface "wxcasframe.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "wxcascanvas.h"

// wxCas Frame
class WxCasFrame:public wxFrame
{
      public:

	//Constructor
	WxCasFrame (const wxChar * title);

	//Destructor
	 ~WxCasFrame ();

      protected:

	void SetFromDefaultAmuleFile ();

	void OnBarRefresh (wxCommandEvent & event);
	void OnBarAbout (wxCommandEvent & event);
	void OnBarSave (wxCommandEvent & event);
	void OnBarPrint (wxCommandEvent & event);

	  DECLARE_EVENT_TABLE ();

      private:
	  wxToolBar * m_toolbar;

	wxBoxSizer *m_frameVBox;
	wxBoxSizer *m_sigPanelVBox;

	wxPanel *m_sigPanel;

	wxStaticBox *m_sigPanelSBox;
	wxStaticBoxSizer *m_sigPanelSBoxSizer;

	wxStaticText *label_1;
	wxStaticText *label_2;
	wxStaticText *label_3;
	wxStaticText *label_4;
	wxStaticText *label_5;
	wxStaticText *label_6;

	WxCasCanvas *m_imgPanel;
	wxBitmap *m_amule_xpm;

	enum
	{
		ID_BAR_REFRESH = 1000,
		ID_BAR_ABOUT = 1001,
		ID_BAR_PRINT = 1002,
		ID_BAR_SAVE = 1003
	};
};

#endif /* _WXCASFRAME_H */
