///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_pcbnew_simul_DC_plane_capacitance_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_BASE::PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
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

	m_net1ComboBox = new wxComboBox( this, wxID_ANY, wxT("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	bSizer2->Add( m_net1ComboBox, 0, wxALL, 5 );


	bSizer32->Add( bSizer2, 0, wxEXPAND, 5 );

	wxBoxSizer* bSizer21;
	bSizer21 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText3121 = new wxStaticText( this, wxID_ANY, wxT("Select a net"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText3121->Wrap( -1 );
	m_staticText3121->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer21->Add( m_staticText3121, 0, wxALL, 5 );

	m_net2ComboBox = new wxComboBox( this, wxID_ANY, wxT("Combo!"), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	bSizer21->Add( m_net2ComboBox, 0, wxALL, 5 );


	bSizer32->Add( bSizer21, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer32, 0, wxEXPAND, 5 );

	m_staticline11 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizer1->Add( m_staticline11, 0, wxEXPAND | wxALL, 5 );

	m_3DviewerSizer = new wxBoxSizer( wxVERTICAL );


	bSizer1->Add( m_3DviewerSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxVERTICAL );

	m_staticText31 = new wxStaticText( this, wxID_ANY, wxT("Outputs"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText31->Wrap( -1 );
	m_staticText31->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer31->Add( m_staticText31, 0, wxALL, 5 );

	m_mapCharge = new wxCheckBox( this, wxID_ANY, wxT("Generate 3D map : charge density"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mapCharge->SetValue(true);
	bSizer31->Add( m_mapCharge, 0, wxALL, 5 );

	m_mapElectricField = new wxCheckBox( this, wxID_ANY, wxT("Generate 3D map : Electric field"), wxDefaultPosition, wxDefaultSize, 0 );
	m_mapElectricField->SetValue(true);
	bSizer31->Add( m_mapElectricField, 0, wxALL, 5 );

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

	wxBoxSizer* bSizer311;
	bSizer311 = new wxBoxSizer( wxVERTICAL );

	m_staticText311 = new wxStaticText( this, wxID_ANY, wxT("Results"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText311->Wrap( -1 );
	m_staticText311->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizer311->Add( m_staticText311, 0, wxALL, 5 );

	m_resultText = new wxStaticText( this, wxID_ANY, wxT("Run simulation for capacitance"), wxDefaultPosition, wxDefaultSize, 0 );
	m_resultText->Wrap( -1 );
	bSizer311->Add( m_resultText, 0, wxALL, 5 );

	m_button1 = new wxButton( this, wxID_ANY, wxT("Run"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer311->Add( m_button1, 0, wxALL, 5 );


	bSizer31->Add( bSizer311, 1, wxEXPAND, 5 );


	bSizer1->Add( bSizer31, 0, wxEXPAND, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();

	// Connect Events
	m_net1ComboBox->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_BASE::onNetSelect ), NULL, this );
	m_net2ComboBox->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_BASE::onNetSelect ), NULL, this );
	m_button1->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_BASE::OnRun ), NULL, this );
}

PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_BASE::~PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_BASE()
{
	// Disconnect Events
	m_net1ComboBox->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_BASE::onNetSelect ), NULL, this );
	m_net2ComboBox->Disconnect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_BASE::onNetSelect ), NULL, this );
	m_button1->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( PANEL_PCBNEW_SIMUL_DC_PLANE_CAPACITANCE_BASE::OnRun ), NULL, this );

}
