//
// CamuleArtProvider — wxArtProvider subclass exposing aMule's bundled
// PNG icons through wxArtProvider::GetBitmap() / GetIcon().
//
// The icon data itself lives in src/icons/icon_data.c (build-time
// generated from src/icons/*.png by src/icons/embed_icons.py).
// CamuleArtProvider just glues that lookup table to the wx art-id
// system: art ids of the form "amule:<name>" map to the PNG entry of
// the same name.  Examples: "amule:amule", "amule:sort_dn",
// "amule:flag_us".
//
// Push one instance during app init (CamuleApp::OnInit and
// CamuleRemoteGuiApp::OnInit do this) so every later
// `wxArtProvider::GetBitmap("amule:<name>", ...)` resolves through it.
//

#ifndef SRC_CAMULE_ART_PROVIDER_H
#define SRC_CAMULE_ART_PROVIDER_H

#include <wx/artprov.h>

class CamuleArtProvider : public wxArtProvider
{
public:
	// Prefix every aMule art id starts with — e.g. "amule:sort_dn".
	static const wxString PREFIX;

	// Build a full art id from a short icon name.  E.g.
	// CamuleArtProvider::MakeId("sort_dn") -> "amule:sort_dn".
	static wxString MakeId(const wxString& name) { return PREFIX + name; }

protected:
	wxBitmap CreateBitmap(const wxArtID& id,
	                      const wxArtClient& client,
	                      const wxSize& size) override;
};

#endif // SRC_CAMULE_ART_PROVIDER_H
