///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Dec 26 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "widgets/stepped_slider.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COMMON_UI_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COMMON_UI_SETTINGS_BASE : public wxPanel
{
	private:

	protected:
		enum
		{
			wxID_AUTOPAN = 1000
		};

		wxChoice* m_antialiasing;
		wxChoice* m_antialiasingFallback;
		wxStaticText* m_staticTexticonscale;
		STEPPED_SLIDER* m_iconScaleSlider;
		wxCheckBox* m_iconScaleAuto;
		wxStaticText* m_staticTextCanvasScale;
		wxSpinCtrlDouble* m_canvasScaleCtrl;
		wxCheckBox* m_canvasScaleAuto;
		wxCheckBox* m_checkBoxIconsInMenus;
		wxCheckBox* m_ZoomCenterOpt;
		wxCheckBox* m_MousewheelPANOpt;
		wxCheckBox* m_AutoPANOpt;
		wxCheckBox* m_PreferSelectToDrag;
		wxCheckBox* m_warpMouseOnMove;
		wxCheckBox* m_NonImmediateActions;

		// Virtual event handlers, overide them in your derived class
		virtual void OnScaleSlider( wxScrollEvent& event ) { event.Skip(); }
		virtual void OnIconScaleAuto( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCanvasScaleAuto( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COMMON_UI_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_COMMON_UI_SETTINGS_BASE();

};

