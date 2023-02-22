///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/wx_html_report_box.h"

#include "panel_pcbnew_simul_DC_power_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PCBNEW_SIMUL_DC_POWER_BASE::PANEL_PCBNEW_SIMUL_DC_POWER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer101;
	bSizer101 = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* bSizer32;
	bSizer32 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText312 = new wxStaticText( this, wxID_ANY, wxT("Select a net"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText312->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer2->Add( m_staticText312, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10 );


	bSizer2->Add( 0, 0, 1, wxEXPAND, 5 );

	m_netComboBox = new wxComboBox( this, wxID_ANY, wxT("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	bSizer2->Add( m_netComboBox, 4, wxALL|wxEXPAND, 5 );


	bSizer32->Add( bSizer2, 0, wxEXPAND, 5 );

	m_staticText1111 = new wxStaticText( this, wxID_ANY, wxT("NOTE : Unit is V for sources, A for sinks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1111->Wrap( -1 );
	bSizer32->Add( m_staticText1111, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_scrolledWindow1 = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
	m_scrolledWindow1->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxHORIZONTAL );

	m_padGrid = new wxGrid( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_padGrid->CreateGrid( 5, 5 );
	m_padGrid->EnableEditing( true );
	m_padGrid->EnableGridLines( true );
	m_padGrid->EnableDragGridSize( false );
	m_padGrid->SetMargins( 0, 0 );

	// Columns
	m_padGrid->EnableDragColMove( false );
	m_padGrid->EnableDragColSize( true );
	m_padGrid->SetColLabelValue( 0, wxT("Component") );
	m_padGrid->SetColLabelValue( 1, wxT("Pad") );
	m_padGrid->SetColLabelValue( 2, wxT("Type") );
	m_padGrid->SetColLabelValue( 3, wxT("Value") );
	m_padGrid->SetColLabelValue( 4, wxT("Unit") );
	m_padGrid->SetColLabelValue( 5, wxEmptyString );
	m_padGrid->SetColLabelSize( 30 );
	m_padGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_padGrid->EnableDragRowSize( true );
	m_padGrid->SetRowLabelSize( 80 );
	m_padGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_padGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer8->Add( m_padGrid, 1, wxEXPAND, 5 );


	m_scrolledWindow1->SetSizer( bSizer8 );
	m_scrolledWindow1->Layout();
	bSizer8->Fit( m_scrolledWindow1 );
	bSizer32->Add( m_scrolledWindow1, 1, wxEXPAND | wxALL, 5 );

	m_staticline11 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer32->Add( m_staticline11, 0, wxEXPAND | wxALL, 5 );

	m_scrolledWindow3 = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_scrolledWindow3->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer10;
	bSizer10 = new wxBoxSizer( wxHORIZONTAL );

	m_resultGrid = new wxGrid( m_scrolledWindow3, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_resultGrid->CreateGrid( 5, 4 );
	m_resultGrid->EnableEditing( false );
	m_resultGrid->EnableGridLines( true );
	m_resultGrid->EnableDragGridSize( false );
	m_resultGrid->SetMargins( 0, 0 );

	// Columns
	m_resultGrid->EnableDragColMove( false );
	m_resultGrid->EnableDragColSize( true );
	m_resultGrid->SetColLabelValue( 0, wxT("Component") );
	m_resultGrid->SetColLabelValue( 1, wxT("Pad") );
	m_resultGrid->SetColLabelValue( 2, wxT("Value") );
	m_resultGrid->SetColLabelValue( 3, wxT("Unit") );
	m_resultGrid->SetColLabelValue( 4, wxEmptyString );
	m_resultGrid->SetColLabelSize( 30 );
	m_resultGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_resultGrid->EnableDragRowSize( true );
	m_resultGrid->SetRowLabelSize( 80 );
	m_resultGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_resultGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer10->Add( m_resultGrid, 1, wxEXPAND, 5 );


	m_scrolledWindow3->SetSizer( bSizer10 );
	m_scrolledWindow3->Layout();
	bSizer10->Fit( m_scrolledWindow3 );
	bSizer32->Add( m_scrolledWindow3, 1, wxEXPAND | wxALL, 5 );


	bSizer101->Add( bSizer32, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer11;
	bSizer11 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	wxString m_radioBox1Choices[] = { wxT("2.5D: Each layer is a 2D surface, vias are simulated using their resistance. Simulation is faster"), wxT("3D : Layers have thickness, vias are simulated based on their geometry. Simulation is slower") };
	int m_radioBox1NChoices = sizeof( m_radioBox1Choices ) / sizeof( wxString );
	m_radioBox1 = new wxRadioBox( this, wxID_ANY, wxT("wxRadioBox"), wxDefaultPosition, wxDefaultSize, m_radioBox1NChoices, m_radioBox1Choices, 1, wxRA_SPECIFY_COLS );
	m_radioBox1->SetSelection( 0 );
	m_radioBox1->Enable( false );

	bSizer3->Add( m_radioBox1, 0, wxALL, 5 );

	m_checkBox2211 = new wxCheckBox( this, wxID_ANY, wxT("Report via stress analysis"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBox2211->Enable( false );

	bSizer3->Add( m_checkBox2211, 0, wxALL, 5 );


	bSizer11->Add( bSizer3, 0, 0, 5 );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_mapPotential = new wxCheckBox( this, wxID_ANY, wxT("Generate 3D map : potential drop"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mapPotential->SetValue(true);
	fgSizer1->Add( m_mapPotential, 0, wxALL, 5 );

	m_mapCurrentDensity = new wxCheckBox( this, wxID_ANY, wxT("Generate 3D map : current density"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mapCurrentDensity->SetValue(true);
	fgSizer1->Add( m_mapCurrentDensity, 0, wxALL, 5 );

	m_mapPowerDensity = new wxCheckBox( this, wxID_ANY, wxT("Generate 3D map : power density density"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer1->Add( m_mapPowerDensity, 0, wxALL, 5 );

	m_textReport = new wxCheckBox( this, wxID_ANY, wxT("Generate text report"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textReport->SetValue(true);
	m_textReport->Enable( false );

	fgSizer1->Add( m_textReport, 0, wxALL, 5 );


	bSizer11->Add( fgSizer1, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer41;
	bSizer41 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText12 = new wxStaticText( this, wxID_ANY, wxT("Select a folder for outputs:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizer41->Add( m_staticText12, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_dirPicker11 = new wxDirPickerCtrl( this, wxID_ANY, wxEmptyString, wxT("Select a folder"), wxDefaultPosition, wxDefaultSize, wxDIRP_DEFAULT_STYLE );
	m_dirPicker11->Enable( false );

	bSizer41->Add( m_dirPicker11, 0, wxALL, 5 );


	bSizer31->Add( bSizer41, 1, wxEXPAND, 5 );


	bSizer11->Add( bSizer31, 0, wxEXPAND, 5 );

	m_3DviewerSizer = new wxBoxSizer( wxVERTICAL );


	bSizer11->Add( m_3DviewerSizer, 3, wxALL|wxEXPAND, 5 );

	m_SimulReportHTML = new WX_HTML_REPORT_BOX( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO );
	bSizer11->Add( m_SimulReportHTML, 1, wxALL|wxEXPAND, 5 );


	bSizer101->Add( bSizer11, 2, wxEXPAND, 5 );


	bSizer1->Add( bSizer101, 1, wxEXPAND, 5 );

	m_button1 = new wxButton( this, wxID_ANY, wxT("Run"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( m_button1, 0, wxALL, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();

	// Connect Events
	m_netComboBox->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( PANEL_PCBNEW_SIMUL_DC_POWER_BASE::onNetSelect ), NULL, this );
	m_button1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_SIMUL_DC_POWER_BASE::OnRun ), NULL, this );
}

PANEL_PCBNEW_SIMUL_DC_POWER_BASE::~PANEL_PCBNEW_SIMUL_DC_POWER_BASE()
{
	// Disconnect Events
	m_netComboBox->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( PANEL_PCBNEW_SIMUL_DC_POWER_BASE::onNetSelect ), NULL, this );
	m_button1->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_SIMUL_DC_POWER_BASE::OnRun ), NULL, this );

}
