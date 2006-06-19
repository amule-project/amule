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

#ifndef MULEGIFCTRL_H
#define MULEGIFCTRL_H

#include <wx/control.h>
#include <wx/timer.h>

const int GIFTIMERID = 271283;

class wxGIFDecoder;
class wxBitmap;

/**
 * MuleGifCtrl is a simple widget for displaying a gif animation.
 * It is based on the animation classes by Julian Smart and 
 * Guillermo Rodriguez Garcia, but is specialized for the reduced
 * requirements of the aMule project. It provides flicker-free
 * redrawing using wxBufferedPaintDC.
 *
 * To reduce complexity, several things have been hardcoded, though
 * they can easily be changed: 
 *  - The animation will continue to loop until Stop() is called.
 *  - The gif image is assumed to be transparent.
 *  - Start will start the animation from the first frame and wont
 *     continue a stopped animation.
 */
class MuleGifCtrl : public wxControl
{
public:
	/**
	 * Contructor. See wxWindow class documentation for more information.
	 */
	MuleGifCtrl( wxWindow *parent, wxWindowID id, 
	              const wxPoint& pos = wxDefaultPosition,
                  const wxSize& size = wxDefaultSize,
                  long style = 0,
                  const wxValidator& validator = wxDefaultValidator,
                  const wxString& name = wxControlNameStr );
				  
	/**
	 * Destructor
	 */
	virtual ~MuleGifCtrl();

	/**
	 * This loads the gif image from a char-array with a specific size.
	 *
	 * @param data The array containing the image.
	 * @param size The size of the array.
	 * @return Returns true if the data was loaded, false otherwise.
	 *
	 * This sets the current animation and displays the first frame. If another
	 * animation was loaded, it will be unloaded and the animation stopped.
	 *
	 * To convert a image to a format readable by this function, you can
	 * use the utility hexdump. Look at inetdownload.h for how to format
	 * the output.
	 */
	bool LoadData(const char* data, int size);
	
	/**
	 * This function starts playing the animation provided that a animation is
	 * set and it's not a static image.
	 */
	void Start();

	/**
	 * Stops the animation.
	 */
	void Stop();

	/**
	 * Returns the prefered size of the widget.
	 *
	 * @return Prefered size, which is the size of the animation.
	 */
	virtual wxSize GetBestSize();

private:
	/**
	 * Timer function that selects the next frame in an animation.
	 */
	void OnTimer( wxTimerEvent& event );

	/**
	 * Function for drawing the animation.
	 * 
	 * This functions draws the current frame, which is changed in OnTimer(),
	 * using a wxBufferedPaintDC. By doing so and also catching the 
	 * ERASE_BACKGROUND events we avoid flickering on redraws.
	 */
	void OnPaint( wxPaintEvent& event );

	/**
	 * This function is used to avoid flicker when redrawing.
	 */
	void OnErase( wxEraseEvent& WXUNUSED(event) ) {}

	//! A pointer to the current gif-animation.
	wxGIFDecoder*	m_decoder;
	//! Timer used for the delay between each frame.
	wxTimer			m_timer;
	//! Current frame.
	wxBitmap		m_frame;
    
	//! Enables the event functions OnErase(), OnTimer() and OnPaint().
	DECLARE_EVENT_TABLE()
};

#endif

// File_checked_for_headers
