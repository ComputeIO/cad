///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May 14 2021)
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
#include <wx/combobox.h>
#include <wx/sizer.h>
#include <wx/grid.h>
#include <wx/statline.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/filepicker.h>
#include <wx/dataview.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PCBNEW_SIMUL_DC_POWER_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PCBNEW_SIMUL_DC_POWER_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_staticText3111;
		wxStaticText* m_staticText312;
		wxComboBox* m_netComboBox;
		wxStaticText* m_staticText1111;
		wxGrid* m_padGrid;
		wxStaticLine* m_staticline111;
		wxRadioBox* m_radioBox1;
		wxCheckBox* m_checkBox2211;
		wxStaticLine* m_staticline11;
		wxStaticText* m_staticText31;
		wxCheckBox* m_checkBox212;
		wxCheckBox* m_checkBox22;
		wxCheckBox* m_checkBox221;
		wxCheckBox* m_checkBox2111;
		wxStaticText* m_staticText111;
		wxStaticText* m_staticText12;
		wxDirPickerCtrl* m_dirPicker11;
		wxDataViewCtrl* m_dataViewCtrl1;
		wxButton* m_button1;

		// Virtual event handlers, overide them in your derived class
		virtual void onNetSelect( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRun( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_PCBNEW_SIMUL_DC_POWER_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 693,576 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_PCBNEW_SIMUL_DC_POWER_BASE();

};

