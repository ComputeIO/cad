///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0-4761b0c5)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "widgets/unit_selector.h"

#include "panel_cable_size_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_CABLE_SIZE_BASE::PANEL_CABLE_SIZE_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : CALCULATOR_PANEL( parent, id, pos, size, style, name )
{
	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer9;
	bSizer9 = new wxBoxSizer( wxHORIZONTAL );

	wxString m_radioUnitsChoices[] = { _("Metric"), _("Imperial") };
	int m_radioUnitsNChoices = sizeof( m_radioUnitsChoices ) / sizeof( wxString );
	m_radioUnits = new wxRadioBox( this, wxID_ANY, _("Units"), wxDefaultPosition, wxDefaultSize, m_radioUnitsNChoices, m_radioUnitsChoices, 1, wxRA_SPECIFY_COLS );
	m_radioUnits->SetSelection( 0 );
	bSizer9->Add( m_radioUnits, 0, wxALL|wxEXPAND, 5 );

	wxGridSizer* gSizer1;
	gSizer1 = new wxGridSizer( 0, 3, 0, 0 );

	m_staticText16 = new wxStaticText( this, wxID_ANY, _("Current"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText16->Wrap( -1 );
	gSizer1->Add( m_staticText16, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_currentCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_currentCtrl, 0, wxALL, 5 );

	m_staticText21 = new wxStaticText( this, wxID_ANY, _("A"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	gSizer1->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxALL, 5 );

	m_staticText161 = new wxStaticText( this, wxID_ANY, _("Length"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText161->Wrap( -1 );
	gSizer1->Add( m_staticText161, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_RIGHT|wxALL, 5 );

	m_lengthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	gSizer1->Add( m_lengthCtrl, 0, wxALL, 5 );

	wxArrayString m_choiceUnitChoices;
	m_choiceUnit = new UNIT_SELECTOR_LEN( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_choiceUnitChoices, 0 );
	m_choiceUnit->SetSelection( 0 );
	gSizer1->Add( m_choiceUnit, 0, wxALL, 5 );


	bSizer9->Add( gSizer1, 0, wxALL, 5 );


	bSizer6->Add( bSizer9, 0, wxEXPAND, 5 );

	m_scrolledWindow1 = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL|wxVSCROLL );
	m_scrolledWindow1->SetScrollRate( 5, 5 );
	wxBoxSizer* bSizer8;
	bSizer8 = new wxBoxSizer( wxVERTICAL );

	m_table = new wxGrid( m_scrolledWindow1, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );

	// Grid
	m_table->CreateGrid( 5, 5 );
	m_table->EnableEditing( true );
	m_table->EnableGridLines( true );
	m_table->EnableDragGridSize( false );
	m_table->SetMargins( 0, 0 );

	// Columns
	m_table->EnableDragColMove( false );
	m_table->EnableDragColSize( true );
	m_table->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Rows
	m_table->EnableDragRowSize( true );
	m_table->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );

	// Label Appearance

	// Cell Defaults
	m_table->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
	bSizer8->Add( m_table, 0, wxALL, 5 );


	m_scrolledWindow1->SetSizer( bSizer8 );
	m_scrolledWindow1->Layout();
	bSizer8->Fit( m_scrolledWindow1 );
	bSizer6->Add( m_scrolledWindow1, 1, wxEXPAND | wxALL, 5 );


	this->SetSizer( bSizer6 );
	this->Layout();

	// Connect Events
	m_radioUnits->Connect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUnitChange ), NULL, this );
	m_currentCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnCurrentChange ), NULL, this );
	m_lengthCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnLengthChange ), NULL, this );
	m_choiceUnit->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnChangeUnitLen ), NULL, this );
}

PANEL_CABLE_SIZE_BASE::~PANEL_CABLE_SIZE_BASE()
{
	// Disconnect Events
	m_radioUnits->Disconnect( wxEVT_COMMAND_RADIOBOX_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnUnitChange ), NULL, this );
	m_currentCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnCurrentChange ), NULL, this );
	m_lengthCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnLengthChange ), NULL, this );
	m_choiceUnit->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( PANEL_CABLE_SIZE_BASE::OnChangeUnitLen ), NULL, this );

}
