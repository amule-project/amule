//
// This file is part of the aMule Project.
//
// Copyright (c) 2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef MULE_STRERROR_R_H
#define MULE_STRERROR_R_H

/**
 * Return string describing error number.
 *
 * This function implements the XSI-compliant strerror_r() function whenever
 * it's possible. Also it is thread safe if there is a thread-safe function
 * to get the error description.
 *
 * @param errnum Error number for which the description is needed.
 * @param buf    Buffer to store the error description.
 * @param buflen Length of the buffer.
 *
 * @return 0 on success; on error, -1 is returned and errno is set to indicate
 *	   the error.
 */
extern "C" int mule_strerror_r(int errnum, char *buf, size_t buflen);

#endif /* MULE_STRERROR_R_H */
