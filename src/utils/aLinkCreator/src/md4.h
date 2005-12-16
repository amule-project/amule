//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Name:         MD4 Class
///
/// Purpose:      aMule ed2k link creator
///
/// Last modified by: ThePolish <thepolish@vipmail.ru>
///
/// Copyright (C) 2004 by ThePolish
///
/// Copyright (C) 2004 by Phoenix
///
/// Copyright (C) 2004 by Madcat
///
/// Copyright (C) 2002, 2003, 2004 by Michael Buesch
/// Email: mbuesch@freenet.de
///
/// The algorithm is due to Ron Rivest.  This code is based on code
/// written by Colin Plumb in 1993.
///
/// This code implements the MD4 message-digest algorithm.
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

#ifndef _MD4_H
#define _MD4_H

// For compilers that support precompilation, includes "wx/wx.h"
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif


#include <inttypes.h> // needed for uint32_t

// Use wxString implementation or not
//#define WANT_STRING_IMPLEMENTATION 1

// Hook for external progress bar
typedef bool (*MD4Hook)(int percent);


const unsigned int MD4_HASHLEN_BYTE = 128 / 8;
const unsigned int BUFSIZE = 64*1024;
const unsigned int PARTSIZE = 9500*1024;


class MD4
  {
  private:

  protected:

    struct MD4Context
      {
        uint32_t buf[4];
        uint32_t bits[2];
        unsigned char in[64];
      };

    void MD4Init(struct MD4Context *context);
    void MD4Update(struct MD4Context *context,
                   unsigned char const *buf, size_t len);
    void MD4Final(struct MD4Context *context,
                  unsigned char *digest);
    void MD4Transform(uint32_t buf[4], uint32_t const in[16]);

    wxString charToHex(const char *buf, size_t len);

    size_t calcBufSize(size_t filesize);

    // Needed to reverse byte order on BIG ENDIAN machines
#if wxBYTE_ORDER == wxBIG_ENDIAN

    void byteReverse(unsigned char *buf, unsigned longs);
#endif

  public:

    /// Constructor
    MD4()
    {}

    /// Destructor
    virtual ~MD4()
    {}

    /// Algorithm verification
    static bool selfTest();

    /// Get Md4 hash from a string
    wxString calcMd4FromString(const wxString &buf);

    /// Get Md4 hash from a file
    wxString calcMd4FromFile(const wxString &filename, MD4Hook hook);
  };

#endif /* _MD4_H */

