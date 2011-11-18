////////////////////////////////////////////////////////////////////////////////
/// Name:         Main wxBase App
///
/// Purpose:      aMule ed2k link creator
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (c) 2004-2011 ThePolish ( thepolish@vipmail.ru )
///
/// This program is free software; you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation; either version 2 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the
/// Free Software Foundation, Inc.,
/// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
////////////////////////////////////////////////////////////////////////////////

#ifndef _ALCC_H
#define _ALCC_H


#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/cmdline.h>

//-----------------------------------------------------------------------------
// This can be put in a separete include file
#include <wx/strconv.h>
static wxCSConv aMuleConv(wxT("iso8859-1"));
#ifdef wxUSE_UNICODE
        #define unicode2char(x) (const char*) aMuleConv.cWX2MB(x)
        #define char2unicode(x) aMuleConv.cMB2WX(x)
#else
        #define unicode2char(x) x.c_str()
        #define char2unicode(x) x
#endif
//-----------------------------------------------------------------------------

// Application
class alcc : public wxAppConsole
{
  private:
    bool m_flagVerbose ;
    bool m_flagPartHashes;
    wxArrayString m_filesToHash;

    /// Parse command line
    virtual void OnInitCmdLine(wxCmdLineParser& cmdline);

    /// Command line preocessing
    virtual bool OnCmdLineParsed(wxCmdLineParser& cmdline);
    
  protected:
    wxLocale m_locale; // Used to tell wxCas to use aMule catalog
    
  public:
    /// Application
    virtual int OnRun ();
    
    /// Cleaning on exit
    virtual int OnExit();
};

DECLARE_APP(alcc)

#endif /* _ALCC_H */

// File_checked_for_headers
