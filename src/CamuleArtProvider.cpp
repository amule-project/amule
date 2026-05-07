//
// CamuleArtProvider — see CamuleArtProvider.h for the contract.
//

#include "CamuleArtProvider.h"

#include "icons/icon_data.h"

#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/mstream.h>


const wxString CamuleArtProvider::PREFIX = "amule:";


wxBitmap CamuleArtProvider::CreateBitmap(const wxArtID& id,
                                          const wxArtClient& WXUNUSED(client),
                                          const wxSize& size)
{
	// Only resolve our own art ids; let other providers handle the rest.
	if (!id.StartsWith(PREFIX)) {
		return wxNullBitmap;
	}

	const wxString short_name = id.Mid(PREFIX.length());
	const struct AMuleIconEntry *entry =
		amule_find_icon(short_name.utf8_str().data());
	if (entry == NULL) {
		return wxNullBitmap;
	}

	// Decode the embedded PNG bytes.  wxImage::LoadFile via a
	// wxMemoryInputStream wins over wxBitmap::NewFromPNGData here
	// because we need to optionally rescale below — Scale() is on
	// wxImage, not wxBitmap.
	wxMemoryInputStream stream(entry->png_data, entry->png_len);
	wxImage image;
	if (!image.LoadFile(stream, wxBITMAP_TYPE_PNG)) {
		return wxNullBitmap;
	}

	// Honour an explicit size request from the caller.  GetBitmap()
	// passes wxDefaultSize when the caller doesn't care; in that
	// case the natural size from the PNG header wins.
	if (size != wxDefaultSize
	    && (size.GetWidth() != image.GetWidth()
	        || size.GetHeight() != image.GetHeight())) {
		image = image.Scale(size.GetWidth(), size.GetHeight(),
		                    wxIMAGE_QUALITY_HIGH);
	}

	return wxBitmap(image);
}
