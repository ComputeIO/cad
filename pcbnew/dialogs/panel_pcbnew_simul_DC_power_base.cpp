///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version May 14 2021)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_pcbnew_simul_DC_power_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PCBNEW_SIMUL_DC_POWER_BASE::PANEL_PCBNEW_SIMUL_DC_POWER_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer32;
	bSizer32 = new wxBoxSizer( wxVERTICAL );

	m_staticText3111 = new wxStaticText( this, wxID_ANY, wxT("Electrical"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3111->Wrap( -1 );
	m_staticText3111->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer32->Add( m_staticText3111, 0, wxALL, 5 );

	m_grid1 = new wxGrid( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_grid1->CreateGrid( 9, 3 );
	m_grid1->EnableEditing( true );
	m_grid1->EnableGridLines( true );
	m_grid1->EnableDragGridSize( false );
	m_grid1->SetMargins( 0, 0 );

	// Columns
	m_grid1->EnableDragColMove( false );
	m_grid1->EnableDragColSize( true );
	m_grid1->SetColLabelSize( 30 );
	m_grid1->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_grid1->EnableDragRowSize( true );
	m_grid1->SetRowLabelSize( 80 );
	m_grid1->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_grid1->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer32->Add( m_grid1, 0, wxALL, 5 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText312 = new wxStaticText( this, wxID_ANY, wxT("Select a net"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText312->Wrap( -1 );
	m_staticText312->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer2->Add( m_staticText312, 0, wxALL, 5 );

	m_comboBox1 = new wxComboBox( this, wxID_ANY, wxT("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	bSizer2->Add( m_comboBox1, 0, wxALL, 5 );


	bSizer32->Add( bSizer2, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer32, 1, wxEXPAND, 5 );

	m_staticline111 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline111, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxVERTICAL );

	m_staticText311 = new wxStaticText( this, wxID_ANY, wxT("Simulation"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText311->Wrap( -1 );
	m_staticText311->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer3->Add( m_staticText311, 0, wxALL, 5 );

	wxString m_radioBox1Choices[] = { wxT("2.5D: Each layer is a 2D surface, vias are simulated using their resistance. Simulation is faster"), wxT("3D : Layers have thickness, vias are simulated based on their geometry. Simulation is slower") };
	int m_radioBox1NChoices = sizeof( m_radioBox1Choices ) / sizeof( wxString );
	m_radioBox1 = new wxRadioBox( this, wxID_ANY, wxT("wxRadioBox"), wxDefaultPosition, wxDefaultSize, m_radioBox1NChoices, m_radioBox1Choices, 1, wxRA_SPECIFY_COLS );
	m_radioBox1->SetSelection( 0 );
	bSizer3->Add( m_radioBox1, 0, wxALL, 5 );

	m_checkBox2211 = new wxCheckBox( this, wxID_ANY, wxT("Report via stress analysis"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_checkBox2211, 0, wxALL, 5 );


	bSizer1->Add( bSizer3, 1, wxEXPAND, 5 );

	m_staticline11 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline11, 0, wxEXPAND | wxALL, 5 );

	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxVERTICAL );

	m_staticText31 = new wxStaticText( this, wxID_ANY, wxT("Outputs"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	m_staticText31->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxEmptyString ) );

	bSizer31->Add( m_staticText31, 0, wxALL, 5 );

	m_checkBox212 = new wxCheckBox( this, wxID_ANY, wxT("Generate 3D map : potential drop"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBox212->SetValue(true);
	bSizer31->Add( m_checkBox212, 0, wxALL, 5 );

	m_checkBox22 = new wxCheckBox( this, wxID_ANY, wxT("Generate 3D map : current density"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBox22->SetValue(true);
	bSizer31->Add( m_checkBox22, 0, wxALL, 5 );

	m_checkBox221 = new wxCheckBox( this, wxID_ANY, wxT("Generate 3D map : power density density"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer31->Add( m_checkBox221, 0, wxALL, 5 );

	m_checkBox2111 = new wxCheckBox( this, wxID_ANY, wxT("Generate text report"), wxDefaultPosition, wxDefaultSize, 0 );
	m_checkBox2111->SetValue(true);
	bSizer31->Add( m_checkBox2111, 0, wxALL, 5 );

	m_staticText111 = new wxStaticText( this, wxID_ANY, wxT("NOTE: you can use gmsh to open 3D maps"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText111->Wrap( -1 );
	bSizer31->Add( m_staticText111, 0, wxALL, 5 );

	wxBoxSizer* bSizer41;
	bSizer41 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText12 = new wxStaticText( this, wxID_ANY, wxT("Select a folder for outputs:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText12->Wrap( -1 );
	bSizer41->Add( m_staticText12, 0, wxALL, 5 );

	m_dirPicker11 = new wxDirPickerCtrl( this, wxID_ANY, wxEmptyString, wxT("Select a folder"), wxDefaultPosition, wxDefaultSize, wxDIRP_DEFAULT_STYLE );
	bSizer41->Add( m_dirPicker11, 0, wxALL, 5 );


	bSizer31->Add( bSizer41, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer31, 1, wxEXPAND, 5 );

	m_dataViewCtrl1 = new wxDataViewCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
	m_dataViewCtrl1->SetMinSize( wxSize( -1,200 ) );
	m_dataViewCtrl1->SetMaxSize( wxSize( -1,200 ) );

	bSizer1->Add( m_dataViewCtrl1, 0, wxEXPAND, 5 );

	m_button1 = new wxButton( this, wxID_ANY, wxT("Run"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer1->Add( m_button1, 0, wxALL, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
}

PANEL_PCBNEW_SIMUL_DC_POWER_BASE::~PANEL_PCBNEW_SIMUL_DC_POWER_BASE()
{
}
