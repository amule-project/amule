/////////////////////////////////////////////////////////////////////////////
// Name:        onlinesig.cpp
// Purpose:     wxCas online signature
// Author:      ThePolish <thepolish@vipmail.ru>
// Created:     2003/04/10
// Modified by:
// Copyright:   (c) ThePolish <thepolish@vipmail.ru>
// Licence:     GPL
// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
/////////////////////////////////////////////////////////////////////////////

#ifndef _ONLINESIG_H
#define _ONLINESIG_H

#ifdef __GNUG__
#pragma interface "wxcasframe.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/filename.h>

// OnLineSig Class
class OnLineSig
{
      private:
		  
	wxString m_isRunning;
	wxString m_serverName;
	wxString m_serverIP;
	wxString m_serverPort;
	wxString m_connexionID;
	wxString m_ULRate;
	wxString m_DLRate;
	wxString m_queue;
	wxString m_sharedFiles;
	wxString m_user;
	wxString m_totalUL;
	wxString m_totalDL;
	wxString m_version;
	wxString m_sessionUL;
	wxString m_sessionDL;
	wxString m_runTime;

	wxString BytesConvertion (wxString bytes);

      public:

	// Constructors
	  OnLineSig ();

	  OnLineSig (wxFileName * file);

	// Destructor
	 ~OnLineSig ();

	// Accessor
	void SetFromAmuleSig (wxFileName * file);
	void SetFromDefaultAmuleSig ();

	bool IsRunning ();

	wxString GetServerName ();
	wxString GetServerIP ();
	wxString GetServerPort ();
	wxString GetConnexionID ();
	wxString GetULRate ();
	wxString GetDLRate ();
	wxString GetQueue ();
	wxString GetSharedFiles ();
	wxString GetUser ();
	wxString GetTotalUL ();
	wxString GetTotalDL ();
	wxString GetVersion ();
	wxString GetSessionUL ();
	wxString GetSessionDL ();
	wxString GetRunTime ();
	wxString GetConvertedTotalUL ();
	wxString GetConvertedTotalDL ();
	wxString GetConvertedSessionUL ();
	wxString GetConvertedSessionDL ();
	wxString GetConnexionIDType ();
};

#endif /* _ONLINESIG_H */
