//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
// Copyright (c) 2002-2011 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "CaptchaGenerator.h"
#include "RandomFunctions.h"
#include "MemFile.h"
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/font.h>


#define LETTERSIZE  32
#define CROWDEDSIZE 20

// fairly simply captcha generator, might be improved is spammers think its really worth it solving captchas on aMule

CCaptchaGenerator::CCaptchaGenerator(uint32 nLetterCount)
{
	ReGenerateCaptcha(nLetterCount);
}

void CCaptchaGenerator::ReGenerateCaptcha(uint32 nLetterCount)
{
	static wxString schCaptchaContent = wxT("ABCDEFGHJKLMNPQRSTUVWXYZ123456789"); 
	m_strCaptchaText.Clear();
	// Bitmap must be created with full depth, or it will fail on GTK
	wxBitmap pimgResult(LETTERSIZE + (nLetterCount-1)*CROWDEDSIZE, 36);
	wxMemoryDC dc(pimgResult);
	dc.SetBackground(*wxWHITE_BRUSH);
	dc.Clear();
	dc.SetTextForeground(*wxBLACK);
	dc.SetTextBackground(*wxWHITE);
	double lastrotate = 15.0;

	for (uint32 i = 0; i < nLetterCount; i++) {
		wxString strLetter(schCaptchaContent[GetRandomUint16() % schCaptchaContent.length()]);
		m_strCaptchaText += strLetter;
		
		uint16 nRandomSize = 30 - GetRandomUint16() % 12;
		wxFont font(nRandomSize, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
		dc.SetFont(font);
		uint16 nRandomOffset = 3 + GetRandomUint16() % 8;
		uint16 maxRotate = 50 - nRandomSize; // rotate small letters more than large letters
		double fRotate = (double)(maxRotate - (GetRandomUint16() % (maxRotate*2)));
		// limit angle diff - it causes too much overlap since wx rotates at the corner
		// (maybe I'll redo that with some coordinate transformation one day)
		if (fRotate - lastrotate > 20) {
			fRotate = lastrotate + 20;
		} else if (fRotate - lastrotate < -20) {
			fRotate = lastrotate - 20;
		}
		dc.DrawRotatedText(strLetter, nRandomOffset + i * CROWDEDSIZE, 0, fRotate);
	}
	m_pimgCaptcha = pimgResult.ConvertToImage();
	// wx always saves as 24 bpp except when it gets this WELL DOCUMENTED option...
	m_pimgCaptcha.SetOption(wxIMAGE_OPTION_BMP_FORMAT, wxBMP_1BPP);
	// m_pimgCaptcha.SaveFile(wxT("captcha.bmp"), wxBITMAP_TYPE_BMP);
}

bool CCaptchaGenerator::WriteCaptchaImage(wxMemoryOutputStream& file)
{
	return m_pimgCaptcha.SaveFile(file, wxBITMAP_TYPE_BMP);
}
