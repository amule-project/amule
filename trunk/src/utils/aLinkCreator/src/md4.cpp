////////////////////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////////////////////


#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include <wx/ffile.h>

#include "md4.h"
#include "bithelp.h"


/// BIG ENDIAN byte reversing
#if wxBYTE_ORDER == wxBIG_ENDIAN
// Note: this code is harmless on little-endian machines.
void MD4::byteReverse(unsigned char *buf, unsigned longs)
{
  uint32_t t;
  do
    {
      t = (uint32_t) ((unsigned) buf[3] << 8 | buf[2]) << 16 |
          ((unsigned) buf[1] << 8 | buf[0]);
      *(uint32_t *) buf = t;
      buf += 4;
    }
  while (--longs);
}
#else
#define byteReverse(buf, len)	do { } while (0)
#endif

/// Start MD4 accumulation.
/// Set bit count to 0 and buffer to mysteriousinitialization constants.
void MD4::MD4Init(struct MD4Context *ctx)
{
  ctx->buf[0] = 0x67452301;
  ctx->buf[1] = 0xefcdab89;
  ctx->buf[2] = 0x98badcfe;
  ctx->buf[3] = 0x10325476;
  ctx->bits[0] = 0;
  ctx->bits[1] = 0;
}

/// Update context to reflect the concatenation of another buffer full of bytes.
void MD4::MD4Update(struct MD4Context *ctx, unsigned char const *buf,
                    size_t len)
{
  register uint32_t t;

  // Update bitcount
  t = ctx->bits[0];
  if ((ctx->bits[0] = t + ((uint32_t) len << 3)) < t)
    ctx->bits[1]++;	// Carry from low to high
  ctx->bits[1] += len >> 29;

  t = (t >> 3) & 0x3f;	// Bytes already in shsInfo->data

  // Handle any leading odd-sized chunks
  if (t)
    {
      unsigned char *p = (unsigned char *) ctx->in + t;

      t = 64 - t;
      if (len < t)
        {
          memcpy(p, buf, len);
          return;
        }
      memcpy(p, buf, t);
      byteReverse(ctx->in, 16);
      MD4Transform(ctx->buf, (uint32_t *) ctx->in);
      buf += t;
      len -= t;
    }

  // Process data in 64-byte chunks
  while (len >= 64)
    {
      memcpy(ctx->in, buf, 64);
      byteReverse(ctx->in, 16);
      MD4Transform(ctx->buf, (uint32_t *) ctx->in);
      buf += 64;
      len -= 64;
    }

  //Handle any remaining bytes of data.
  memcpy(ctx->in, buf, len);
}


///  Final wrapup - pad to 64-byte boundary with the bit pattern
///  1 0* (64-bit count of bits processed, MSB-first)
void MD4::MD4Final(struct MD4Context *ctx, unsigned char* digest)
{
  unsigned int count;
  unsigned char *p;

  // Compute number of bytes mod 64
  count = (ctx->bits[0] >> 3) & 0x3F;

  // Set the first char of padding to 0x80.
  //This is safe since there is always at least one byte free
  p = ctx->in + count;
  *p++ = 0x80;

  // Bytes of padding needed to make 64 bytes
  count = 64 - 1 - count;

  // Pad out to 56 mod 64
  if (count < 8)
    {
      // Two lots of padding:  Pad the first block to 64 bytes
      memset(p, 0, count);
      byteReverse(ctx->in, 16);
      MD4Transform(ctx->buf, (uint32_t *) ctx->in);

      // Now fill the next block with 56 bytes
      memset(ctx->in, 0, 56);
    }
  else
    {
      // Pad block to 56 bytes
      memset(p, 0, count - 8);
    }
  byteReverse(ctx->in, 14);

  // Append length in bits and transform
  ((uint32_t *) ctx->in)[14] = ctx->bits[0];
  ((uint32_t *) ctx->in)[15] = ctx->bits[1];

  MD4Transform(ctx->buf, (uint32_t *) ctx->in);
  byteReverse((unsigned char *) ctx->buf, 4);

  if (digest!=NULL)
    {
      memcpy(digest, ctx->buf, 16);
    }
  memset(ctx, 0, sizeof(ctx));	// In case it's sensitive
}

/// The three core functions
#define MD4_F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define MD4_G(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define MD4__H(x, y, z) ((x) ^ (y) ^ (z))

#define MD4_FF(a, b, c, d, x, s) { \
    (a) += MD4_F ((b), (c), (d)) + (x); \
    (a) = rol ((a), (s)); \
  }
#define MD4_GG(a, b, c, d, x, s) { \
    (a) += MD4_G ((b), (c), (d)) + (x) + (uint32_t)0x5a827999; \
    (a) = rol ((a), (s)); \
  }
#define MD4_HH(a, b, c, d, x, s) { \
    (a) += MD4__H ((b), (c), (d)) + (x) + (uint32_t)0x6ed9eba1; \
    (a) = rol ((a), (s)); \
  }

/// The core of the MD4 algorithm
void MD4::MD4Transform(uint32_t buf[4], uint32_t const in[16])
{
  register uint32_t a, b, c, d;

  a = buf[0];
  b = buf[1];
  c = buf[2];
  d = buf[3];

  MD4_FF(a, b, c, d, in[0], 3);	    /* 1 */
  MD4_FF(d, a, b, c, in[1], 7);	    /* 2 */
  MD4_FF(c, d, a, b, in[2], 11);	/* 3 */
  MD4_FF(b, c, d, a, in[3], 19);	/* 4 */
  MD4_FF(a, b, c, d, in[4], 3);		/* 5 */
  MD4_FF(d, a, b, c, in[5], 7);		/* 6 */
  MD4_FF(c, d, a, b, in[6], 11);	/* 7 */
  MD4_FF(b, c, d, a, in[7], 19);	/* 8 */
  MD4_FF(a, b, c, d, in[8], 3);		/* 9 */
  MD4_FF(d, a, b, c, in[9], 7);		/* 10 */
  MD4_FF(c, d, a, b, in[10], 11);	/* 11 */
  MD4_FF(b, c, d, a, in[11], 19);	/* 12 */
  MD4_FF(a, b, c, d, in[12], 3);	/* 13 */
  MD4_FF(d, a, b, c, in[13], 7);	/* 14 */
  MD4_FF(c, d, a, b, in[14], 11);	/* 15 */
  MD4_FF(b, c, d, a, in[15], 19);	/* 16 */

  MD4_GG(a, b, c, d, in[0], 3);		/* 17 */
  MD4_GG(d, a, b, c, in[4], 5);		/* 18 */
  MD4_GG(c, d, a, b, in[8], 9);		/* 19 */
  MD4_GG(b, c, d, a, in[12], 13);	/* 20 */
  MD4_GG(a, b, c, d, in[1], 3);		/* 21 */
  MD4_GG(d, a, b, c, in[5], 5);		/* 22 */
  MD4_GG(c, d, a, b, in[9], 9);		/* 23 */
  MD4_GG(b, c, d, a, in[13], 13);	/* 24 */
  MD4_GG(a, b, c, d, in[2], 3);		/* 25 */
  MD4_GG(d, a, b, c, in[6], 5);		/* 26 */
  MD4_GG(c, d, a, b, in[10], 9);	/* 27 */
  MD4_GG(b, c, d, a, in[14], 13);	/* 28 */
  MD4_GG(a, b, c, d, in[3], 3);		/* 29 */
  MD4_GG(d, a, b, c, in[7], 5);		/* 30 */
  MD4_GG(c, d, a, b, in[11], 9);	/* 31 */
  MD4_GG(b, c, d, a, in[15], 13);	/* 32 */

  MD4_HH(a, b, c, d, in[0], 3);		/* 33 */
  MD4_HH(d, a, b, c, in[8], 9);		/* 34 */
  MD4_HH(c, d, a, b, in[4], 11);	/* 35 */
  MD4_HH(b, c, d, a, in[12], 15);	/* 36 */
  MD4_HH(a, b, c, d, in[2], 3);		/* 37 */
  MD4_HH(d, a, b, c, in[10], 9);	/* 38 */
  MD4_HH(c, d, a, b, in[6], 11);	/* 39 */
  MD4_HH(b, c, d, a, in[14], 15);	/* 40 */
  MD4_HH(a, b, c, d, in[1], 3);		/* 41 */
  MD4_HH(d, a, b, c, in[9], 9);		/* 42 */
  MD4_HH(c, d, a, b, in[5], 11);	/* 43 */
  MD4_HH(b, c, d, a, in[13], 15);	/* 44 */
  MD4_HH(a, b, c, d, in[3], 3);		/* 45 */
  MD4_HH(d, a, b, c, in[11], 9);	/* 46 */
  MD4_HH(c, d, a, b, in[7], 11);	/* 47 */
  MD4_HH(b, c, d, a, in[15], 15);	/* 48 */

  buf[0] += a;
  buf[1] += b;
  buf[2] += c;
  buf[3] += d;
}

/// Algorithm verification
bool MD4::selfTest()
{
  wxString test1(wxEmptyString);
  wxString test1_md(wxT("31D6CFE0D16AE931B73C59D7E0C089C0"));
  wxString test2(wxT("a"));
  wxString test2_md(wxT("BDE52CB31DE33E46245E05FBDBD6FB24"));
  wxString test3(wxT("abc"));
  wxString test3_md(wxT("A448017AAF21D8525FC10AE87AA6729D"));
  wxString test4(wxT("message digest"));
  wxString test4_md(wxT("D9130A8164549FE818874806E1C7014B"));
  wxString test5(wxT("abcdefghijklmnopqrstuvwxyz"));
  wxString test5_md(wxT("D79E1C308AA5BBCDEEA8ED63DF412DA9"));
  wxString test6(wxT("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"));
  wxString test6_md(wxT("043F8582F241DB351CE627E153E7F0E4"));
  wxString test7(wxT("12345678901234567890123456789012345678901234567890123456789012345678901234567890"));
  wxString test7_md(wxT("E33B4DDC9C38F2199C3E7B164FCC0536"));

  MD4 md4;

  if (md4.calcMd4FromString(test1) != test1_md)
    return false;
  if (md4.calcMd4FromString(test2) != test2_md)
    return false;
  if (md4.calcMd4FromString(test3) != test3_md)
    return false;
  if (md4.calcMd4FromString(test4) != test4_md)
    return false;
  if (md4.calcMd4FromString(test5) != test5_md)
    return false;
  if (md4.calcMd4FromString(test6) != test6_md)
    return false;
  if (md4.calcMd4FromString(test7) != test7_md)
    return false;

  return true;
}

/// Get Md4 hash from a string
wxString MD4::calcMd4FromString(const wxString &buf)
{
  MD4Context hdc;
  unsigned char ret[MD4_HASHLEN_BYTE];

  MD4Init(&hdc);
  MD4Update(&hdc, (const unsigned char*)buf.c_str(), buf.length());
  MD4Final(&hdc, ret);

  return charToHex((const char*)ret, MD4_HASHLEN_BYTE);
}

/// Get Md4 hash from a file
wxString MD4::calcMd4FromFile(const wxString &filename, MD4Hook hook)
{
  unsigned int bufSize;
  unsigned char ret[MD4_HASHLEN_BYTE];
  MD4Context hdc;

  // Open file and let wxFFile destructor close the file
  // Closing it explicitly may crash on Win32 ...
  wxFFile file(filename, wxT("rbS"));
  if (! file.IsOpened())
    {
      return wxEmptyString;
    }
  else
    {
      bufSize = calcBufSize(file.Length());
      char *buf = new char[bufSize];

      bool keep_going = true;
      size_t read = 0;
      size_t totalread = 0;

      bool goAhead = true;

      MD4Init(&hdc);
      while (!file.Eof() && keep_going)
        {
          if (hook)
            {
              goAhead = hook( (int)((double)(100.0 * totalread) / file.Length()));
            }
          if (goAhead)
            {
              read = file.Read(buf, bufSize);
              MD4Update(&hdc, reinterpret_cast<unsigned char const *>(buf),
                        read );
              totalread += read;
            }
          else
            {
              return (_("Canceled !"));
            }
        }
      MD4Final(&hdc, ret);

      delete [] buf;

      return charToHex(reinterpret_cast<const char *>(ret),
                       MD4_HASHLEN_BYTE);
    }
}

/// Convert hash to hexa string
wxString MD4::charToHex(const char *buf, size_t len)
{
  size_t i;
  wxString hexString;

  for (i = 0; i < len; ++i)
    {
      hexString += wxString::Format(wxT("%02x"), 0xFF & *(buf + i));
    }

  // Reduce memory usage
  hexString.Shrink();

  return (hexString);
}

/// Compute Md4 buffsize
size_t MD4::calcBufSize(size_t filesize)
{
  if (filesize < 100000)
    {
      filesize = 100000;
    }
  else if (filesize > 200000)
    {
      filesize = 200000;
    }

  return (filesize);
}
// File_checked_for_headers
