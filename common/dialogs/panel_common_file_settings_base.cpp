///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 Dec 26 2019)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_common_file_settings_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_COMMON_FILE_SETTINGS_BASE::PANEL_COMMON_FILE_SETTINGS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bPanelSizer;
	bPanelSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bLeftSizer;
	bLeftSizer = new wxBoxSizer( wxVERTICAL );

	wxGridBagSizer* gbSizer1;
	gbSizer1 = new wxGridBagSizer( 4, 4 );
	gbSizer1->SetFlexibleDirection( wxBOTH );
	gbSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gbSizer1->SetEmptyCellSize( wxSize( -1,2 ) );

	m_staticTextautosave = new wxStaticText( this, wxID_ANY, _("&Auto save interval:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextautosave->Wrap( -1 );
	gbSizer1->Add( m_staticTextautosave, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxHORIZONTAL );

	m_SaveTime = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 10, 0 );
	m_SaveTime->SetToolTip( _("Delay after the first change to create a backup file of the board on disk.\nIf set to 0, auto backup is disabled") );

	bSizer6->Add( m_SaveTime, 0, wxALIGN_CENTER_VERTICAL, 5 );

	wxStaticText* minutesLabel;
	minutesLabel = new wxStaticText( this, wxID_ANY, _("minutes"), wxDefaultPosition, wxDefaultSize, 0 );
	minutesLabel->Wrap( -1 );
	bSizer6->Add( minutesLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );


	gbSizer1->Add( bSizer6, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxEXPAND, 5 );

	m_staticTextFileHistorySize = new wxStaticText( this, wxID_ANY, _("File history size:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticTextFileHistorySize->Wrap( -1 );
	gbSizer1->Add( m_staticTextFileHistorySize, wxGBPosition( 1, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_fileHistorySize = new wxSpinCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 0 );
	gbSizer1->Add( m_fileHistorySize, wxGBPosition( 1, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	gbSizer1->AddGrowableCol( 1 );

	bLeftSizer->Add( gbSizer1, 0, wxEXPAND|wxALL, 10 );

	wxStaticBoxSizer* sizerCompression;
	sizerCompression = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("File Compression") ), wxVERTICAL );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );

	m_preferCompressedPcb = new wxCheckBox( sizerCompression->GetStaticBox(), wxID_ANY, _("Prefer compressed PCB files"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_preferCompressedPcb, 0, wxALL, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	compressionLabel = new wxStaticText( sizerCompression->GetStaticBox(), wxID_ANY, _("Compression level:"), wxDefaultPosition, wxDefaultSize, 0 );
	compressionLabel->Wrap( -1 );
	bSizer5->Add( compressionLabel, 0, wxALIGN_CENTER|wxLEFT|wxRIGHT, 5 );

	m_compressionLevel = new wxSpinCtrl( sizerCompression->GetStaticBox(), wxID_ANY, wxT("6"), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9, 0 );
	bSizer5->Add( m_compressionLevel, 0, wxALIGN_CENTER, 5 );


	bSizer4->Add( bSizer5, 1, wxEXPAND, 5 );


	sizerCompression->Add( bSizer4, 1, wxEXPAND, 5 );


	bLeftSizer->Add( sizerCompression, 1, wxALL|wxEXPAND, 5 );

	wxStaticBoxSizer* sizerHelperApps;
	sizerHelperApps = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, _("Helper Applications") ), wxVERTICAL );

	wxGridBagSizer* gridHelperApps;
	gridHelperApps = new wxGridBagSizer( 3, 3 );
	gridHelperApps->SetFlexibleDirection( wxBOTH );
	gridHelperApps->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	gridHelperApps->SetEmptyCellSize( wxSize( -1,5 ) );

	wxStaticText* textEditorLabel;
	textEditorLabel = new wxStaticText( sizerHelperApps->GetStaticBox(), wxID_ANY, _("Text editor:"), wxDefaultPosition, wxDefaultSize, 0 );
	textEditorLabel->Wrap( -1 );
	gridHelperApps->Add( textEditorLabel, wxGBPosition( 0, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 4 );

	m_textEditorPath = new wxTextCtrl( sizerHelperApps->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_textEditorPath->SetMinSize( wxSize( 280,-1 ) );

	gridHelperApps->Add( m_textEditorPath, wxGBPosition( 0, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_textEditorBtn = new wxBitmapButton( sizerHelperApps->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_textEditorBtn->SetMinSize( wxSize( 29,29 ) );

	gridHelperApps->Add( m_textEditorBtn, wxGBPosition( 0, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_defaultPDFViewer = new wxRadioButton( sizerHelperApps->GetStaticBox(), wxID_ANY, _("System default PDF viewer"), wxDefaultPosition, wxDefaultSize, 0 );
	gridHelperApps->Add( m_defaultPDFViewer, wxGBPosition( 2, 0 ), wxGBSpan( 1, 3 ), 0, 4 );

	m_otherPDFViewer = new wxRadioButton( sizerHelperApps->GetStaticBox(), wxID_ANY, _("Other:"), wxDefaultPosition, wxDefaultSize, 0 );
	gridHelperApps->Add( m_otherPDFViewer, wxGBPosition( 3, 0 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 4 );

	m_PDFViewerPath = new wxTextCtrl( sizerHelperApps->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_PDFViewerPath->SetMinSize( wxSize( 280,-1 ) );

	gridHelperApps->Add( m_PDFViewerPath, wxGBPosition( 3, 1 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );

	m_pdfViewerBtn = new wxBitmapButton( sizerHelperApps->GetStaticBox(), wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW|0 );
	m_pdfViewerBtn->SetMinSize( wxSize( 29,29 ) );

	gridHelperApps->Add( m_pdfViewerBtn, wxGBPosition( 3, 2 ), wxGBSpan( 1, 1 ), wxALIGN_CENTER_VERTICAL, 5 );


	gridHelperApps->AddGrowableCol( 1 );

	sizerHelperApps->Add( gridHelperApps, 0, wxEXPAND|wxBOTTOM|wxLEFT, 5 );


	bLeftSizer->Add( sizerHelperApps, 0, wxALL|wxEXPAND, 5 );


	bLeftSizer->Add( 0, 0, 1, wxEXPAND, 5 );


	bPanelSizer->Add( bLeftSizer, 1, wxEXPAND|wxBOTTOM, 5 );

	wxBoxSizer* bSizer61;
	bSizer61 = new wxBoxSizer( wxVERTICAL );


	bSizer61->Add( 0, 0, 1, wxEXPAND, 5 );


	bPanelSizer->Add( bSizer61, 1, wxEXPAND, 5 );


	this->SetSizer( bPanelSizer );
	this->Layout();
	bPanelSizer->Fit( this );

	// Connect Events
	m_textEditorBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMMON_FILE_SETTINGS_BASE::OnTextEditorClick ), NULL, this );
	m_PDFViewerPath->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_COMMON_FILE_SETTINGS_BASE::onUpdateUIPdfPath ), NULL, this );
	m_pdfViewerBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMMON_FILE_SETTINGS_BASE::OnPDFViewerClick ), NULL, this );
	m_pdfViewerBtn->Connect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_COMMON_FILE_SETTINGS_BASE::onUpdateUIPdfPath ), NULL, this );
}

PANEL_COMMON_FILE_SETTINGS_BASE::~PANEL_COMMON_FILE_SETTINGS_BASE()
{
	// Disconnect Events
	m_textEditorBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMMON_FILE_SETTINGS_BASE::OnTextEditorClick ), NULL, this );
	m_PDFViewerPath->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_COMMON_FILE_SETTINGS_BASE::onUpdateUIPdfPath ), NULL, this );
	m_pdfViewerBtn->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_COMMON_FILE_SETTINGS_BASE::OnPDFViewerClick ), NULL, this );
	m_pdfViewerBtn->Disconnect( wxEVT_UPDATE_UI, wxUpdateUIEventHandler( PANEL_COMMON_FILE_SETTINGS_BASE::onUpdateUIPdfPath ), NULL, this );

}
