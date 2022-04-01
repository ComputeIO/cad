///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class UNIT_SELECTOR_LEN;

#include "calculator_panels/calculator_panel.h"
#include <wx/string.h>
#include <wx/radiobox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_CABLE_SIZE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_CABLE_SIZE_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxRadioBox* m_radioUnits;
		wxStaticText* m_staticText16;
		wxTextCtrl* m_currentCtrl;
		wxStaticText* m_staticText21;
		wxStaticText* m_staticText161;
		wxTextCtrl* m_lengthCtrl;
		UNIT_SELECTOR_LEN* m_choiceUnit;
		wxScrolledWindow* m_scrolledWindow1;
		wxGrid* m_table;

		// Virtual event handlers, override them in your derived class
		virtual void OnUnitChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCurrentChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnLengthChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnChangeUnitLen( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_CABLE_SIZE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 677,453 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_CABLE_SIZE_BASE();

};

