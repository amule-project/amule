/* bithelp.h  -  Some bit manipulation helpers
 *	Copyright (C) 1999, 2002 Free Software Foundation, Inc.
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser general Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef G10_BITHELP_H
#define G10_BITHELP_H

#include <inttypes.h>

/****************
 * Rotate the 32 bit unsigned integer X by N bits left/right
 */
#if defined(__GNUC__) && defined(__i386__)
static inline uint32_t
rol(uint32_t x, int n)
{
  __asm__("roll %%cl,%0"
        :"=r" (x)
              :"0" (x),"c" (n));
  return x;
}
#else
#define rol(x,n) ( ((x) << (n)) | ((x) >> (32-(n))) )
#endif

#if defined(__GNUC__) && defined(__i386__)
static inline uint32_t
ror(uint32_t x, int n)
{
  __asm__("rorl %%cl,%0"
        :"=r" (x)
              :"0" (x),"c" (n));
  return x;
}
#else
#define ror(x,n) ( ((x) >> (n)) | ((x) << (32-(n))) )
#endif


#endif /*G10_BITHELP_H*/
