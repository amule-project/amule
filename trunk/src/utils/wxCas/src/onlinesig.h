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

  wxString BytesConvertion (wxString bytes);

  wxFileName* Default_aMule_onlinesig;

public:

  // Constructors
    OnLineSig ();

    OnLineSig (wxFileName * file);

  // Destructor
   ~OnLineSig ();

  // Accessor
  void SetFromAmuleSig (wxFileName * file);
  void SetFromDefaultAmuleSig ()
  {
    SetFromAmuleSig (Default_aMule_onlinesig);
  }


	
	bool IsRunning ()
	{
	  if (m_isRunning == "1")
	    {
		 return TRUE;
	    }
	  else
	    {
		 return FALSE;
	    }
	}
	
	wxString GetServerName ()
	{
	  return m_serverName;
	}
	
	wxString GetServerIP ()
	{
	  return m_serverIP;
	}
	
	wxString GetServerPort ()
	{
	  return m_serverPort;
	}
	
	wxString GetConnexionID ()
	{
	  return m_connexionID;
	}
	
	wxString GetULRate ()
	{
	  return m_ULRate;
	}
	
	wxString GetDLRate ()
	{
	  return m_DLRate;
	}
	
	wxString GetQueue ()
	{
	  return m_queue;
	}
	
	wxString GetSharedFiles ()
	{
	  return m_sharedFiles;
	}
	
	wxString GetUser ()
	{
	  return m_user;
	}
	
	wxString GetTotalUL ()
	{
	  return m_totalUL;
	}
	
	
	wxString GetTotalDL ()
	{
	  return m_totalDL;
	}
	
	wxString GetVersion ()
	{
	  return m_version;
	}
	
	wxString GetSessionUL ()
	{
	  return m_sessionUL;
	}
	
	
	wxString GetSessionDL ()
	{
	  return m_sessionDL;
	}
	
	wxString GetRunTime ()
	{
	  return m_runTime;
	}
	
	wxString GetConvertedTotalUL ()
	{
	  return (BytesConvertion (m_totalUL));
	}
	
	wxString GetConvertedTotalDL ()
	{
	  return (BytesConvertion (m_totalDL));
	}
	
	wxString GetConvertedSessionUL ()
	{
	  return (BytesConvertion (m_sessionUL));
	}
	
	wxString GetConvertedSessionDL ()
	{
	  return (BytesConvertion (m_sessionDL));
	}
	
	wxString GetConnexionIDType ()
	{
	  if (m_connexionID == "H")
	    {
		 return (wxString ("HighID"));
	    }
	  else
	    {
		 return (wxString ("LowID"));
	    }
	}


};

#endif /* _ONLINESIG_H */
