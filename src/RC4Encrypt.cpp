//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2006 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "RC4Encrypt.h"

///////////////////////////////////////////////////////////////////////////////
// RC4 Encryption
//

static void swap_byte (uint8* a, uint8* b){
	uint8 bySwap;
	bySwap = *a;
	*a = *b;
	*b = bySwap;
}

RC4_Key_Struct* RC4CreateKey(const uint8* pachKeyData, uint32 nLen, RC4_Key_Struct* key, bool bSkipDiscard){
	uint8 index1;
	uint8 index2;
	uint8* pabyState;

	if (key == NULL) {
		key = new RC4_Key_Struct;
	}

	pabyState= &key->abyState[0];
	for (int i = 0; i < 256; i++) {
		pabyState[i] = (uint8)i;
	}

	key->byX = 0;
	key->byY = 0;
	index1 = 0;
	index2 = 0;
	
	for (int i = 0; i < 256; i++){
		index2 = (pachKeyData[index1] + pabyState[i] + index2) % 256;
		swap_byte(&pabyState[i], &pabyState[index2]);
		index1 = (uint8)((index1 + 1) % nLen);
	}
	
	if (!bSkipDiscard) {
		RC4Crypt(NULL, NULL, 1024, key);
	}
	
	return key;
}

void RC4Crypt(const uint8* pachIn, uint8* pachOut, uint32 nLen, RC4_Key_Struct* key){
	wxASSERT( key != NULL && nLen > 0 );
	
	if (key == NULL) {
		return;
	}
	
	uint8 byX = key->byX;;
	uint8 byY = key->byY;
	uint8* pabyState = &key->abyState[0];;
	uint8 byXorIndex;

	for (uint32 i = 0; i < nLen; i++) {
		byX = (byX + 1) % 256;
		byY = (pabyState[byX] + byY) % 256;
		swap_byte(&pabyState[byX], &pabyState[byY]);
		byXorIndex = (pabyState[byX] + pabyState[byY]) % 256;
		
		if (pachIn != NULL) {
			pachOut[i] = pachIn[i] ^ pabyState[byXorIndex];
		}
    }
	
	key->byX = byX;
	key->byY = byY;
}
