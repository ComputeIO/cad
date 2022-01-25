///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "noname.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PCBNEW_SIMUL_DC_POWER_BASE::PANEL_PCBNEW_SIMUL_DC_POWER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer32;
	bSizer32 = new wxBoxSizer( wxVERTICAL );

	m_staticText3111 = new wxStaticText( this, wxID_ANY, wxT("Electrical"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3111->Wrap( -1 );
	m_staticText3111->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer32->Add( m_staticText3111, 0, wxALL, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText312 = new wxStaticText( this, wxID_ANY, wxT("Select a net"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText312->Wrap( -1 );
	m_staticText312->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer2->Add( m_staticText312, 0, wxALL, 5 );

	m_netComboBox = new wxComboBox( this, wxID_ANY, wxT("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	bSizer2->Add( m_netComboBox, 0, wxALL, 5 );


	bSizer32->Add( bSizer2, 0, wxEXPAND, 5 );

	m_staticText1111 = new wxStaticText( this, wxID_ANY, wxT("NOTE : Unit is V for sources, A for sinks"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1111->Wrap( -1 );
	bSizer32->Add( m_staticText1111, 0, wxALL, 5 );

	m_padGrid = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_padGrid->CreateGrid( 5, 5 );
	m_padGrid->EnableEditing( true );
	m_padGrid->EnableGridLines( true );
	m_padGrid->EnableDragGridSize( false );
	m_padGrid->SetMargins( 0, 0 );

	// Columns
	m_padGrid->EnableDragColMove( false );
	m_padGrid->EnableDragColSize( true );
	m_padGrid->SetColLabelSize( 30 );
	m_padGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_padGrid->EnableDragRowSize( true );
	m_padGrid->SetRowLabelSize( 80 );
	m_padGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_padGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer32->Add( m_padGrid, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer32, 1, wxEXPAND, 5 );

	m_staticline111 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline111, 0, wxEXPAND | wxALL, 5 );

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


	bSizer1->Add( bSizer3, 0, 0, 5 );

	m_staticline11 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline11, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxVERTICAL );

	m_staticText31 = new wxStaticText( this, wxID_ANY, wxT("Outputs"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	m_staticText31->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer31->Add( m_staticText31, 0, wxALL, 5 );

	m_mapPotential = new wxCheckBox( this, wxID_ANY, wxT("Generate 3D map : potential drop"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mapPotential->SetValue(true);
	bSizer31->Add( m_mapPotential, 0, wxALL, 5 );

	m_mapCurrentDensity = new wxCheckBox( this, wxID_ANY, wxT("Generate 3D map : current density"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mapCurrentDensity->SetValue(true);
	bSizer31->Add( m_mapCurrentDensity, 0, wxALL, 5 );

	m_mapPowerDensity = new wxCheckBox( this, wxID_ANY, wxT("Generate 3D map : power density density"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer31->Add( m_mapPowerDensity, 0, wxALL, 5 );

	m_textReport = new wxCheckBox( this, wxID_ANY, wxT("Generate text report"), wxDefaultPosition, wxDefaultSize, 0 );
	m_textReport->SetValue(true);
	m_textReport->Enable( false );

	bSizer31->Add( m_textReport, 0, wxALL, 5 );

	m_staticText111 = new wxStaticText( this, wxID_ANY, wxT("NOTE: you can use gmsh to open 3D maps"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText111->Wrap( -1 );
	bSizer31->Add( m_staticText111, 0, wxALL, 5 );

	wxBoxSizer* bSizer41;
	bSizer41 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText12 = new wxStaticText( this, wxID_ANY, wxT("Select a folder for outputs:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizer41->Add( m_staticText12, 0, wxALL, 5 );

	m_dirPicker11 = new wxDirPickerCtrl( this, wxID_ANY, wxEmptyString, wxT("Select a folder"), wxDefaultPosition, wxDefaultSize, wxDIRP_DEFAULT_STYLE );
	m_dirPicker11->Enable( false );

	bSizer41->Add( m_dirPicker11, 0, wxALL, 5 );


	bSizer31->Add( bSizer41, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer31, 0, wxEXPAND, 5 );

	m_resultGrid = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_resultGrid->CreateGrid( 5, 4 );
	m_resultGrid->EnableEditing( true );
	m_resultGrid->EnableGridLines( true );
	m_resultGrid->EnableDragGridSize( false );
	m_resultGrid->SetMargins( 0, 0 );

	// Columns
	m_resultGrid->EnableDragColMove( false );
	m_resultGrid->EnableDragColSize( true );
	m_resultGrid->SetColLabelSize( 30 );
	m_resultGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_resultGrid->EnableDragRowSize( true );
	m_resultGrid->SetRowLabelSize( 80 );
	m_resultGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_resultGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer1->Add( m_resultGrid, 0, wxALL, 5 );

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
