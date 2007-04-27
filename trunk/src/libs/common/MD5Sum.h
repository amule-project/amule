// This file is part of the aMule Project.
 /* 
 Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
 Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
 rights reserved.

 License to copy and use this software is granted provided that it
 is identified as the "RSA Data Security, Inc. MD5 Message-Digest
 Algorithm" in all material mentioning or referencing this software
 or this function.

 License is also granted to make and use derivative works provided
 that such works are identified as "derived from the RSA Data
 Security, Inc. MD5 Message-Digest Algorithm" in all material
 mentioning or referencing the derived work.

 RSA Data Security, Inc. makes no representations concerning either
 the merchantability of this software or the suitability of this
 software for any particular purpose. It is provided "as is"
 without express or implied warranty of any kind.

 These notices must be retained in any copies of any part of this
 documentation and/or software.
 */

#ifndef MD5SUM_H
#define MD5SUM_H


class MD5Sum {
public:
	MD5Sum();
	MD5Sum(const wxString& sSource);
	MD5Sum(const uint8* buffer, int len);
	
	wxString Calculate(const wxString& sSource);
	wxString Calculate(const uint8* buffer, int len);

	wxString GetHash();
	Unicode2CharBuf GetRawHash() const { return unicode2char(m_sHash); }

private:
	wxString	m_sHash;
};

#endif // MD5SUM_H
// File_checked_for_headers
