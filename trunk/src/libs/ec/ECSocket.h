//
// This file is part of the aMule Project.
//
// Copyright (c) 2004-2005 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2004-2005 Angel Vidal Veiga ( kry@users.sourceforge.net )
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA, 02111-1307, USA
//

#ifndef ECSOCKET_H
#define ECSOCKET_H

#include <wx/string.h>
#include <wx/socket.h>
#include "Types.h"

#include <zlib.h>		// Needed for packet (de)compression

#include <list>

class CECPacket;

enum {
	EC_SOCKET_HANDLER = wxID_HIGHEST + 666,
};

class CECSocketHandler: public wxEvtHandler {
	public:
        CECSocketHandler() {};
		
	private:
        void SocketHandler(wxSocketEvent& event);
        DECLARE_EVENT_TABLE()
};

/*! \class CECSocket
 *
 * \brief Socket handler for External Communications (EC).
 *
 * CECSocket takes care of the transmission of EC packets
 */

class CECSocket : public wxSocketClient {
	public:
		//
		// Constructors/Destructor
		//
		CECSocket();

		~CECSocket();

		//
		// Packet I/O
		//
		// These 4 methods are to be used by CECPacket & CECTag
		bool ReadNumber(void *buffer, unsigned int len);
		bool ReadBuffer(void *buffer, unsigned int len);

		bool WriteNumber(const void *buffer, unsigned int len);
		bool WriteBuffer(const void *buffer, unsigned int len);

		//
		// Wrapper functions for client sockets
		//
		CECPacket *ReadPacket(void);
		bool WritePacket(const CECPacket *packet);

		virtual void OnConnect();
		virtual void OnSend();
		virtual void OnReceive();
		virtual void OnClose();
		virtual void OnError();

	private:
		CECSocketHandler handler;

		bool FlushBuffers();
		void InitBuffers();

		uint32	ReadFlags();
		bool	WriteFlags(uint32);		

		//
		// working buffers
		struct socket_desc {
			bool			firsttransfer;
			uint32			accepts;
			unsigned char *		in_ptr;
			unsigned char *		out_ptr;
			// This transfer only
			wxSocketError		LastSocketError;
			uint32			used_flags;
			z_stream		z;
		} parms;

		/*
		 * Those buffers needed for event driven io (both rx and tx path)
		 */
		// RX queue
		int m_buf_size;
		unsigned char *m_buffer, *m_curr_ptr;
		int m_tags_left, m_bytes_left; // how match to wait

		// TX queue
		class EC_OUTBUF {
			public:
				unsigned char *m_buf, *m_current;
				int m_size;
				~EC_OUTBUF() { if ( m_buf ) delete [] m_buf; }
		};
		std::list<EC_OUTBUF> m_pending_tx;
		
};

#endif // ECSOCKET_H
