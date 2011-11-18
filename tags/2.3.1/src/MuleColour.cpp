//
// This file is part of the aMule Project.
//
// Copyright (c) 2003-2011 aMule Team ( admin@amule.org / http://www.amule.org )
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

#include "MuleColour.h"

#include <wx/pen.h>
#include <wx/brush.h>

#include <map>

#define USE_MULE_PEN_CACHE 1
#define USE_MULE_BRUSH_CACHE 1

std::map<uint32_t, wxPen*> wxPenCache; 
std::map<uint32_t, wxBrush*> wxBrushCache;

const wxPen& CMuleColour::GetPen(int width, int style) const
{
#if USE_MULE_PEN_CACHE
	wxPen* result = NULL;
	
	if (m_cachedpen && (m_cachedpen->GetWidth() == width) && (m_cachedpen->GetStyle() == style)) {
		result = m_cachedpen;
	} else {
		const uint32_t hash = ((width & 0xF) << 28) | ((style & 0xF) << 24) | (GetULong() & 0xFFFFFF);
		std::map<uint32_t, wxPen*>::iterator it = wxPenCache.find(hash);
		if (it != wxPenCache.end()) {
			result = it->second;
			m_cachedpen = result;
		} else {
			result = wxThePenList->FindOrCreatePen(wxColour(m_red, m_green, m_blue), width, style);
			m_cachedpen = result;
			wxPenCache.insert(std::pair<uint32_t, wxPen*>(hash, result));
		}
	}
	
	return *result;
#else 
	return *wxThePenList->FindOrCreatePen(wxColour(m_red, m_green, m_blue), width, style);
#endif
}

const wxBrush& CMuleColour::GetBrush(int style) const
{
#if USE_MULE_BRUSH_CACHE	
	wxBrush* result = NULL;
	
	if (m_cachedbrush && (m_cachedbrush->GetStyle() == style)) {
		result = m_cachedbrush;
	} else {
		const uint32_t hash = ((style & 0xF) << 24) | (GetULong() & 0xFFFFFF);
		std::map<uint32_t, wxBrush*>::iterator it = wxBrushCache.find(hash);
		if (it != wxBrushCache.end()) {
			result = it->second;
			m_cachedbrush = result;
		} else {
			result = wxTheBrushList->FindOrCreateBrush(wxColour(m_red, m_green, m_blue), style);
			m_cachedbrush = result;
			wxBrushCache.insert(std::pair<uint32_t, wxBrush*>(hash, result));
		}
	}
	
	return *result;
#else
	return *wxTheBrushList->FindOrCreateBrush(wxColour(m_red, m_green, m_blue), style);
#endif
}
