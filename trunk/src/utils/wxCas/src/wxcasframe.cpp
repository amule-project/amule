/////////////////////////////////////////////////////////////////////////////
// Name:        wxcasframe.cpp
// Purpose:     wxCas main frame
// Author:      ThePolish <thepolish@vipmail.ru>
// Created:     2004/04/15
// Modified by:
// Copyright:   (c) ThePolish <thepolish@vipmail.ru>
// Licence:     GPL
// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
// Pixmats from aMule http://www.amule.org
/////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "wxcasframe.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "wxcasframe.h"
#include "onlinesig.h"

#include <wx/statline.h>
#include <wx/toolbar.h>

#if defined(__WXGTK__) || defined(__WXMOTIF__) || wxUSE_XPM_IN_MSW
#include "../pixmaps/wxcas.xpm"
#include "../pixmaps/amule.xpm"
#include "../pixmaps/refresh.xpm"
#include "../pixmaps/save.xpm"
#include "../pixmaps/print.xpm"
#include "../pixmaps/about.xpm"
#endif

// Constructor
WxCasFrame::WxCasFrame (const wxChar * title):
wxFrame ((wxFrame *) NULL, -1, title, wxDefaultPosition, wxDefaultSize,
	 wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION)
{
	// Give it an icon
	SetIcon (wxICON (wxcas));

	// Status Bar
	CreateStatusBar ();
	SetStatusText (_("Welcome!"));

	// Frame Vertical Sizer
	m_frameVBox = new wxBoxSizer (wxVERTICAL);

	// Draw and populate panel
	m_sigPanel = new wxPanel (this, -1, wxDefaultPosition,
				  wxDefaultSize, wxTAB_TRAVERSAL,
				  "wxCasPanel");

	// Panel Vertical Sizer
	m_sigPanelSBox = new wxStaticBox (m_sigPanel, -1, "");
	m_sigPanelSBoxSizer =
		new wxStaticBoxSizer (m_sigPanelSBox, wxVERTICAL);

	// Statistics
	OnLineSig *aMuleSig = new OnLineSig ();
	aMuleSig->SetFromDefaultAmuleSig ();

	label_1 = new wxStaticText (m_sigPanel, -1,
				    "aMule " +
				    aMuleSig->GetVersion () +
				    _(" has been running for ") +
				    aMuleSig->GetRunTime ());

	label_2 = new wxStaticText (m_sigPanel, -1,
				    aMuleSig->GetUser () +
				    _(" is on ") +
				    aMuleSig->GetServerName () +
				    " [" + aMuleSig->GetServerIP () +
				    _("] with ") +
				    aMuleSig->GetConnexionIDType ());

	label_3 = new wxStaticText (m_sigPanel, -1,
				    _("Total Download: ") +
				    aMuleSig->GetConvertedTotalDL () +
				    _(", Upload: ") +
				    aMuleSig->GetConvertedTotalUL ());

	label_4 = new wxStaticText (m_sigPanel, -1,
				    _("Session Download: ") +
				    aMuleSig->
				    GetConvertedSessionDL () +
				    _(", Upload: ") +
				    aMuleSig->GetConvertedSessionUL ());

	label_5 = new wxStaticText (m_sigPanel, -1,
				    _("Download: ") +
				    aMuleSig->GetDLRate () +
				    _(" kB/s, Upload: ") +
				    aMuleSig->GetULRate () + _("kB/s"));

	label_6 = new wxStaticText (m_sigPanel, -1,
				    _("Sharing: ") +
				    aMuleSig->GetSharedFiles () +
				    _(" file(s), Clients on queue: ")
				    + aMuleSig->GetQueue ());


	if (aMuleSig->IsRunning ())
	{
		SetStatusText (_("aMule is running"));
	}
	else
	{
		SetStatusText (_("WARNING: aMule is NOT running"));
	}

	delete aMuleSig;

	m_sigPanelSBoxSizer->Add (label_1, 0, wxALL | wxEXPAND, 5);
	m_sigPanelSBoxSizer->Add (label_2, 0, wxALL | wxEXPAND, 5);
	m_sigPanelSBoxSizer->Add (label_3, 0, wxALL | wxEXPAND, 5);
	m_sigPanelSBoxSizer->Add (label_4, 0, wxALL | wxEXPAND, 5);
	m_sigPanelSBoxSizer->Add (label_5, 0, wxALL | wxEXPAND, 5);
	m_sigPanelSBoxSizer->Add (label_6, 0, wxALL | wxEXPAND, 5);

	// Panel Layout
	m_sigPanel->SetSizerAndFit (m_sigPanelSBoxSizer);

	// Add panel to sizer
	m_frameVBox->Add (new wxStaticLine (this, -1), 0, wxALL | wxEXPAND,
			  5);
	m_frameVBox->Add (m_sigPanel, 0, wxALL | wxEXPAND, 10);

	// Add Bitmap
	wxBitmap bitmap (amule_xpm);
	
	wxInt32 h, w;
	m_sigPanel->GetSize (&w, &h);

	m_amule_xpm = new wxBitmap (wxImage (bitmap.ConvertToImage ()).
			       Scale (w, h));

	m_imgPanel = new WxCasCanvas (this, m_amule_xpm);

	m_frameVBox->Add (m_imgPanel, 0, wxALL | wxEXPAND, 10);


// Toolbar
	wxBitmap *toolBarBitmaps[4];

#if USE_XPM_BITMAPS
	toolBarBitmaps[0] = new wxBitmap (refresh_xpm);
	toolBarBitmaps[1] = new wxBitmap (save_xpm);
	toolBarBitmaps[2] = new wxBitmap (print_xpm);
	toolBarBitmaps[3] = new wxBitmap (about_xpm);
#else
	toolBarBitmaps[0] = new wxBITMAP (refresh);
	toolBarBitmaps[1] = new wxBITMAP (save);
	toolBarBitmaps[2] = new wxBITMAP (print);
	toolBarBitmaps[3] = new wxBITMAP (about);
#endif

	m_toolbar =
		new wxToolBar (this, -1, wxDefaultPosition, wxDefaultSize,
			       wxTB_HORIZONTAL | wxTB_FLAT);
	SetToolBar (m_toolbar);

	m_toolbar->AddTool (ID_BAR_REFRESH, "Refresh", *(toolBarBitmaps[0]),
			    _("Refresh Online statistics"));

	m_toolbar->AddTool (ID_BAR_SAVE, "Refresh", *(toolBarBitmaps[1]),
			    _("Save Online statistics image"));

	m_toolbar->AddTool (ID_BAR_PRINT, "Refresh", *(toolBarBitmaps[2]),
			    _("Print Online statistics image"));

	m_toolbar->AddSeparator ();

	m_toolbar->AddTool (ID_BAR_ABOUT, "About", *(toolBarBitmaps[3]),
			    _("About wxCas"));

#ifdef __WXMSW__
	m_toolbar->SetMargins (16, 15);
	//int width = 24;
#else
	m_toolbar->SetMargins (24, 24);
	//int width = 16;
#endif

	m_toolbar->SetMargins (2, 2);
	m_toolbar->Realize ();

	// Frame Layout
	SetAutoLayout (TRUE);
	SetSizerAndFit (m_frameVBox);
}

// Destructor
WxCasFrame::~WxCasFrame ()
{
}

BEGIN_EVENT_TABLE (WxCasFrame, wxFrame)
EVT_TOOL (ID_BAR_REFRESH, WxCasFrame::OnBarRefresh)
EVT_TOOL (ID_BAR_SAVE, WxCasFrame::OnBarSave)
EVT_TOOL (ID_BAR_PRINT, WxCasFrame::OnBarPrint)
EVT_TOOL (ID_BAR_ABOUT, WxCasFrame::OnBarAbout) END_EVENT_TABLE ()
// Refresh button
     void
     WxCasFrame::OnBarRefresh (wxCommandEvent & event)
{
	SetFromDefaultAmuleFile ();
}

// Save button
void
WxCasFrame::OnBarSave (wxCommandEvent & event)
{
	printf ("To be implemented\n");
}

// Print button
void
WxCasFrame::OnBarPrint (wxCommandEvent & event)
{
	printf ("To be implemented\n");
}

// About button
void
WxCasFrame::OnBarAbout (wxCommandEvent & event)
{
	wxMessageBox (_
		      ("wxCas, aMule OnLigne Signature Statistics\n\n"
		       "(c) 2004 ThePolish <thepolish@vipmail.ru>\n\n"
		       "Based on CAS by Pedro de Oliveira <falso@rdk.homeip.net>\n\n"
		       "Distributed under GPL"),
		      _("About wxCas"), wxOK | wxCENTRE | wxICON_INFORMATION);
}

// Accessors
void
WxCasFrame::SetFromDefaultAmuleFile ()
{
	OnLineSig *aMuleSig = new OnLineSig ();
	aMuleSig->SetFromDefaultAmuleSig ();

	label_1->SetLabel ("aMule " +
			   aMuleSig->GetVersion () +
			   _(" has been running for ") +
			   aMuleSig->GetRunTime ());

	label_2->SetLabel (aMuleSig->GetUser () +
			   _(" is on ") +
			   aMuleSig->GetServerName () +
			   " [" + aMuleSig->GetServerIP () +
			   _("] with ") + aMuleSig->GetConnexionIDType ());

	label_3->SetLabel (_("Total Download: ") +
			   aMuleSig->GetConvertedTotalDL () +
			   _(", Upload: ") +
			   aMuleSig->GetConvertedTotalUL ());

	label_4->SetLabel (_("Session Download: ") +
			   aMuleSig->
			   GetConvertedSessionDL () +
			   _(", Upload: ") +
			   aMuleSig->GetConvertedSessionUL ());

	label_5->SetLabel (_("Download: ") +
			   aMuleSig->GetDLRate () +
			   _(" kB/s, Upload: ") +
			   aMuleSig->GetULRate () + _("kB/s"));

	label_6->SetLabel (_("Sharing: ") +
			   aMuleSig->GetSharedFiles () +
			   _(" file(s), Clients on queue: ")
			   + aMuleSig->GetQueue ());


	if (aMuleSig->IsRunning ())
	{
		SetStatusText (_("aMule is running"));
	}
	else
	{
		SetStatusText (_("WARNING: aMule is NOT running"));
	}

	delete aMuleSig;
	Refresh ();
}
