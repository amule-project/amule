#include <wx/textctrl.h>


class wxMenuEvent;
class wxMouseEvent;


/**
 * This class is a slightly improved wxTextCtrl that supports the traditional
 * popup-menu usually provided by text-ctrls. It provides the following options:
 *  - Cut
 *  - Copy
 *  - Paste
 *  - Clear
 *  - Selected All
 *
 * Other than that, it acts exactly like an ordinary wxTextCtrl.
 */
class CMuleTextCtrl : public wxTextCtrl
{
public:
	/**
	 * Constructor is identical to the wxTextCtrl one.
	 */
	CMuleTextCtrl(wxWindow* parent, wxWindowID id, const wxString& value = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, const wxValidator& validator = wxDefaultValidator, const wxString& name = wxTextCtrlNameStr);

	/**
	 * Destructor, which currently does nothing.
	 */
	virtual ~CMuleTextCtrl() {};


protected:
	/**
	 * This function takes care of creating the popup-menu.
	 *
	 * Please note that by using the RIGHT_DOWN event, I'm disabling the second
	 * type of selection that the wxTextCtrl supports. However, I frankly only 
	 * noticed that second selection type while implementing this, so I doubth 
	 * that anyone will be missing it ...
	 */
	void OnRightDown( wxMouseEvent& evt );

	/**
	 * This function takes care of pasting text.
	 *
	 * Pleaes note that it is only needed because wxMenu disallows enabling and
	 * disabling of items that use the predefined wxID_PASTE id. This is the 
	 * only one of the already provided commands we need to override, since the 
	 * others already work just fine.
	 */
	void OnPaste( wxMenuEvent& evt );
	
	/**
	 * This functions takes care of selecting all text.
	 */
	void OnSelAll( wxMenuEvent& evt );
	
	/**
	 * This functions takes care of clearing the text.
	 */ 
	void OnClear( wxMenuEvent& evt );


	DECLARE_EVENT_TABLE()
};


