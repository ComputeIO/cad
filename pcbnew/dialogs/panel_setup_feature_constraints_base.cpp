///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "panel_setup_feature_constraints_base.h"

///////////////////////////////////////////////////////////////////////////

PANEL_SETUP_FEATURE_CONSTRAINTS_BASE::PANEL_SETUP_FEATURE_CONSTRAINTS_BASE( wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name ) : wxPanel( parent, id, pos, size, style, name )
{
	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxHORIZONTAL );

	wxBoxSizer* sbFeatureRules;
	sbFeatureRules = new wxBoxSizer( wxVERTICAL );

	m_staticText26 = new wxStaticText( this, wxID_ANY, _("Allowed features"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText26->Wrap( -1 );
	sbFeatureRules->Add( m_staticText26, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizerViaOpt;
	fgSizerViaOpt = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizerViaOpt->SetFlexibleDirection( wxBOTH );
	fgSizerViaOpt->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_bitmapBlindBuried = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerViaOpt->Add( m_bitmapBlindBuried, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_OptAllowBlindBuriedVias = new wxCheckBox( this, wxID_ANY, _("Allow blind/buried vias"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerViaOpt->Add( m_OptAllowBlindBuriedVias, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_bitmap_uVia = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerViaOpt->Add( m_bitmap_uVia, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_OptAllowMicroVias = new wxCheckBox( this, wxID_ANY, _("Allow micro vias (uVias)"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerViaOpt->Add( m_OptAllowMicroVias, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );


	sbFeatureRules->Add( fgSizerViaOpt, 0, wxEXPAND|wxTOP, 5 );


	sbFeatureRules->Add( 0, 5, 0, wxEXPAND|wxTOP|wxBOTTOM, 5 );

	wxBoxSizer* bSizerArcToPoly;
	bSizerArcToPoly = new wxBoxSizer( wxVERTICAL );

	m_staticline2 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bSizerArcToPoly->Add( m_staticline2, 0, wxEXPAND | wxALL, 2 );

	m_stCircleToPolyOpt = new wxStaticText( this, wxID_ANY, _("Arc/circle drawing"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stCircleToPolyOpt->Wrap( -1 );
	m_stCircleToPolyOpt->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bSizerArcToPoly->Add( m_stCircleToPolyOpt, 0, wxTOP|wxRIGHT|wxLEFT, 5 );

	wxFlexGridSizer* fgSizer2;
	fgSizer2 = new wxFlexGridSizer( 0, 4, 3, 0 );
	fgSizer2->AddGrowableCol( 2 );
	fgSizer2->SetFlexibleDirection( wxBOTH );
	fgSizer2->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );


	fgSizer2->Add( 15, 0, 1, wxEXPAND, 5 );

	m_maxErrorTitle = new wxStaticText( this, wxID_ANY, _("Maximum deviation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxErrorTitle->Wrap( -1 );
	m_maxErrorTitle->SetToolTip( _("This is the maximum distance between a circle and the polygonal shape that approximate it.\nThe error max defines the number of segments of this polygon.") );

	fgSizer2->Add( m_maxErrorTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT|wxLEFT, 5 );

	m_maxErrorCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizer2->Add( m_maxErrorCtrl, 0, wxALIGN_CENTER_VERTICAL|wxALL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_maxErrorUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_maxErrorUnits->Wrap( -1 );
	fgSizer2->Add( m_maxErrorUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	bSizerArcToPoly->Add( fgSizer2, 0, wxEXPAND|wxBOTTOM, 5 );


	sbFeatureRules->Add( bSizerArcToPoly, 0, wxEXPAND|wxTOP, 5 );

	m_bSizerPolygonFillOption = new wxBoxSizer( wxVERTICAL );

	m_staticline1 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	m_bSizerPolygonFillOption->Add( m_staticline1, 0, wxEXPAND | wxALL, 2 );

	m_stZoneFilledPolysOpt = new wxStaticText( this, wxID_ANY, _("Zone fill strategy"), wxDefaultPosition, wxDefaultSize, 0 );
	m_stZoneFilledPolysOpt->Wrap( -1 );
	m_stZoneFilledPolysOpt->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	m_bSizerPolygonFillOption->Add( m_stZoneFilledPolysOpt, 0, wxALL, 5 );

	wxBoxSizer* bSizer5;
	bSizer5 = new wxBoxSizer( wxHORIZONTAL );

	m_bitmapZoneFillOpt = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	bSizer5->Add( m_bitmapZoneFillOpt, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	wxBoxSizer* bSizerOutlinesOpts;
	bSizerOutlinesOpts = new wxBoxSizer( wxVERTICAL );

	m_rbOutlinePolygonBestQ = new wxRadioButton( this, wxID_ANY, _("Stroked outlines (legacy)"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizerOutlinesOpts->Add( m_rbOutlinePolygonBestQ, 0, wxALL, 4 );

	m_rbOutlinePolygonFastest = new wxRadioButton( this, wxID_ANY, _("Smoothed polygons (best performance)"), wxDefaultPosition, wxDefaultSize, 0 );
	m_rbOutlinePolygonFastest->SetValue( true );
	bSizerOutlinesOpts->Add( m_rbOutlinePolygonFastest, 0, wxALL, 4 );


	bSizer5->Add( bSizerOutlinesOpts, 1, wxEXPAND, 5 );


	m_bSizerPolygonFillOption->Add( bSizer5, 1, wxEXPAND, 15 );


	sbFeatureRules->Add( m_bSizerPolygonFillOption, 0, wxEXPAND|wxTOP, 10 );


	bMainSizer->Add( sbFeatureRules, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	bMainSizer->Add( 0, 0, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );

	wxBoxSizer* sbFeatureConstraints;
	sbFeatureConstraints = new wxBoxSizer( wxVERTICAL );

	wxFlexGridSizer* fgFeatureConstraints;
	fgFeatureConstraints = new wxFlexGridSizer( 0, 4, 0, 0 );
	fgFeatureConstraints->SetFlexibleDirection( wxBOTH );
	fgFeatureConstraints->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_staticText23 = new wxStaticText( this, wxID_ANY, _("Copper"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText23->Wrap( -1 );
	fgFeatureConstraints->Add( m_staticText23, 0, wxTOP|wxBOTTOM|wxLEFT, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND, 5 );

	m_bitmapClearance = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapClearance, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_clearanceTitle = new wxStaticText( this, wxID_ANY, _("Minimum clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_clearanceTitle, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_clearanceCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_clearanceCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_clearanceUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_clearanceUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_clearanceUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_bitmapMinTrackWidth = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinTrackWidth, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_TrackMinWidthTitle = new wxStaticText( this, wxID_ANY, _("Minimum track width:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TrackMinWidthTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_TrackMinWidthTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_TrackMinWidthCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	m_TrackMinWidthCtrl->SetMinSize( wxSize( 120,-1 ) );

	fgFeatureConstraints->Add( m_TrackMinWidthCtrl, 0, wxALIGN_LEFT|wxALIGN_TOP|wxEXPAND|wxALL, 5 );

	m_TrackMinWidthUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_TrackMinWidthUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_TrackMinWidthUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_bitmapMinViaAnnulus = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinViaAnnulus, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_ViaMinAnnulusTitle = new wxStaticText( this, wxID_ANY, _("Minimum via annulus:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaMinAnnulusTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinAnnulusTitle, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_ViaMinAnnulusCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_ViaMinAnnulusCtrl, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_ViaMinAnnulusUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ViaMinAnnulusUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinAnnulusUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_bitmapMinViaDiameter = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinViaDiameter, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_ViaMinTitle = new wxStaticText( this, wxID_ANY, _("Minimum via diameter:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_ViaMinTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_SetViasMinSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_SetViasMinSizeCtrl, 0, wxEXPAND|wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_ViaMinUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_ViaMinUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_ViaMinUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_bitmapEdgeClearance = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapEdgeClearance, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_EdgeClearanceLabel = new wxStaticText( this, wxID_ANY, _("Copper edge clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EdgeClearanceLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_EdgeClearanceLabel, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_EdgeClearanceCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_EdgeClearanceCtrl, 0, wxALL|wxEXPAND, 5 );

	m_EdgeClearanceUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_EdgeClearanceUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_EdgeClearanceUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticline3 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline3, 0, wxTOP|wxEXPAND, 20 );

	m_staticline4 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline4, 0, wxEXPAND|wxTOP, 20 );

	m_staticline5 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline5, 0, wxEXPAND|wxTOP, 20 );

	m_staticline6 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline6, 0, wxEXPAND|wxTOP, 20 );

	m_staticText24 = new wxStaticText( this, wxID_ANY, _("Holes"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText24->Wrap( -1 );
	fgFeatureConstraints->Add( m_staticText24, 0, wxALL, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );

	m_bitmapMinViaDrill = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinViaDrill, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_MinDrillTitle = new wxStaticText( this, wxID_ANY, _("Minimum through hole:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_MinDrillTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_MinDrillTitle, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_MinDrillCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_MinDrillCtrl, 0, wxEXPAND|wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_MinDrillUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_MinDrillUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_MinDrillUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_bitmapMinHoleClearance = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinHoleClearance, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_HoleToHoleTitle = new wxStaticText( this, wxID_ANY, _("Hole to hole clearance:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleToHoleTitle->Wrap( -1 );
	fgFeatureConstraints->Add( m_HoleToHoleTitle, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_SetHoleToHoleCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_SetHoleToHoleCtrl, 0, wxEXPAND|wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_HoleToHoleUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, 0 );
	m_HoleToHoleUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_HoleToHoleUnits, 0, wxALIGN_CENTER_VERTICAL, 5 );

	m_staticline8 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline8, 0, wxEXPAND|wxTOP, 20 );

	m_staticline9 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline9, 0, wxEXPAND|wxTOP, 20 );

	m_staticline10 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline10, 0, wxEXPAND|wxTOP, 20 );

	m_staticline11 = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	fgFeatureConstraints->Add( m_staticline11, 0, wxEXPAND|wxTOP, 20 );

	m_staticText25 = new wxStaticText( this, wxID_ANY, _("uVias"), wxDefaultPosition, wxDefaultSize, 0 );
	m_staticText25->Wrap( -1 );
	fgFeatureConstraints->Add( m_staticText25, 0, wxALL, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );


	fgFeatureConstraints->Add( 0, 0, 1, wxEXPAND|wxTOP, 5 );

	m_bitmapMinuViaDiameter = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinuViaDiameter, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_uviaMinSizeLabel = new wxStaticText( this, wxID_ANY, _("Minimum uVia diameter:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinSizeLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinSizeLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_uviaMinSizeCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_uviaMinSizeCtrl, 0, wxEXPAND|wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_uviaMinSizeUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinSizeUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinSizeUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_bitmapMinuViaDrill = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_bitmapMinuViaDrill, 0, wxALL|wxALIGN_CENTER_HORIZONTAL, 5 );

	m_uviaMinDrillLabel = new wxStaticText( this, wxID_ANY, _("Minimum uVia drill:"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinDrillLabel->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinDrillLabel, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );

	m_uviaMinDrillCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgFeatureConstraints->Add( m_uviaMinDrillCtrl, 0, wxEXPAND|wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_uviaMinDrillUnits = new wxStaticText( this, wxID_ANY, _("mm"), wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT );
	m_uviaMinDrillUnits->Wrap( -1 );
	fgFeatureConstraints->Add( m_uviaMinDrillUnits, 0, wxALIGN_CENTER_VERTICAL|wxALIGN_LEFT, 5 );


	sbFeatureConstraints->Add( fgFeatureConstraints, 1, wxEXPAND|wxLEFT, 5 );


	bMainSizer->Add( sbFeatureConstraints, 0, wxEXPAND|wxRIGHT|wxLEFT, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();
	bMainSizer->Fit( this );

	// Connect Events
	m_rbOutlinePolygonBestQ->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_SETUP_FEATURE_CONSTRAINTS_BASE::onChangeOutlineOpt ), NULL, this );
	m_rbOutlinePolygonFastest->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_SETUP_FEATURE_CONSTRAINTS_BASE::onChangeOutlineOpt ), NULL, this );
}

PANEL_SETUP_FEATURE_CONSTRAINTS_BASE::~PANEL_SETUP_FEATURE_CONSTRAINTS_BASE()
{
	// Disconnect Events
	m_rbOutlinePolygonBestQ->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_SETUP_FEATURE_CONSTRAINTS_BASE::onChangeOutlineOpt ), NULL, this );
	m_rbOutlinePolygonFastest->Disconnect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( PANEL_SETUP_FEATURE_CONSTRAINTS_BASE::onChangeOutlineOpt ), NULL, this );

}
