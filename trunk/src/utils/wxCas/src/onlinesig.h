//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Name:        wxCas
// Purpose:    Display aMule Online Statistics
// Author:       ThePolish <thepolish@vipmail.ru>
// Copyright (C) 2004 by ThePolish
//
// Derived from CAS by Pedro de Oliveira <falso@rdk.homeip.net>
// Pixmats from aMule http://www.amule.org
//
// This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the
// Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

  wxFileName *m_amulesig;

  wxString BytesConvertion (wxString bytes);


public:

  // Constructors
    OnLineSig ();

    OnLineSig (wxFileName * file);

  // Destructor
   ~OnLineSig ();

  // Accessor
  void SetAmuleSig (wxFileName * file);
  void Refresh ();
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
