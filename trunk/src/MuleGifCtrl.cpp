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

#include <wx/mstream.h>
#include <wx/gifdecod.h>
#include <wx/dcbuffer.h>

#include "MuleGifCtrl.h"

BEGIN_EVENT_TABLE(MuleGifCtrl, wxControl)
	EVT_TIMER(GIFTIMERID, MuleGifCtrl::OnTimer)
	EVT_PAINT(MuleGifCtrl::OnPaint)
	EVT_ERASE_BACKGROUND(MuleGifCtrl::OnErase)
END_EVENT_TABLE()

#if wxCHECK_VERSION(2, 7, 1)
// Wrapper that emulates old wxGIFDecoder API

class MuleGIFDecoder : public wxGIFDecoder {
public:
	MuleGIFDecoder(wxInputStream* stream, bool dummy) {
		m_stream = stream;
		dummy = dummy; // Unused.
		m_nframe = 0;
	}
	
	~MuleGIFDecoder() { /* don't delete the stream! */ }
	
	wxGIFErrorCode ReadGIF() {
		return LoadGIF(*m_stream);
	}
	
	void GoFirstFrame() { m_nframe = 0; }
	void GoNextFrame(bool dummy) { m_nframe < GetFrameCount() ? m_nframe++ : m_nframe = 0; }
	void GoLastFrame() { m_nframe = GetFrameCount(); }
	
	void ConvertToImage(wxImage* image) { wxGIFDecoder::ConvertToImage(m_nframe, image); }
	
	size_t GetLogicalScreenWidth() { return GetAnimationSize().GetWidth(); }
	size_t GetLogicalScreenHeight() { return GetAnimationSize().GetHeight(); }
	
	size_t	GetLeft() { return 0; }
	size_t	GetTop() { return 0; }
	
	long GetDelay() { return wxGIFDecoder::GetDelay(m_nframe); }
	
private:
	uint32_t m_nframe;
	wxInputStream* m_stream;
};

#else
#define MuleGIFDecoder wxGIFDecoder
#endif



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
}


bool MuleGifCtrl::LoadData(const char* data, int size)
{
	if (m_decoder) {
		m_timer.Stop();
   		delete m_decoder;
  		m_decoder = NULL;
	}

  	wxMemoryInputStream stream(data, size);
  	m_decoder = new MuleGIFDecoder(&stream, TRUE);
  	if ( m_decoder->ReadGIF() != wxGIF_OK ) {
   		delete m_decoder;
   		m_decoder = NULL;
   		return false;
  	}

	m_decoder->GoFirstFrame();
	wxImage frame;
	m_decoder->ConvertToImage( &frame );
	m_frame = wxBitmap(frame);

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
	return wxSize( m_decoder->GetLogicalScreenWidth(), m_decoder->GetLogicalScreenHeight() );
}


void MuleGifCtrl::OnTimer( wxTimerEvent& WXUNUSED(event) )
{
	if ( m_decoder ) {
		if ( m_decoder->IsAnimation() )
			m_decoder->GoNextFrame( true );

		wxImage frame;
		m_decoder->ConvertToImage( &frame );
		m_frame = wxBitmap(frame);

		Refresh();

		if ( m_decoder->IsAnimation() )
			m_timer.Start( m_decoder->GetDelay(), true );
	}
}


void MuleGifCtrl::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
	wxBufferedPaintDC dc(this);

    wxSize size = GetClientSize();
	int x = (size.GetWidth()-m_decoder->GetLogicalScreenWidth())/2;
	int y = (size.GetHeight()-m_decoder->GetLogicalScreenHeight())/2;

	dc.SetBackground( wxBrush( GetBackgroundColour(), wxSOLID ));
	dc.Clear();
	dc.DrawBitmap( m_frame, x + m_decoder->GetLeft(), y + m_decoder->GetTop(), true);
}

// File_checked_for_headers
