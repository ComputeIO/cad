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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/radiobut.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_COMMON_FILE_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_COMMON_FILE_SETTINGS_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_staticTextautosave;
		wxSpinCtrl* m_SaveTime;
		wxStaticText* m_staticTextFileHistorySize;
		wxSpinCtrl* m_fileHistorySize;
		wxCheckBox* m_preferCompressedPcb;
		wxStaticText* compressionLabel;
		wxSpinCtrl* m_compressionLevel;
		wxTextCtrl* m_textEditorPath;
		wxBitmapButton* m_textEditorBtn;
		wxRadioButton* m_defaultPDFViewer;
		wxRadioButton* m_otherPDFViewer;
		wxTextCtrl* m_PDFViewerPath;
		wxBitmapButton* m_pdfViewerBtn;

		// Virtual event handlers, overide them in your derived class
		virtual void OnTextEditorClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onUpdateUIPdfPath( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnPDFViewerClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_COMMON_FILE_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_COMMON_FILE_SETTINGS_BASE();

};

