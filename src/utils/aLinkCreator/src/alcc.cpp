//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         Main wxBase App
///
/// Purpose:      aMule ed2k link creator
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Pixmaps from http://www.everaldo.com and http://www.amule.org
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
/// 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __GNUG__
#pragma implementation "alcc.h"
#endif

// For compilers that support precompilation
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP || defined(__WXMAC__)
#include <wx/filename.h>
#endif

#include "alcc.h"
#include "ed2khash.h"

// Application implementation
IMPLEMENT_APP (alcc)

/// Running Alcc
int alcc::OnRun ()
{
  Ed2kHash hash;
  size_t i;
  for (i=0;i<(m_filesToHash.GetCount());i++)
    {
      if (wxFileExists(m_filesToHash[i]))
        {
          if (m_flagVerbose)
            {
              printf(_("Processing file number %u: %s\n"),i+1,m_filesToHash[i].mb_str());
              if (m_flagPartHashes)
                {
                  printf(_("You have asked for part hashes (Only used for files > 9.5 MB)\n"));
                }
            }
	    
          printf (_("Please wait... "));
	    
          if (hash.SetED2KHashFromFile(m_filesToHash[i], NULL))
            {
              printf (_("Done !\n"));
		    
              printf (wxT("%s ---> %s\n\n"),m_filesToHash[i].mb_str(),
                      hash.GetED2KLink(m_flagPartHashes).mb_str());
            }
        }
      else
        {
          printf (_("%s ---> Non existant file !\n\n"),m_filesToHash[i].mb_str());
        }
    }
  return 0;
}


/// Parse command line
void alcc::OnInitCmdLine(wxCmdLineParser& cmdline)
{
  cmdline.SetDesc(cmdLineDesc);
}

/// Command line preocessing
bool alcc::OnCmdLineParsed(wxCmdLineParser& cmdline)
{

  wxFileName filename;
  size_t i;

  m_flagVerbose = cmdline.Found(wxT("v"));
  m_flagPartHashes = cmdline.Found(wxT("p"));

  m_filesToHash.Clear();
  for (i = 0; i < cmdline.GetParamCount(); i++)
    {
      filename.Assign(cmdline.GetParam(i));
      m_filesToHash.Add(filename.GetFullPath());
    }
  m_filesToHash.Shrink();
    
  return true;
}
