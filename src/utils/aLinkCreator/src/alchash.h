//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         AlcHash Class
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

#ifndef _ALCHASH_H
#define _ALCHASH_H

#ifdef __GNUG__
#pragma interface "alchash.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/filename.h>

#include "md4.h"

class AlcHash
  {
  private:

    wxArrayString m_ed2kArrayOfHashes;
    wxString m_filename;
    size_t m_fileSize;

  protected:

    /// Strip all non-alphanumeric characters of a filename string
    wxString CleanFilename(const wxString& filename);

  public:
    /// Constructor
    AlcHash ();

    /// Destructor
    ~AlcHash ();

    /// Set Ed2k hash from a file
    void SetED2KHashFromFile(const wxFileName& filename);

    /// Set Ed2k hash from a file
    void SetED2KHashFromFile(const wxString& filename);

    /// Get Ed2k Array of hashes
    wxArrayString GetED2KHash();

    /// Get Ed2k link
    wxString GetED2KLink(const wxArrayString& arrayOfUrls=0, const bool addPartHashes=FALSE);
  };

#endif /* _ALCHASH_H */
