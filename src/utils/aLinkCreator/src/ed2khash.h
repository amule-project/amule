//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         Ed2kHash Class
///
/// Purpose:      aMule ed2k link creator
///
/// Author:       ThePolish <thepolish@vipmail.ru>
///
/// Copyright (c) 2004-2011 ThePolish ( thepolish@vipmail.ru )
///
/// Copyright (c) 2004-2011 Marcelo Roberto Jimenez ( phoenix@amule.org )
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

#ifndef _ED2KHASH_H
#define _ED2KHASH_H


#include <wx/filename.h>

#include "md4.h"

class Ed2kHash:public MD4
  {
  private:

    wxArrayString m_ed2kArrayOfHashes;
    wxString m_filename;
    uint64_t m_fileSize;

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
    wxString GetED2KLink(const bool addPartHashes=false, const wxArrayString* arrayOfUrls = NULL);
  };

#endif /* _ED2KHASH_H */

// File_checked_for_headers
