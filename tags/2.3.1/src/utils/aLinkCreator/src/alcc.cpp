//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"             // Needed for PACKAGE
#else
#define PACKAGE "amule"
#endif


#include "alcc.h"
#include "ed2khash.h"

// Application implementation
IMPLEMENT_APP (alcc)

/// Running Alcc
int alcc::OnRun ()
{
  // Used to tell alcc to use aMule catalog
  m_locale.Init();
  m_locale.AddCatalog(wxT(PACKAGE));

  wxLog::DontCreateOnDemand();
  wxLogStderr * stderrLog = new wxLogStderr;
  wxLogStderr * stdoutLog = new wxLogStderr(stdout);
  delete wxLog::SetActiveTarget(stderrLog); // Log on Stderr
#if wxCHECK_VERSION(2, 9, 0)  
  wxLog::SetTimestamp("");   // Disable timestamp on messages
#else
  wxLog::SetTimestamp(NULL); // Disable timestamp on messages
#endif

  Ed2kHash hash;
  size_t i;
  for (i=0;i<(m_filesToHash.GetCount());++i)
    {
      if (wxFileExists(m_filesToHash[i]))
        {
          if (m_flagVerbose)
            {
              wxLogMessage(_("Processing file number %u: %s"),i+1,m_filesToHash[i].c_str());

              if (m_flagPartHashes)
                {
                  wxLogMessage(_("You have asked for part hashes (Only used for files > 9.5 MB)"));
                }
            }

          if (hash.SetED2KHashFromFile(m_filesToHash[i], NULL))
            {
				// Print the link to stdout
				wxLog::SetActiveTarget(stdoutLog);
                wxLogMessage(wxT("%s"), hash.GetED2KLink(m_flagPartHashes).c_str());
				// Everything else goes to stderr
				wxLog::SetActiveTarget(stderrLog);
            }
        }
      else
        {
            if (m_flagVerbose)
                {
                    wxLogMessage(_("%s ---> Non existant file !\n"),m_filesToHash[i].c_str());
                }
        }
    }
  return 0;
}

// On exit
int
alcc::OnExit()
{
  delete wxLog::SetActiveTarget(NULL);
  return 0;
}

/// Parse command line
void alcc::OnInitCmdLine(wxCmdLineParser& cmdline)
{
	cmdline.AddSwitch(wxT("h"), wxT("help"), wxT("show this help message"), wxCMD_LINE_OPTION_HELP);
	cmdline.AddSwitch(wxT("v"), wxT("verbose"), wxT("be verbose"));
	cmdline.AddSwitch(wxT("p"), wxT("parthashes"), wxT("add part-hashes to ed2k link"));
	cmdline.AddParam(wxT("input files"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE);
}

/// Command line preocessing
bool alcc::OnCmdLineParsed(wxCmdLineParser& cmdline)
{

  wxFileName filename;
  size_t i;

  m_flagVerbose = cmdline.Found(wxT("v"));
  m_flagPartHashes = cmdline.Found(wxT("p"));

  m_filesToHash.Clear();
  for (i = 0; i < cmdline.GetParamCount(); ++i)
    {
      filename.Assign(cmdline.GetParam(i));
      m_filesToHash.Add(filename.GetFullPath());
    }
  m_filesToHash.Shrink();

  return true;
}
// File_checked_for_headers
