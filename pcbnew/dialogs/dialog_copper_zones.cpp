/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <zones.h>
#include <bitmaps.h>
#include <widgets/unit_binder.h>
#include <class_zone.h>
#include <class_board.h>

#include <dialog_copper_zones_base.h>


class DIALOG_COPPER_ZONE : public DIALOG_COPPER_ZONE_BASE
{
public:
    DIALOG_COPPER_ZONE( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings );

private:
    PCB_BASE_FRAME* m_Parent;

    bool            m_settingsExported;     // settings will be written to all other zones

    ZONE_SETTINGS   m_settings;
    ZONE_SETTINGS*  m_ptr;

    bool            m_NetSortingByPadCount;
    long            m_NetFiltering;
    static wxString m_netNameShowFilter;    // the filter to show nets (default * "*").  Static
                                            // to keep this pattern for an entire Pcbnew session
    int             m_cornerSmoothingType;

    UNIT_BINDER     m_cornerRadius;
    UNIT_BINDER     m_clearance;
    UNIT_BINDER     m_minWidth;
    UNIT_BINDER     m_antipadClearance ;
    UNIT_BINDER     m_spokeWidth;

    UNIT_BINDER     m_gridStyleRotation;
    UNIT_BINDER     m_gridStyleThickness;
    UNIT_BINDER     m_gridStyleGap;
    UNIT_BINDER     m_islandThreshold;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    /**
     * @param aUseExportableSetupOnly is true to use exportable parameters only (used to
     *                                export this setup to other zones).
     * @return bool - false if incorrect options, true if ok.
     */
    bool AcceptOptions( bool aUseExportableSetupOnly = false );

    void OnStyleSelection( wxCommandEvent& event ) override;
    void OnLayerSelection( wxDataViewEvent& event ) override;
    void OnNetSortingOptionSelected( wxCommandEvent& event ) override;
    void ExportSetupToOtherCopperZones( wxCommandEvent& event ) override;
    void OnRunFiltersButtonClick( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& ) override;
    void OnButtonCancelClick( wxCommandEvent& event ) override;
    void OnClose( wxCloseEvent& event ) override;

    void buildAvailableListOfNets();
};


// Initialize static member variables
wxString DIALOG_COPPER_ZONE::m_netNameShowFilter( wxT( "*" ) );


int InvokeCopperZonesEditor( PCB_BASE_FRAME* aCaller, ZONE_SETTINGS* aSettings )
{
    DIALOG_COPPER_ZONE dlg( aCaller, aSettings );

    return dlg.ShowModal();
}

#define MIN_THICKNESS ZONE_THICKNESS_MIN_VALUE_MIL*IU_PER_MILS

DIALOG_COPPER_ZONE::DIALOG_COPPER_ZONE( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings ) :
    DIALOG_COPPER_ZONE_BASE( aParent ),
    m_cornerSmoothingType( ZONE_SETTINGS::SMOOTHING_UNDEFINED ),
    m_cornerRadius( aParent, m_cornerRadiusLabel, m_cornerRadiusCtrl, m_cornerRadiusUnits, true ),
    m_clearance( aParent, m_clearanceLabel, m_clearanceCtrl, m_clearanceUnits, true ),
    m_minWidth( aParent, m_minWidthLabel, m_minWidthCtrl, m_minWidthUnits, true ),
    m_antipadClearance( aParent, m_antipadLabel, m_antipadCtrl, m_antipadUnits, true ),
    m_spokeWidth( aParent, m_spokeWidthLabel, m_spokeWidthCtrl, m_spokeWidthUnits, true ),
    m_gridStyleRotation( aParent, m_staticTextGrindOrient, m_tcGridStyleOrientation,
                         m_staticTextRotUnits, false ),
    m_gridStyleThickness( aParent, m_staticTextStyleThickness,
                          m_tcGridStyleThickness, m_GridStyleThicknessUnits, false ),
    m_gridStyleGap( aParent, m_staticTextGridGap, m_tcGridStyleGap, m_GridStyleGapUnits, false ),
    m_islandThreshold( aParent, m_islandThresholdLabel,
                       m_tcIslandThreshold, m_islandThresholdUnits, false )
{
    m_Parent = aParent;
    m_bitmapNoNetWarning->SetBitmap( KiBitmap( dialog_warning_xpm ) );

    m_ptr = aSettings;
    m_settings = *aSettings;
    m_settings.SetupLayersList( m_layers, m_Parent, true );

    m_settingsExported = false;

    m_NetFiltering = false;
    m_NetSortingByPadCount = true;      // false = alphabetic sort, true = pad count sort

    m_sdbSizerOK->SetDefault();

    m_cbRemoveIslands->Bind( wxEVT_CHOICE,
            [&]( wxCommandEvent& )
            {
                // Area mode is index 2
                bool val = m_cbRemoveIslands->GetSelection() == 2;

                m_tcIslandThreshold->Enable( val );
                m_islandThresholdLabel->Enable( val );
                m_islandThresholdUnits->Enable( val );
            } );

    FinishDialogSettings();
}


bool DIALOG_COPPER_ZONE::TransferDataToWindow()
{
    m_constrainOutline->SetValue( m_settings.m_Zone_45_Only );
    m_cornerSmoothingChoice->SetSelection( m_settings.GetCornerSmoothingType() );
    m_cornerRadius.SetValue( m_settings.GetCornerRadius() );
    m_PriorityLevelCtrl->SetValue( m_settings.m_ZonePriority );

    switch( m_settings.m_Zone_HatchingStyle )
    {
    case ZONE_HATCH_STYLE::NO_HATCH:
        m_OutlineAppearanceCtrl->SetSelection( 0 );
        break;
    case ZONE_HATCH_STYLE::DIAGONAL_EDGE:
        m_OutlineAppearanceCtrl->SetSelection( 1 );
        break;
    case ZONE_HATCH_STYLE::DIAGONAL_FULL:
        m_OutlineAppearanceCtrl->SetSelection( 2 );
        break;
    }

    m_clearance.SetValue( m_settings.m_ZoneClearance );
    m_minWidth.SetValue( m_settings.m_ZoneMinThickness );

    switch( m_settings.GetPadConnection() )
    {
    default:
    case ZONE_CONNECTION::THERMAL:
        m_PadInZoneOpt->SetSelection( 1 );
        break;
    case ZONE_CONNECTION::THT_THERMAL:
        m_PadInZoneOpt->SetSelection( 2 );
        break;
    case ZONE_CONNECTION::NONE:
        m_PadInZoneOpt->SetSelection( 3 );
        break;
    case ZONE_CONNECTION::FULL:
        m_PadInZoneOpt->SetSelection( 0 );
        break;
    }

    // Do not enable/disable antipad clearance and spoke width.  They might be needed if
    // a module or pad overrides the zone to specify a thermal connection.
    m_antipadClearance.SetValue( m_settings.m_ThermalReliefGap );
    m_spokeWidth.SetValue( m_settings.m_ThermalReliefCopperBridge );

    m_islandThreshold.SetDataType( EDA_DATA_TYPE::AREA );
    m_islandThreshold.SetDoubleValue( static_cast<double>( m_settings.GetMinIslandArea() ) );

    m_cbRemoveIslands->SetSelection( static_cast<int>( m_settings.GetIslandRemovalMode() ) );

    bool val = m_settings.GetIslandRemovalMode() == ISLAND_REMOVAL_MODE::AREA;

    m_tcIslandThreshold->Enable( val );
    m_islandThresholdLabel->Enable( val );
    m_islandThresholdUnits->Enable( val );

    wxString netNameDoNotShowFilter = wxT( "Net-*" );
    m_NetFiltering = false;
    m_NetSortingByPadCount = true;

    auto cfg = m_Parent->GetPcbNewSettings();

    int opt = cfg->m_Zones.net_sort_mode;
    m_NetFiltering = opt >= 2;
    m_NetSortingByPadCount = opt % 2;

    netNameDoNotShowFilter = cfg->m_Zones.net_filter;

    m_ShowNetNameFilter->SetValue( m_netNameShowFilter );
    m_DoNotShowNetNameFilter->SetValue( netNameDoNotShowFilter );
    m_showAllNetsOpt->SetValue( !m_NetFiltering );
    m_sortByPadsOpt->SetValue( m_NetSortingByPadCount );

    // Build list of nets:
    buildAvailableListOfNets();

    SetInitialFocus( m_ListNetNameSelection );

    switch( m_settings.m_FillMode )
    {
    case ZONE_FILL_MODE::HATCH_PATTERN:
        m_GridStyleCtrl->SetSelection( 1 ); break;
    default:
        m_GridStyleCtrl->SetSelection( 0 ); break;
    }

    m_gridStyleRotation.SetUnits( EDA_UNITS::DEGREES );
    m_gridStyleRotation.SetValue( m_settings.m_HatchFillTypeOrientation*10 ); // IU is decidegree

    // Gives a reasonable value to grid style parameters, if currently there are no defined
    // parameters for grid pattern thickness and gap (if the value is 0)
    // the grid pattern thickness default value is (arbitrary) m_ZoneMinThickness * 4
    // or 1mm
    // the grid pattern gap default value is (arbitrary) m_ZoneMinThickness * 6
    // or 1.5 mm
    int bestvalue = m_settings.m_HatchFillTypeThickness;

    if( bestvalue <= 0 )     // No defined value for m_HatchFillTypeThickness
        bestvalue = std::max( m_settings.m_ZoneMinThickness * 4, Millimeter2iu( 1.0 ) );

    m_gridStyleThickness.SetValue( std::max( bestvalue, m_settings.m_ZoneMinThickness ) );

    bestvalue = m_settings.m_HatchFillTypeGap;

    if( bestvalue <= 0 )     // No defined value for m_HatchFillTypeGap
        bestvalue = std::max( m_settings.m_ZoneMinThickness * 6, Millimeter2iu( 1.5 ) );

    m_gridStyleGap.SetValue( std::max( bestvalue, m_settings.m_ZoneMinThickness ) );

    m_spinCtrlSmoothLevel->SetValue( m_settings.m_HatchFillTypeSmoothingLevel );
    m_spinCtrlSmoothValue->SetValue( m_settings.m_HatchFillTypeSmoothingValue );

    m_tcZoneName->SetValue( m_settings.m_Name );

    // Enable/Disable some widgets
    wxCommandEvent event;
    OnStyleSelection( event );

    Fit();

    return true;
}


void DIALOG_COPPER_ZONE::OnUpdateUI( wxUpdateUIEvent& )
{
    if( m_ListNetNameSelection->GetSelection() < 0 )
        m_ListNetNameSelection->SetSelection( 0 );

    bool noNetSelected = m_ListNetNameSelection->GetSelection() == 0;
    bool enableSize    = !noNetSelected && ( m_cbRemoveIslands->GetSelection() == 2 );

    m_bNoNetWarning->Show( noNetSelected );

    // Zones with no net never have islands removed
    m_cbRemoveIslands->Enable( !noNetSelected );
    m_islandThresholdLabel->Enable( enableSize );
    m_islandThresholdUnits->Enable( enableSize );
    m_tcIslandThreshold->Enable( enableSize );

    if( m_cornerSmoothingType != m_cornerSmoothingChoice->GetSelection() )
    {
        m_cornerSmoothingType = m_cornerSmoothingChoice->GetSelection();

        if( m_cornerSmoothingChoice->GetSelection() == ZONE_SETTINGS::SMOOTHING_CHAMFER )
            m_cornerRadiusLabel->SetLabel( _( "Chamfer distance:" ) );
        else
            m_cornerRadiusLabel->SetLabel( _( "Fillet radius:" ) );
    }

    m_cornerRadiusCtrl->Enable(m_cornerSmoothingType > ZONE_SETTINGS::SMOOTHING_NONE );
}


void DIALOG_COPPER_ZONE::OnButtonCancelClick( wxCommandEvent& event )
{
    // After an "Export Settings to Other Zones" cancel and close must return
    // ZONE_EXPORT_VALUES instead of wxID_CANCEL.
    Close( true );
}


bool DIALOG_COPPER_ZONE::TransferDataFromWindow()
{
    m_netNameShowFilter = m_ShowNetNameFilter->GetValue();

    if( m_GridStyleCtrl->GetSelection() > 0 )
        m_settings.m_FillMode = ZONE_FILL_MODE::HATCH_PATTERN;
    else
        m_settings.m_FillMode = ZONE_FILL_MODE::POLYGONS;

    if( !AcceptOptions() )
        return false;

    m_settings.m_HatchFillTypeOrientation = m_gridStyleRotation.GetValue()/10.0; // value is returned in deci-degree
    m_settings.m_HatchFillTypeThickness = m_gridStyleThickness.GetValue();
    m_settings.m_HatchFillTypeGap = m_gridStyleGap.GetValue();
    m_settings.m_HatchFillTypeSmoothingLevel = m_spinCtrlSmoothLevel->GetValue();
    m_settings.m_HatchFillTypeSmoothingValue = m_spinCtrlSmoothValue->GetValue();

    *m_ptr = m_settings;
    return true;
}


void DIALOG_COPPER_ZONE::OnClose( wxCloseEvent& event )
{
    EndModal( m_settingsExported ? ZONE_EXPORT_VALUES : wxID_CANCEL );
}


bool DIALOG_COPPER_ZONE::AcceptOptions( bool aUseExportableSetupOnly )
{
    if( !m_clearance.Validate( 0, Mils2iu( ZONE_CLEARANCE_MAX_VALUE_MIL ) ) )
        return false;

    if( !m_minWidth.Validate( Mils2iu( ZONE_THICKNESS_MIN_VALUE_MIL ), INT_MAX ) )
        return false;

    if( !m_cornerRadius.Validate( 0, INT_MAX ) )
        return false;

    if( !m_spokeWidth.Validate( 0, INT_MAX ) )
        return false;

    m_gridStyleRotation.SetValue( NormalizeAngle180( m_gridStyleRotation.GetValue() ) );

    if( m_settings.m_FillMode == ZONE_FILL_MODE::HATCH_PATTERN )
    {
        int minThickness = m_minWidth.GetValue();

        if( !m_gridStyleThickness.Validate( minThickness, INT_MAX ) )
            return false;

        if( !m_gridStyleGap.Validate( minThickness, INT_MAX ) )
            return false;
    }

    switch( m_PadInZoneOpt->GetSelection() )
    {
    case 3:
        m_settings.SetPadConnection( ZONE_CONNECTION::NONE );
        break;
    case 2:
        m_settings.SetPadConnection( ZONE_CONNECTION::THT_THERMAL );
        break;
    case 1:
        m_settings.SetPadConnection( ZONE_CONNECTION::THERMAL );
        break;
    case 0:
        m_settings.SetPadConnection( ZONE_CONNECTION::FULL );
        break;
    }

    switch( m_OutlineAppearanceCtrl->GetSelection() )
    {
    case 0:
        m_settings.m_Zone_HatchingStyle = ZONE_HATCH_STYLE::NO_HATCH;
        break;
    case 1:
        m_settings.m_Zone_HatchingStyle = ZONE_HATCH_STYLE::DIAGONAL_EDGE;
        break;
    case 2:
        m_settings.m_Zone_HatchingStyle = ZONE_HATCH_STYLE::DIAGONAL_FULL;
        break;
    }

    auto cfg = m_Parent->GetPcbNewSettings();

    cfg->m_Zones.hatching_style = static_cast<int>( m_settings.m_Zone_HatchingStyle );
    cfg->m_Zones.net_filter = m_DoNotShowNetNameFilter->GetValue().ToStdString();

    m_netNameShowFilter = m_ShowNetNameFilter->GetValue();

    m_settings.m_ZoneClearance = m_clearance.GetValue();
    m_settings.m_ZoneMinThickness = m_minWidth.GetValue();

    m_settings.SetCornerSmoothingType( m_cornerSmoothingChoice->GetSelection() );

    m_settings.SetCornerRadius( m_settings.GetCornerSmoothingType() == ZONE_SETTINGS::SMOOTHING_NONE
                                ? 0 : m_cornerRadius.GetValue() );

    m_settings.m_ZonePriority = m_PriorityLevelCtrl->GetValue();

    m_settings.m_Zone_45_Only = m_constrainOutline->GetValue();

    m_settings.m_ThermalReliefGap = m_antipadClearance.GetValue();
    m_settings.m_ThermalReliefCopperBridge = m_spokeWidth.GetValue();

    if( m_settings.m_ThermalReliefCopperBridge < m_settings.m_ZoneMinThickness )
    {
        DisplayError( this, _( "Thermal spoke width cannot be smaller than the minimum width." ) );
        return false;
    }

    cfg->m_Zones.clearance                   = Iu2Mils( m_settings.m_ZoneClearance );
    cfg->m_Zones.min_thickness               = Iu2Mils( m_settings.m_ZoneMinThickness );
    cfg->m_Zones.thermal_relief_gap          = Iu2Mils( m_settings.m_ThermalReliefGap );
    cfg->m_Zones.thermal_relief_copper_width = Iu2Mils( m_settings.m_ThermalReliefCopperBridge );

    m_settings.SetIslandRemovalMode(
            static_cast<ISLAND_REMOVAL_MODE>( m_cbRemoveIslands->GetSelection() ) );
    m_settings.SetMinIslandArea( m_islandThreshold.GetValue() );

    // If we use only exportable to others zones parameters, exit here:
    if( aUseExportableSetupOnly )
        return true;

    // Get the layer selection for this zone
    int layer = -1;
    for( int ii = 0; ii < m_layers->GetItemCount(); ++ii )
    {
        if( m_layers->GetToggleValue( (unsigned) ii, 0 ) )
        {
            layer = ii;
            break;
        }
    }

    if( layer < 0 )
    {
        DisplayError( this, _( "No layer selected." ) );
        return false;
    }

    NETINFO_ITEM* net = nullptr;

    // Search net_code for this net, if a net was selected
    if( m_ListNetNameSelection->GetSelection() > 0 )
    {
        wxString netname = m_ListNetNameSelection->GetStringSelection();
        net = m_Parent->GetBoard()->FindNet( netname );
    }

    m_settings.m_NetcodeSelection = net ? net->GetNet() : 0;

    m_settings.m_Name = m_tcZoneName->GetValue();

    return true;
}


void DIALOG_COPPER_ZONE::OnStyleSelection( wxCommandEvent& event )
{
    bool enable = m_GridStyleCtrl->GetSelection() >= 1;
    m_tcGridStyleThickness->Enable( enable );
    m_tcGridStyleGap->Enable( enable );
    m_tcGridStyleOrientation->Enable( enable );
    m_spinCtrlSmoothLevel->Enable( enable );
    m_spinCtrlSmoothValue->Enable( enable );
}


void DIALOG_COPPER_ZONE::OnLayerSelection( wxDataViewEvent& event )
{
    if( event.GetColumn() != 0 )
        return;

    int row = m_layers->ItemToRow( event.GetItem() );

    wxVariant layerID;
    m_layers->GetValue( layerID, row, 2 );
    m_settings.m_Layers.set( ToLAYER_ID( layerID.GetInteger() ),
            m_layers->GetToggleValue( row, 0 ) );
}


void DIALOG_COPPER_ZONE::OnNetSortingOptionSelected( wxCommandEvent& event )
{
    m_NetFiltering = !m_showAllNetsOpt->GetValue();
    m_NetSortingByPadCount = m_sortByPadsOpt->GetValue();
    m_netNameShowFilter = m_ShowNetNameFilter->GetValue();

    buildAvailableListOfNets();

    auto cfg = m_Parent->GetPcbNewSettings();

    int configValue = m_NetFiltering ? 2 : 0;

    if( m_NetSortingByPadCount )
        configValue += 1;

    cfg->m_Zones.net_sort_mode = configValue;
    cfg->m_Zones.net_filter = m_DoNotShowNetNameFilter->GetValue().ToStdString();
}


void DIALOG_COPPER_ZONE::ExportSetupToOtherCopperZones( wxCommandEvent& event )
{
    if( !AcceptOptions( true ) )
        return;

    // Export settings ( but layer and netcode ) to others copper zones
    BOARD* pcb = m_Parent->GetBoard();

    for( int ii = 0; ii < pcb->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = pcb->GetArea( ii );

        // Cannot export settings from a copper zone
        // to a zone keepout:
        if( zone->GetIsKeepout() )
            continue;

        m_settings.ExportSetting( *zone, false );  // false = partial export
        m_settingsExported = true;
        m_Parent->OnModify();
    }
}


void DIALOG_COPPER_ZONE::OnRunFiltersButtonClick( wxCommandEvent& event )
{
    m_NetFiltering = true;
    m_showAllNetsOpt->SetValue( false );

    buildAvailableListOfNets();
}


void DIALOG_COPPER_ZONE::buildAvailableListOfNets()
{
    wxArrayString   listNetName;

    m_Parent->GetBoard()->SortedNetnamesList( listNetName, m_NetSortingByPadCount );

    if( m_NetFiltering )
    {
        wxString doNotShowFilter = m_DoNotShowNetNameFilter->GetValue();
        wxString ShowFilter = m_ShowNetNameFilter->GetValue();

        for( unsigned ii = 0; ii < listNetName.GetCount(); ii++ )
        {
            if( listNetName[ii].Matches( doNotShowFilter ) )
            {
                listNetName.RemoveAt( ii );
                ii--;
            }
            else if( !listNetName[ii].Matches( ShowFilter ) )
            {
                listNetName.RemoveAt( ii );
                ii--;
            }
        }
    }

    listNetName.Insert( wxT( "<no net>" ), 0 );

    // Ensure currently selected net for the zone is visible, regardless of filters
    int selectedNetListNdx = 0;
    int net_select = m_settings.m_NetcodeSelection;

    if( net_select > 0 )
    {
        NETINFO_ITEM* selectedNet = m_Parent->GetBoard()->FindNet( net_select );
        if( selectedNet )
        {
            selectedNetListNdx = listNetName.Index( selectedNet->GetNetname() );

            if( wxNOT_FOUND == selectedNetListNdx )
            {
                // the currently selected net must *always* be visible.
		        // <no net> is the zero'th index, so pick next lowest
                listNetName.Insert( selectedNet->GetNetname(), 1 );
                selectedNetListNdx = 1;
            }
        }
    }

    m_ListNetNameSelection->Clear();
    m_ListNetNameSelection->InsertItems( listNetName, 0 );
    m_ListNetNameSelection->SetSelection( selectedNetListNdx );
    m_ListNetNameSelection->EnsureVisible( selectedNetListNdx );
}

