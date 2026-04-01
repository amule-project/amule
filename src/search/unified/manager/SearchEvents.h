//								-*- C++ -*-
// This file is part of the aMule Project.
//
// Copyright (c) 2024 aMule Team
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

#ifndef SEARCH_EVENTS_H
#define SEARCH_EVENTS_H

#include <wx/event.h>

// Custom event type for search events
wxDECLARE_EVENT(wxEVT_SEARCH_EVENT, wxCommandEvent);

/**
 * Container for serialized search event data
 */
class wxClientDataContainer : public wxClientData {
public:
    explicit wxClientDataContainer(const std::vector<uint8_t>& data)
        : m_data(data)
    {}

    const std::vector<uint8_t>& GetData() const { return m_data; }

private:
    std::vector<uint8_t> m_data;
};

#endif // SEARCH_EVENTS_H
