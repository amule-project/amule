//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         Ed2kHash Class
///
/// Purpose:      aMule ed2k link creator
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Copyright (C) 2004 by Phoenix
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

#ifndef _ED2KHASH_H
#define _ED2KHASH_H

#ifdef __GNUG__
#pragma interface "ed2khash.h"
#endif

// Include wxWindows' headers
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/filename.h>
#endif

#include "md4.h"

class Ed2kHash:public MD4
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
    Ed2kHash ();

    /// Destructor
    ~Ed2kHash ();

    /// Set Ed2k hash from a file
    bool SetED2KHashFromFile(const wxFileName& filename, MD4Hook hook);

    /// Set Ed2k hash from a file
    bool SetED2KHashFromFile(const wxString& filename, MD4Hook hook);

    /// Get Ed2k Array of hashes
    wxArrayString GetED2KHash();

    /// Get Ed2k link
    wxString GetED2KLink(const wxArrayString& arrayOfUrls=0, const bool addPartHashes=false);
  };

#endif /* _ED2KHASH_H */

