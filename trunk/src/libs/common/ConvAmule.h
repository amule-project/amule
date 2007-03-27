//
// This file is part of the aMule Project.
//
// Copyright (c) 2007 Marcelo Roberto Jimenez - Phoenix (phoenix@amule.org)
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
//
// Any parts of this program derived from the xMule, lMule or eMule project,
// or contributed by third-party developers are copyrighted by their
// respective authors.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
//


#include <wx/url.h>


class WXDLLIMPEXP_BASE ConvAmuleBrokenFileNames : public wxMBConvUTF8
{
private:
	const wxString m_charset;
	wxCSConv m_conv;
	
public:
	ConvAmuleBrokenFileNames(const wxChar *charset);
	ConvAmuleBrokenFileNames(const ConvAmuleBrokenFileNames& conv);
	virtual ~ConvAmuleBrokenFileNames();
	ConvAmuleBrokenFileNames &operator=(const ConvAmuleBrokenFileNames &conv);
	
	virtual size_t MB2WC(wchar_t *out, const char *in, size_t outLen) const;
	virtual size_t WC2MB(char *out, const wchar_t *in, size_t outLen) const;
	virtual size_t GetMBNulLen() const;
	virtual wxMBConv *Clone() const;
};


#ifdef CONVAMULE_CPP
	wxCSConv aMuleConv(wxConvLocal);
	ConvAmuleBrokenFileNames aMuleConvBrokenFileNames(wxT("ISO-8859-1"));
#else
	extern wxCSConv aMuleConv;
	extern ConvAmuleBrokenFileNames aMuleConvBrokenFileNames;
#endif

