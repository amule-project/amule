// This file is part of the aMule Project.
//
// Copyright (c) 2003-2004 aMule Project ( http://www.amule-project.net )
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
//

#include <wx/mstream.h>
#include <wx/gifdecod.h>
#include <wx/dcbuffer.h>

#include "MuleGifCtrl.h"

BEGIN_EVENT_TABLE(MuleGifCtrl, wxControl)
	EVT_TIMER(GIFTIMERID, MuleGifCtrl::OnTimer)
	EVT_PAINT(MuleGifCtrl::OnPaint)
	EVT_ERASE_BACKGROUND(MuleGifCtrl::OnErase)
END_EVENT_TABLE()

	
MuleGifCtrl::MuleGifCtrl( wxWindow *parent, wxWindowID id, const wxPoint& pos,
                          const wxSize& size, long style, const wxValidator& validator,
                          const wxString& name )
: wxControl( parent, id, pos, size, style, validator, name ),
  m_decoder( NULL ),
  m_timer( this, GIFTIMERID )
{
}


MuleGifCtrl::~MuleGifCtrl()
{
	m_timer.Stop();
	
	if ( m_decoder ) {
		delete m_decoder;
		m_decoder = NULL;
	}
};


bool MuleGifCtrl::LoadData(const char* data, int size)
{
	if (m_decoder) {
		m_timer.Stop();
   		delete m_decoder;
  		m_decoder = NULL;
	}
 
  	wxMemoryInputStream stream(data, size);
  	m_decoder = new wxGIFDecoder(&stream, TRUE);
  	if ( m_decoder->ReadGIF() != wxGIF_OK ) {
   		delete m_decoder;
   		m_decoder = NULL;
   		return false;
  	}

	m_decoder->GoFirstFrame();
	wxImage frame;
	m_decoder->ConvertToImage( &frame ); 
	m_frame = frame;
 
	return true;
}
	
	
void MuleGifCtrl::Start()
{
	if ( m_decoder && m_decoder->IsAnimation() ) {
		m_timer.Stop();
		m_decoder->GoLastFrame();
	
		wxTimerEvent evt;
		OnTimer( evt );
	}
}


void MuleGifCtrl::Stop()
{
	m_timer.Stop();
}


wxSize MuleGifCtrl::GetBestSize()
{
	return wxSize( m_decoder->GetWidth(), m_decoder->GetHeight() );
}


void MuleGifCtrl::OnTimer( wxTimerEvent& WXUNUSED(event) )
{
	if ( m_decoder ) {
		if ( m_decoder->IsAnimation() )
			m_decoder->GoNextFrame( true );

		wxImage frame;
		m_decoder->ConvertToImage( &frame ); 
		m_frame = frame;
		
		Refresh();
			
		if ( m_decoder->IsAnimation() )
			m_timer.Start( m_decoder->GetDelay(), true );
	}
}


void MuleGifCtrl::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
	wxBufferedPaintDC dc(this);
   
    wxSize size = GetClientSize();
	int x = (size.GetWidth()-m_frame.GetWidth())/2;
	int y = (size.GetHeight()-m_frame.GetHeight())/2;

	dc.SetBackground( wxBrush( GetBackgroundColour(), wxSOLID ));
	dc.Clear();
	dc.DrawBitmap( m_frame, x, y, true);
}

