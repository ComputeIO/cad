///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "sim_plot_axis_horizontal_base.h"

///////////////////////////////////////////////////////////////////////////

SIM_PLOT_AXIS_HORIZONTAL_BASE::SIM_PLOT_AXIS_HORIZONTAL_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxDialog( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText1 = new wxStaticText( this, wxID_ANY, wxT("Quantity plotted:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText1->Wrap( -1 );
	bSizer2->Add( m_staticText1, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_quantityCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_quantityCtrl->Enable( false );

	bSizer2->Add( m_quantityCtrl, 1, wxALL|wxEXPAND, 5 );


	bSizer1->Add( bSizer2, 0, wxEXPAND, 5 );

	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Axis limits") ), wxVERTICAL );

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer( wxHORIZONTAL );

	m_staticText2 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, wxT("Left:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText2->Wrap( -1 );
	bSizer3->Add( m_staticText2, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_leftCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_leftCtrl, 2, wxALL, 5 );


	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText21 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, wxT("Ticks:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText21->Wrap( -1 );
	bSizer3->Add( m_staticText21, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_ticksCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_ticksCtrl, 2, wxALL, 5 );


	bSizer3->Add( 0, 0, 1, wxEXPAND, 5 );

	m_staticText211 = new wxStaticText( sbSizer1->GetStaticBox(), wxID_ANY, wxT("Right:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText211->Wrap( -1 );
	bSizer3->Add( m_staticText211, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_rightCtrl = new wxTextCtrl( sbSizer1->GetStaticBox(), wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer3->Add( m_rightCtrl, 2, wxALL, 5 );


	sbSizer1->Add( bSizer3, 0, wxEXPAND, 5 );


	bSizer1->Add( sbSizer1, 0, wxALL|wxEXPAND, 5 );

	wxBoxSizer* bSizer31;
	bSizer31 = new wxBoxSizer( wxHORIZONTAL );

	m_logCheckbox = new wxCheckBox( this, wxID_ANY, wxT("Logarithmic"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer31->Add( m_logCheckbox, 0, wxALIGN_BOTTOM|wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_sdbSizer1 = new wxStdDialogButtonSizer();
	m_sdbSizer1OK = new wxButton( this, wxID_OK );
	m_sdbSizer1->AddButton( m_sdbSizer1OK );
	m_sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer1->AddButton( m_sdbSizer1Cancel );
	m_sdbSizer1->Realize();

	bSizer31->Add( m_sdbSizer1, 1, wxALIGN_BOTTOM|wxALL|wxEXPAND, 5 );


	bSizer1->Add( bSizer31, 1, wxEXPAND, 5 );


	this->SetSizer( bSizer1 );
	this->Layout();
	bSizer1->Fit( this );

	this->Centre( wxBOTH );

	// Connect Events
	m_logCheckbox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( SIM_PLOT_AXIS_HORIZONTAL_BASE::OnLogCheckBox ), NULL, this );
	m_sdbSizer1OK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SIM_PLOT_AXIS_HORIZONTAL_BASE::OnOKButtonClick ), NULL, this );
}

SIM_PLOT_AXIS_HORIZONTAL_BASE::~SIM_PLOT_AXIS_HORIZONTAL_BASE()
{
	// Disconnect Events
	m_logCheckbox->Disconnect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( SIM_PLOT_AXIS_HORIZONTAL_BASE::OnLogCheckBox ), NULL, this );
	m_sdbSizer1OK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( SIM_PLOT_AXIS_HORIZONTAL_BASE::OnOKButtonClick ), NULL, this );

}
