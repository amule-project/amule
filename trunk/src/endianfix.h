// This file is part of the aMule project.
//
// Copyright (c) 2004, aMule team
//
// Copyright (c) Angel Vidal Veiga (kry@users.sourceforge.net)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef ENDIANFIX_H
#define ENDIANFIX_H


#if wxBYTE_ORDER == wxLITTLE_ENDIAN
        #define ENDIAN_SWAP_16(x) (x)
        #define ENDIAN_SWAP_I_16(x) {}
        #define ENDIAN_SWAP_32(x) (x)
        #define ENDIAN_SWAP_I_32(x) {}
        #if defined __GNUC__ && __GNUC__ >= 2
                #define ENDIAN_SWAP_64(x) (x)
                #define ENDIAN_SWAP_I_64(x) {}
        #endif
#else
        #warning BIG ENDIAN BOX
        #define ENDIAN_SWAP_16(x) (wxUINT16_SWAP_ALWAYS(x))
        #define ENDIAN_SWAP_I_16(x) x = wxUINT16_SWAP_ALWAYS(x)
        #define ENDIAN_SWAP_32(x) (wxUINT32_SWAP_ALWAYS(x))
        #define ENDIAN_SWAP_I_32(x) x = wxUINT32_SWAP_ALWAYS(x)
        #if defined __GNUC__ && __GNUC__ >= 2
                #define ENDIAN_SWAP_64(x) (wxUINT64_SWAP_ALWAYS(x))
                #define ENDIAN_SWAP_I_64(x) x = wxUINT64_FROM_LE(x)
        #endif
#endif


#endif // ENDIANFIX_H

