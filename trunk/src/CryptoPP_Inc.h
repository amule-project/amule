//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2006 aMule Team ( admin@amule.org / http://www.amule.org )
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

#ifndef CRYPTOPP_INC_H

#define CRYPTOPP_INC_H

#ifdef USE_EMBEDDED_CRYPTO
	#include "CryptoPP.h"
#else
	#ifdef __CRYPTO_DEBIAN_GENTOO__
		#include <crypto++/config.h>
		#include <crypto++/md4.h>
		#include <crypto++/rsa.h>
		#include <crypto++/base64.h>
		#include <crypto++/osrng.h>
		#include <crypto++/files.h>
		#include <crypto++/sha.h>
	#else 
		#include <cryptopp/config.h>
		#include <cryptopp/md4.h>
		#include <cryptopp/rsa.h>
		#include <cryptopp/base64.h>
		#include <cryptopp/osrng.h>
		#include <cryptopp/files.h>
		#include <cryptopp/sha.h>
	#endif
#endif

#endif /* CRYPTOPP_INC_H */
