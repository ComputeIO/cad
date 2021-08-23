///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.9.0 May 11 2021)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "dialog_text_item_properties_base.h"

///////////////////////////////////////////////////////////////////////////

DIALOG_TEXT_ITEM_PROPERTIES_BASE::DIALOG_TEXT_ITEM_PROPERTIES_BASE( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : DIALOG_SHIM( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( -1,-1 ), wxDefaultSize );

	wxBoxSizer* bMainSizer;
	bMainSizer = new wxBoxSizer( wxVERTICAL );

	m_SingleLineSizer = new wxBoxSizer( wxHORIZONTAL );

	m_SingleLineLabel = new wxStaticText( this, wxID_ANY, _("Reference designator:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SingleLineLabel->Wrap( -1 );
	m_SingleLineSizer->Add( m_SingleLineLabel, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

	m_SingleLineText = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	m_SingleLineSizer->Add( m_SingleLineText, 1, wxTOP|wxBOTTOM|wxRIGHT|wxALIGN_CENTER_VERTICAL, 5 );


	bMainSizer->Add( m_SingleLineSizer, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	m_MultiLineSizer = new wxBoxSizer( wxVERTICAL );

	m_MultiLineLabel = new wxStaticText( this, wxID_ANY, _("Text:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_MultiLineLabel->Wrap( -1 );
	m_MultiLineSizer->Add( m_MultiLineLabel, 0, wxRIGHT|wxLEFT, 5 );

	m_MultiLineText = new wxStyledTextCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString );
	m_MultiLineText->SetUseTabs( true );
	m_MultiLineText->SetTabWidth( 4 );
	m_MultiLineText->SetIndent( 4 );
	m_MultiLineText->SetTabIndents( false );
	m_MultiLineText->SetBackSpaceUnIndents( false );
	m_MultiLineText->SetViewEOL( false );
	m_MultiLineText->SetViewWhiteSpace( false );
	m_MultiLineText->SetMarginWidth( 2, 0 );
	m_MultiLineText->SetIndentationGuides( true );
	m_MultiLineText->SetReadOnly( false );
	m_MultiLineText->SetMarginWidth( 1, 0 );
	m_MultiLineText->SetMarginWidth( 0, 0 );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS );
	m_MultiLineText->MarkerSetBackground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("BLACK") ) );
	m_MultiLineText->MarkerSetForeground( wxSTC_MARKNUM_FOLDER, wxColour( wxT("WHITE") ) );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS );
	m_MultiLineText->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("BLACK") ) );
	m_MultiLineText->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPEN, wxColour( wxT("WHITE") ) );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS );
	m_MultiLineText->MarkerSetBackground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("BLACK") ) );
	m_MultiLineText->MarkerSetForeground( wxSTC_MARKNUM_FOLDEREND, wxColour( wxT("WHITE") ) );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS );
	m_MultiLineText->MarkerSetBackground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("BLACK") ) );
	m_MultiLineText->MarkerSetForeground( wxSTC_MARKNUM_FOLDEROPENMID, wxColour( wxT("WHITE") ) );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY );
	m_MultiLineText->MarkerDefine( wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY );
	m_MultiLineText->SetSelBackground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHT ) );
	m_MultiLineText->SetSelForeground( true, wxSystemSettings::GetColour( wxSYS_COLOUR_HIGHLIGHTTEXT ) );
	m_MultiLineText->SetToolTip( _("Enter the text placed on selected layer.") );

	m_MultiLineSizer->Add( m_MultiLineText, 1, wxBOTTOM|wxEXPAND|wxLEFT|wxRIGHT, 5 );


	bMainSizer->Add( m_MultiLineSizer, 20, wxEXPAND|wxALL, 10 );

	wxBoxSizer* m_NetSizer;
	m_NetSizer = new wxBoxSizer( wxHORIZONTAL );

	m_NetLabel = new wxStaticText( this, wxID_ANY, _("Net:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_NetLabel->Wrap( -1 );
	m_NetSizer->Add( m_NetLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_NetValue = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER );
	m_NetValue->SetMinSize( wxSize( 445,-1 ) );

	m_NetSizer->Add( m_NetValue, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	bMainSizer->Add( m_NetSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* m_ShapeSizer;
	m_ShapeSizer = new wxBoxSizer( wxHORIZONTAL );

	m_ShapeLabel = new wxStaticText( this, wxID_ANY, _("Shape:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ShapeLabel->Wrap( -1 );
	m_ShapeSizer->Add( m_ShapeLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	wxString m_ShapeChoices[] = { _("Input"), _("Output"), _("Bidirectional"), _("Tri-state"), _("Passive") };
	int m_ShapeNChoices = sizeof( m_ShapeChoices ) / sizeof( wxString );
	m_Shape = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_ShapeNChoices, m_ShapeChoices, 0 );
	m_Shape->SetSelection( 0 );
	m_ShapeSizer->Add( m_Shape, 0, wxALL, 5 );

	m_ShapeBitmap = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxDefaultSize, 0 );
	m_ShapeSizer->Add( m_ShapeBitmap, 0, wxALIGN_CENTER_VERTICAL, 5 );


	m_ShapeSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_cbLocked = new wxCheckBox( this, wxID_ANY, _("Locked"), wxDefaultPosition, wxDefaultSize, 0 );
	m_cbLocked->SetValue(true);
	m_ShapeSizer->Add( m_cbLocked, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bMainSizer->Add( m_ShapeSizer, 1, wxEXPAND, 5 );

	wxBoxSizer* bSizer14;
	bSizer14 = new wxBoxSizer( wxHORIZONTAL );

	m_note1 = new wxStaticText( this, wxID_ANY, _("Note:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_note1->Wrap( -1 );
	bSizer14->Add( m_note1, 0, wxALIGN_TOP|wxLEFT|wxRIGHT|wxTOP, 5 );

	m_note2 = new wxStaticText( this, wxID_ANY, _("The margins around the text are controlled by the text offset ratio\nin Schematic Setup > General > Formatting."), wxDefaultPosition, wxDefaultSize, 0 );
	m_note2->Wrap( -1 );
	bSizer14->Add( m_note2, 0, wxALL, 5 );


	bMainSizer->Add( bSizer14, 1, wxALIGN_BOTTOM|wxALIGN_LEFT|wxALIGN_RIGHT|wxEXPAND, 5 );

	wxBoxSizer* bNote;
	bNote = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bFontName;
	bFontName = new wxBoxSizer( wxHORIZONTAL );

	m_FontLabel = new wxStaticText( this, wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FontLabel->Wrap( -1 );
	bFontName->Add( m_FontLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_FontCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	bFontName->Add( m_FontCtrl, 7, wxALL, 5 );

	m_FontSelectionButton = new wxButton( this, wxID_ANY, _("Select font..."), wxDefaultPosition, wxDefaultSize, 0 );
	bFontName->Add( m_FontSelectionButton, 0, wxALL, 5 );


	bNote->Add( bFontName, 1, wxEXPAND, 10 );

	m_FontProperties = new wxBoxSizer( wxHORIZONTAL );

	m_FontBold = new wxCheckBox( this, wxID_ANY, _("Bold"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FontProperties->Add( m_FontBold, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_FontItalic = new wxCheckBox( this, wxID_ANY, _("Italic"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FontProperties->Add( m_FontItalic, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );


	m_FontProperties->Add( 0, 0, 1, wxEXPAND, 5 );

	m_formattingHelp = new wxHyperlinkCtrl( this, wxID_ANY, _("Syntax help"), wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE );
	m_formattingHelp->SetToolTip( _("Show syntax help window") );

	m_FontProperties->Add( m_formattingHelp, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxLEFT|wxRIGHT, 5 );


	bNote->Add( m_FontProperties, 1, wxEXPAND, 5 );


	bMainSizer->Add( bNote, 0, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 10 );

	fgSizerSetup = new wxFlexGridSizer( 0, 5, 3, 0 );
	fgSizerSetup->AddGrowableCol( 1 );
	fgSizerSetup->AddGrowableCol( 4 );
	fgSizerSetup->SetFlexibleDirection( wxBOTH );
	fgSizerSetup->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_LayerLabel = new wxStaticText( this, wxID_ANY, _("Layer:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_LayerLabel->Wrap( -1 );
	fgSizerSetup->Add( m_LayerLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_LayerSelectionCtrl = new wxBitmapComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	fgSizerSetup->Add( m_LayerSelectionCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );


	fgSizerSetup->Add( 0, 0, 0, wxRIGHT|wxLEFT, 40 );

	m_Visible = new wxCheckBox( this, wxID_ANY, _("Visible"), wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSetup->Add( m_Visible, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );


	fgSizerSetup->Add( 0, 0, 1, wxEXPAND, 5 );

	m_SizeXLabel = new wxStaticText( this, wxID_ANY, _("Width:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXLabel->Wrap( -1 );
	m_SizeXLabel->SetToolTip( _("Text width") );

	fgSizerSetup->Add( m_SizeXLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_SizeXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	fgSizerSetup->Add( m_SizeXCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_SizeXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeXUnits->Wrap( -1 );
	fgSizerSetup->Add( m_SizeXUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_FontLineSpacingLabel = new wxStaticText( this, wxID_ANY, _("Line spacing:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_FontLineSpacingLabel->Wrap( -1 );
	fgSizerSetup->Add( m_FontLineSpacingLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_FontLineSpacing = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0 );
	fgSizerSetup->Add( m_FontLineSpacing, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 5 );

	m_SizeYLabel = new wxStaticText( this, wxID_ANY, _("Height:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYLabel->Wrap( -1 );
	m_SizeYLabel->SetToolTip( _("Text height") );

	fgSizerSetup->Add( m_SizeYLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_SizeYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	fgSizerSetup->Add( m_SizeYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_SizeYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_SizeYUnits->Wrap( -1 );
	fgSizerSetup->Add( m_SizeYUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_JustifyLabel = new wxStaticText( this, wxID_ANY, _("Justification:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_JustifyLabel->Wrap( -1 );
	fgSizerSetup->Add( m_JustifyLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	wxString m_JustifyChoices[] = { _("Left"), _("Center"), _("Right") };
	int m_JustifyNChoices = sizeof( m_JustifyChoices ) / sizeof( wxString );
	m_Justify = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, m_JustifyNChoices, m_JustifyChoices, 0 );
	m_Justify->SetSelection( 0 );
	fgSizerSetup->Add( m_Justify, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxRIGHT, 3 );

	m_ThicknessLabel = new wxStaticText( this, wxID_ANY, _("Thickness:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThicknessLabel->Wrap( -1 );
	m_ThicknessLabel->SetToolTip( _("Text thickness") );

	fgSizerSetup->Add( m_ThicknessLabel, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT, 5 );

	m_ThicknessCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	fgSizerSetup->Add( m_ThicknessCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxBOTTOM, 2 );

	m_ThicknessUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_ThicknessUnits->Wrap( -1 );
	fgSizerSetup->Add( m_ThicknessUnits, 0, wxALIGN_CENTER_VERTICAL|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	m_OrientLabel = new wxStaticText( this, wxID_ANY, _("Rotation:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_OrientLabel->Wrap( -1 );
	m_OrientLabel->SetToolTip( _("Text orientation") );

	fgSizerSetup->Add( m_OrientLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_OrientCtrl = new wxComboBox( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, 0 );
	m_OrientCtrl->Append( _("0.0") );
	m_OrientCtrl->Append( _("90.0") );
	m_OrientCtrl->Append( _("-90.0") );
	m_OrientCtrl->Append( _("180.0") );
	fgSizerSetup->Add( m_OrientCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_PositionXLabel = new wxStaticText( this, wxID_ANY, _("Position X:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PositionXLabel->Wrap( -1 );
	m_PositionXLabel->SetToolTip( _("Text pos X") );

	fgSizerSetup->Add( m_PositionXLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_PositionXCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	fgSizerSetup->Add( m_PositionXCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_PositionXUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PositionXUnits->Wrap( -1 );
	fgSizerSetup->Add( m_PositionXUnits, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT|wxLEFT, 5 );

	m_Mirrored = new wxCheckBox( this, wxID_ANY, _("Mirrored"), wxDefaultPosition, wxDefaultSize, 0 );
	m_Mirrored->SetToolTip( _("Mirror text") );

	fgSizerSetup->Add( m_Mirrored, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	fgSizerSetup->Add( 0, 0, 0, 0, 5 );

	m_PositionYLabel = new wxStaticText( this, wxID_ANY, _("Position Y:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PositionYLabel->Wrap( -1 );
	m_PositionYLabel->SetToolTip( _("Text pos Y") );

	fgSizerSetup->Add( m_PositionYLabel, 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_PositionYCtrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	fgSizerSetup->Add( m_PositionYCtrl, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND, 5 );

	m_PositionYUnits = new wxStaticText( this, wxID_ANY, _("unit"), wxDefaultPosition, wxDefaultSize, 0 );
	m_PositionYUnits->Wrap( -1 );
	fgSizerSetup->Add( m_PositionYUnits, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 5 );

	m_KeepUpright = new wxCheckBox( this, wxID_ANY, _("Keep upright"), wxDefaultPosition, wxDefaultSize, 0 );
	m_KeepUpright->SetToolTip( _("Keep text upright") );

	fgSizerSetup->Add( m_KeepUpright, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );


	fgSizerSetup->Add( 0, 0, 1, wxEXPAND, 5 );


	bMainSizer->Add( fgSizerSetup, 0, wxEXPAND|wxRIGHT|wxLEFT, 15 );


	bMainSizer->Add( 0, 0, 0, wxTOP, 5 );

	wxBoxSizer* bMargins;
	bMargins = new wxBoxSizer( wxVERTICAL );

	m_statusLine = new wxStaticText( this, wxID_ANY, _("Parent footprint description"), wxDefaultPosition, wxDefaultSize, 0 );
	m_statusLine->Wrap( -1 );
	m_statusLine->SetFont( wxFont( 11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString ) );

	bMargins->Add( m_statusLine, 0, wxBOTTOM|wxRIGHT|wxLEFT, 3 );


	bMainSizer->Add( bMargins, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 12 );

	m_staticline = new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
	bMainSizer->Add( m_staticline, 0, wxEXPAND|wxTOP|wxRIGHT|wxLEFT, 10 );

	wxBoxSizer* lowerSizer;
	lowerSizer = new wxBoxSizer( wxHORIZONTAL );


	lowerSizer->Add( 0, 0, 1, wxEXPAND, 5 );

	m_sdbSizer = new wxStdDialogButtonSizer();
	m_sdbSizerOK = new wxButton( this, wxID_OK );
	m_sdbSizer->AddButton( m_sdbSizerOK );
	m_sdbSizerCancel = new wxButton( this, wxID_CANCEL );
	m_sdbSizer->AddButton( m_sdbSizerCancel );
	m_sdbSizer->Realize();

	lowerSizer->Add( m_sdbSizer, 0, wxALL, 5 );


	bMainSizer->Add( lowerSizer, 0, wxEXPAND, 5 );


	this->SetSizer( bMainSizer );
	this->Layout();

	this->Centre( wxBOTH );

	// Connect Events
	this->Connect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnInitDlg ) );
	m_SingleLineText->Connect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnSetFocusText ), NULL, this );
	m_SingleLineText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_Shape->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnShapeChoice ), NULL, this );
	m_FontCtrl->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnFontFieldChange ), NULL, this );
	m_FontSelectionButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnShowFontDialog ), NULL, this );
	m_formattingHelp->Connect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnFormattingHelp ), NULL, this );
	m_SizeXCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_SizeYCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_ThicknessCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_OrientCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_PositionXCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_PositionYCtrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_sdbSizerOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
}

DIALOG_TEXT_ITEM_PROPERTIES_BASE::~DIALOG_TEXT_ITEM_PROPERTIES_BASE()
{
	// Disconnect Events
	this->Disconnect( wxEVT_INIT_DIALOG, wxInitDialogEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnInitDlg ) );
	m_SingleLineText->Disconnect( wxEVT_SET_FOCUS, wxFocusEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnSetFocusText ), NULL, this );
	m_SingleLineText->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_Shape->Disconnect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnShapeChoice ), NULL, this );
	m_FontCtrl->Disconnect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnFontFieldChange ), NULL, this );
	m_FontSelectionButton->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnShowFontDialog ), NULL, this );
	m_formattingHelp->Disconnect( wxEVT_COMMAND_HYPERLINK, wxHyperlinkEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnFormattingHelp ), NULL, this );
	m_SizeXCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_SizeYCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_ThicknessCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_OrientCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_PositionXCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_PositionYCtrl->Disconnect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );
	m_sdbSizerOK->Disconnect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( DIALOG_TEXT_ITEM_PROPERTIES_BASE::OnOkClick ), NULL, this );

}
