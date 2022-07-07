///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class SIM_PLOT_AXIS_HORIZONTAL_BASE
///////////////////////////////////////////////////////////////////////////////
class SIM_PLOT_AXIS_HORIZONTAL_BASE : public wxDialog
{
	private:

	protected:
		wxStaticText* m_staticText1;
		wxTextCtrl* m_quantityCtrl;
		wxStaticText* m_staticText2;
		wxTextCtrl* m_leftCtrl;
		wxStaticText* m_staticText21;
		wxTextCtrl* m_ticksCtrl;
		wxStaticText* m_staticText211;
		wxTextCtrl* m_rightCtrl;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnOKButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:
		wxCheckBox* m_logCheckbox;

		SIM_PLOT_AXIS_HORIZONTAL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Horizontal axis"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );

		~SIM_PLOT_AXIS_HORIZONTAL_BASE();

};

