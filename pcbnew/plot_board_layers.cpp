/**
 * @file plot_board_layers.cpp
 * @brief Functions to plot one board layer (silkscreen layers or other layers).
 * Silkscreen layers have specific requirement for pads (not filled) and texts
 * (with option to remove them from some copper areas (pads...)
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <common.h>
#include <plotter.h>
#include <base_struct.h>
#include <gr_text.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_segment.h>
#include <trigo.h>
#include <pcb_base_frame.h>
#include <macros.h>
#include <math/util.h>      // for KiROUND

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <class_pcb_text.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_pcb_target.h>
#include <class_dimension.h>

#include <pcbnew.h>
#include <pcbplot.h>
#include <pcb_painter.h>
#include <gbr_metadata.h>

/*
 * Plot a solder mask layer.  Solder mask layers have a minimum thickness value and cannot be
 * drawn like standard layers, unless the minimum thickness is 0.
 */
static void PlotSolderMaskLayer( BOARD *aBoard, PLOTTER* aPlotter, LSET aLayerMask,
                                 const PCB_PLOT_PARAMS& aPlotOpt, int aMinThickness );

void PlotOneBoardLayer( BOARD *aBoard, PLOTTER* aPlotter, PCB_LAYER_ID aLayer,
                        const PCB_PLOT_PARAMS& aPlotOpt )
{
    PCB_PLOT_PARAMS plotOpt = aPlotOpt;
    int soldermask_min_thickness = aBoard->GetDesignSettings().m_SolderMaskMinWidth;

    // Set a default color and the text mode for this layer
    aPlotter->SetColor( aPlotOpt.GetColor() );
    aPlotter->SetTextMode( aPlotOpt.GetTextMode() );

    // Specify that the contents of the "Edges Pcb" layer are to be plotted in addition to the
    // contents of the currently specified layer.
    LSET    layer_mask( aLayer );

    if( !aPlotOpt.GetExcludeEdgeLayer() )
        layer_mask.set( Edge_Cuts );

    if( IsCopperLayer( aLayer ) )
    {
        // Skip NPTH pads on copper layers ( only if hole size == pad size ):
        // Drill mark will be plotted if drill mark is SMALL_DRILL_SHAPE  or FULL_DRILL_SHAPE
        if( plotOpt.GetFormat() == PLOT_FORMAT::DXF )
        {
            plotOpt.SetSkipPlotNPTH_Pads( false );
            PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
        }
        else
        {
            plotOpt.SetSkipPlotNPTH_Pads( true );
            PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
        }
    }
    else
    {
        switch( aLayer )
        {
        case B_Mask:
        case F_Mask:
            plotOpt.SetSkipPlotNPTH_Pads( false );
            // Disable plot pad holes
            plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

            // Plot solder mask:
            if( soldermask_min_thickness == 0 )
            {
                if( plotOpt.GetFormat() == PLOT_FORMAT::DXF )
                    PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
                else
                    PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
            }
            else
                PlotSolderMaskLayer( aBoard, aPlotter, layer_mask, plotOpt,
                                     soldermask_min_thickness );

            break;

        case B_Adhes:
        case F_Adhes:
        case B_Paste:
        case F_Paste:
            plotOpt.SetSkipPlotNPTH_Pads( false );
            // Disable plot pad holes
            plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

            if( plotOpt.GetFormat() == PLOT_FORMAT::DXF )
                PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
            else
                PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
            break;

        case F_SilkS:
        case B_SilkS:
            if( plotOpt.GetFormat() == PLOT_FORMAT::DXF && plotOpt.GetDXFPlotPolygonMode() )
                // PlotLayerOutlines() is designed only for DXF plotters.
                // and must not be used for other plot formats
                PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
            else
                PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );

            // Gerber: Subtract soldermask from silkscreen if enabled
            if( aPlotter->GetPlotterType() == PLOT_FORMAT::GERBER
                    && plotOpt.GetSubtractMaskFromSilk() )
            {
                if( aLayer == F_SilkS )
                    layer_mask = LSET( F_Mask );
                else
                    layer_mask = LSET( B_Mask );

                // Create the mask to subtract by creating a negative layer polarity
                aPlotter->SetLayerPolarity( false );

                // Disable plot pad holes
                plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

                // Plot the mask
                PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
            }
            break;

        // These layers are plotted like silk screen layers.
        // Mainly, pads on these layers are not filled.
        // This is not necessary the best choice.
        case Dwgs_User:
        case Cmts_User:
        case Eco1_User:
        case Eco2_User:
        case Edge_Cuts:
        case Margin:
        case F_CrtYd:
        case B_CrtYd:
        case F_Fab:
        case B_Fab:
            plotOpt.SetSkipPlotNPTH_Pads( false );
            plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

            if( plotOpt.GetFormat() == PLOT_FORMAT::DXF && plotOpt.GetDXFPlotPolygonMode() )
                // PlotLayerOutlines() is designed only for DXF plotters.
                // and must not be used for other plot formats
                PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
            else
                PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
            break;

        default:
            plotOpt.SetSkipPlotNPTH_Pads( false );
            plotOpt.SetDrillMarksType( PCB_PLOT_PARAMS::NO_DRILL_SHAPE );

            if( plotOpt.GetFormat() == PLOT_FORMAT::DXF && plotOpt.GetDXFPlotPolygonMode() )
                // PlotLayerOutlines() is designed only for DXF plotters.
                // and must not be used for other plot formats
                PlotLayerOutlines( aBoard, aPlotter, layer_mask, plotOpt );
            else
                PlotStandardLayer( aBoard, aPlotter, layer_mask, plotOpt );
            break;
        }
    }
}


/* Plot a copper layer or mask.
 * Silk screen layers are not plotted here.
 */
void PlotStandardLayer( BOARD *aBoard, PLOTTER* aPlotter,
                        LSET aLayerMask, const PCB_PLOT_PARAMS& aPlotOpt )
{
    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );

    itemplotter.SetLayerSet( aLayerMask );

    EDA_DRAW_MODE_T plotMode = aPlotOpt.GetPlotMode();
    bool onCopperLayer = ( LSET::AllCuMask() & aLayerMask ).any();
    bool onSolderMaskLayer = ( LSET( 2, F_Mask, B_Mask ) & aLayerMask ).any();
    bool onSolderPasteLayer = ( LSET( 2, F_Paste, B_Paste ) & aLayerMask ).any();
    bool onFrontFab = ( LSET(  F_Fab ) & aLayerMask ).any();
    bool onBackFab  = ( LSET( B_Fab ) & aLayerMask ).any();
    bool sketchPads = ( onFrontFab || onBackFab ) && aPlotOpt.GetSketchPadsOnFabLayers();

     // Plot edge layer and graphic items
    itemplotter.PlotBoardGraphicItems();

    // Draw footprint texts:
    for( MODULE* module : aBoard->Modules() )
        itemplotter.PlotFootprintTextItems( module );

    // Draw footprint other graphic items:
    for( MODULE* module : aBoard->Modules() )
        itemplotter.PlotFootprintGraphicItems( module );

    // Plot footprint pads
    for( MODULE* module : aBoard->Modules() )
    {
        aPlotter->StartBlock( NULL );

        for( D_PAD* pad : module->Pads() )
        {
            EDA_DRAW_MODE_T padPlotMode = plotMode;

            if( !( pad->GetLayerSet() & aLayerMask ).any() )
            {
                if( sketchPads &&
                        ( ( onFrontFab && pad->GetLayerSet().Contains( F_Cu ) ) ||
                          ( onBackFab && pad->GetLayerSet().Contains( B_Cu ) ) ) )
                    padPlotMode = SKETCH;
                else
                    continue;
            }

            COLOR4D color = COLOR4D::BLACK;

            if( pad->GetLayerSet()[B_Cu] )
               color = aPlotOpt.ColorSettings()->GetColor( LAYER_PAD_BK );

            if( pad->GetLayerSet()[F_Cu] )
                color = color.LegacyMix( aPlotOpt.ColorSettings()->GetColor( LAYER_PAD_FR ) );

            if( sketchPads && aLayerMask[F_Fab] )
                color = aPlotOpt.ColorSettings()->GetColor( F_Fab );
            else if( sketchPads && aLayerMask[B_Fab] )
                color = aPlotOpt.ColorSettings()->GetColor( B_Fab );

            wxSize margin;
            int width_adj = 0;

            if( onCopperLayer )
                width_adj =  itemplotter.getFineWidthAdj();

            if( onSolderMaskLayer )
                margin.x = margin.y = pad->GetSolderMaskMargin();

            if( onSolderPasteLayer )
                margin = pad->GetSolderPasteMargin();

            // Now offset the pad size by margin + width_adj
            wxSize padPlotsSize = pad->GetSize() + margin * 2 + wxSize( width_adj, width_adj );

            // Store these parameters that can be modified to plot inflated/deflated pads shape
            PAD_SHAPE_T padShape = pad->GetShape();
            wxSize      padSize = pad->GetSize();
            wxSize      padDelta = pad->GetDelta(); // has meaning only for trapezoidal pads
            double      padCornerRadius = pad->GetRoundRectCornerRadius();

            // Don't draw a null size item :
            if( padPlotsSize.x <= 0 || padPlotsSize.y <= 0 )
                continue;

            switch( pad->GetShape() )
            {
            case PAD_SHAPE_CIRCLE:
            case PAD_SHAPE_OVAL:
                pad->SetSize( padPlotsSize );

                if( aPlotOpt.GetSkipPlotNPTH_Pads() &&
                    ( aPlotOpt.GetDrillMarksType() == PCB_PLOT_PARAMS::NO_DRILL_SHAPE ) &&
                    ( pad->GetSize() == pad->GetDrillSize() ) &&
                    ( pad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED ) )
                    break;

                itemplotter.PlotPad( pad, color, padPlotMode );
                break;

            case PAD_SHAPE_RECT:
                if( margin.x > 0 )
                {
                    pad->SetShape( PAD_SHAPE_ROUNDRECT );
                    pad->SetRoundRectCornerRadius( margin.x );
                }

                pad->SetSize( padPlotsSize );
                itemplotter.PlotPad( pad, color, padPlotMode );
                break;

            case PAD_SHAPE_TRAPEZOID:
            {
                wxSize scale( padPlotsSize.x / padSize.x, padPlotsSize.y / padSize.y );
                pad->SetDelta( wxSize( padDelta.x * scale.x, padDelta.y * scale.y ) );

                pad->SetSize( padPlotsSize );
                itemplotter.PlotPad( pad, color, padPlotMode );
            }
                break;

            case PAD_SHAPE_ROUNDRECT:
            case PAD_SHAPE_CHAMFERED_RECT:
                // Chamfer and rounding are stored as a percent and so don't need scaling
                pad->SetSize( padPlotsSize );
                itemplotter.PlotPad( pad, color, padPlotMode );
                break;

            case PAD_SHAPE_CUSTOM:
            {
                // inflate/deflate a custom shape is a bit complex.
                // so build a similar pad shape, and inflate/deflate the polygonal shape
                D_PAD dummy( *pad );
                SHAPE_POLY_SET shape;
                pad->MergePrimitivesAsPolygon( &shape );
                // Shape polygon can have holes so use InflateWithLinkedHoles(), not Inflate()
                // which can create bad shapes if margin.x is < 0
                int maxError = aBoard->GetDesignSettings().m_MaxError;
                int numSegs = std::max( GetArcToSegmentCount( margin.x, maxError, 360.0 ), 6 );
                shape.InflateWithLinkedHoles( margin.x, numSegs, SHAPE_POLY_SET::PM_FAST );
                dummy.DeletePrimitivesList();
                dummy.AddPrimitivePoly( shape, 0 );

                // Be sure the anchor pad is not bigger than the deflated shape because this
                // anchor will be added to the pad shape when plotting the pad. So now the
                // polygonal shape is built, we can clamp the anchor size
                if( margin.x < 0 )  // we expect margin.x = margin.y for custom pads
                    dummy.SetSize( padPlotsSize );

                itemplotter.PlotPad( &dummy, color, padPlotMode );
            }
                break;
            }

            // Restore the pad parameters modified by the plot code
            pad->SetSize( padSize );
            pad->SetDelta( padDelta );
            pad->SetShape( padShape );
            pad->SetRoundRectCornerRadius( padCornerRadius );
        }

        aPlotter->EndBlock( NULL );
    }

    // Plot vias on copper layers, and if aPlotOpt.GetPlotViaOnMaskLayer() is true,
    // plot them on solder mask

    GBR_METADATA gbr_metadata;

    bool isOnCopperLayer = ( aLayerMask & LSET::AllCuMask() ).any();

    if( isOnCopperLayer )
    {
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_VIAPAD );
        gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_NET );
    }

    aPlotter->StartBlock( NULL );

    for( auto track : aBoard->Tracks() )
    {
        const VIA* Via = dyn_cast<const VIA*>( track );

        if( !Via )
            continue;

        // vias are not plotted if not on selected layer, but if layer is SOLDERMASK_LAYER_BACK
        // or SOLDERMASK_LAYER_FRONT, vias are drawn only if they are on the corresponding
        // external copper layer
        LSET via_mask_layer = Via->GetLayerSet();

        if( aPlotOpt.GetPlotViaOnMaskLayer() )
        {
            if( via_mask_layer[B_Cu] )
                via_mask_layer.set( B_Mask );

            if( via_mask_layer[F_Cu] )
                via_mask_layer.set( F_Mask );
        }

        if( !( via_mask_layer & aLayerMask ).any() )
            continue;

        int via_margin = 0;
        double width_adj = 0;

        // If the current layer is a solder mask, use the global mask clearance for vias
        if( aLayerMask[B_Mask] || aLayerMask[F_Mask] )
            via_margin = aBoard->GetDesignSettings().m_SolderMaskMargin;

        if( ( aLayerMask & LSET::AllCuMask() ).any() )
            width_adj = itemplotter.getFineWidthAdj();

        int diameter = Via->GetWidth() + 2 * via_margin + width_adj;

        // Don't draw a null size item :
        if( diameter <= 0 )
            continue;

        // Some vias can be not connected (no net).
        // Set the m_NotInNet for these vias to force a empty net name in gerber file
        gbr_metadata.m_NetlistMetadata.m_NotInNet = Via->GetNetname().IsEmpty();

        gbr_metadata.SetNetName( Via->GetNetname() );

        COLOR4D color = aPlotOpt.ColorSettings()->GetColor(
                LAYER_VIAS + static_cast<int>( Via->GetViaType() ) );
        // Set plot color (change WHITE to LIGHTGRAY because the white items are not seen on a
        // white paper or screen
        aPlotter->SetColor( color != WHITE ? color : LIGHTGRAY );
        aPlotter->FlashPadCircle( Via->GetStart(), diameter, plotMode, &gbr_metadata );
    }

    aPlotter->EndBlock( NULL );
    aPlotter->StartBlock( NULL );
    gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONDUCTOR );

    // Plot tracks (not vias) :
    for( auto track : aBoard->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            continue;

        if( !aLayerMask[track->GetLayer()] )
            continue;

        // Some track segments can be not connected (no net).
        // Set the m_NotInNet for these segments to force a empty net name in gerber file
        gbr_metadata.m_NetlistMetadata.m_NotInNet = track->GetNetname().IsEmpty();

        gbr_metadata.SetNetName( track->GetNetname() );
        int width = track->GetWidth() + itemplotter.getFineWidthAdj();
        aPlotter->SetColor( itemplotter.getColor( track->GetLayer() ) );

        if( track->Type() == PCB_ARC_T )
        {
            ARC* arc = static_cast<ARC*>( track );
            VECTOR2D center( arc->GetCenter() );
            auto radius = arc->GetRadius();
            auto start_angle = arc->GetArcAngleStart();
            auto end_angle = start_angle + arc->GetAngle();

            aPlotter->ThickArc( wxPoint( center.x, center.y ), -end_angle, -start_angle,
                                radius, width, plotMode, &gbr_metadata );
        }
        else
            aPlotter->ThickSegment( track->GetStart(), track->GetEnd(), width, plotMode, &gbr_metadata );
    }

    aPlotter->EndBlock( NULL );

    // Plot filled ares
    aPlotter->StartBlock( NULL );

    // Plot all zones of the same layer & net together so we don't end up with divots where
    // zones touch each other.
    std::set<std::pair<PCB_LAYER_ID, ZONE_CONTAINER*>> plotted;

    NETINFO_ITEM nonet( aBoard );

    for( ZONE_CONTAINER* zone : aBoard->Zones() )
    {
        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            auto pair = std::make_pair( layer, zone );

            if( !aLayerMask[layer] || plotted.count( pair )  )
                continue;

            plotted.insert( pair );

            SHAPE_POLY_SET aggregateArea = zone->GetFilledPolysList( layer );
            SHAPE_POLY_SET islands;
            bool needFracture = false; // If 2 or more filled areas are combined, resulting
                                       // aggregateArea will be simplified and fractured
                                       // (Long calculation time)

            for( int i = 0; i < aggregateArea.OutlineCount(); i++ )
            {
                if( zone->IsIsland( layer, i ) )
                {
                    islands.AddOutline( aggregateArea.CPolygon( i )[0] );
                    aggregateArea.DeletePolygon( i );
                }
            }

            for( ZONE_CONTAINER* candidate : aBoard->Zones() )
            {
                if( !candidate->IsOnLayer( layer ) )
                    continue;

                auto candidate_pair = std::make_pair( layer, candidate );

                if( plotted.count( candidate_pair ) )
                    continue;

                if( candidate->GetNetCode() != zone->GetNetCode() )
                    continue;

                // Merging zones of the same net can be done only for areas
                // having compatible settings for drawings:
                // use or not outline thickness, and if using outline thickness,
                // having the same thickness
                // because after merging only one outline thickness is used
                if( candidate->GetFilledPolysUseThickness() != zone->GetFilledPolysUseThickness() )
                    // Should not happens, because usually the same option is used for filling
                    continue;

                if( zone->GetFilledPolysUseThickness()
                        && ( candidate->GetMinThickness() != zone->GetMinThickness() ) )
                    continue;

                plotted.insert( candidate_pair );

                SHAPE_POLY_SET candidateArea = candidate->GetFilledPolysList( layer );

                for( int i = 0; i < candidateArea.OutlineCount(); i++ )
                {
                    if( candidate->IsIsland( layer, i ) )
                    {
                        islands.AddOutline( candidateArea.CPolygon( i )[0] );
                        candidateArea.DeletePolygon( i );
                    }
                }

                aggregateArea.Append( candidateArea );
                needFracture = true;
            }

            if( needFracture )
            {
                aggregateArea.Unfracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
                aggregateArea.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
            }

            itemplotter.PlotFilledAreas( zone, aggregateArea );

            if( !islands.IsEmpty() )
            {
                ZONE_CONTAINER dummy( *zone );
                dummy.SetNet( &nonet );
                itemplotter.PlotFilledAreas( &dummy, islands );
            }
        }
    }
    aPlotter->EndBlock( NULL );

    // Adding drill marks, if required and if the plotter is able to plot them:
    if( aPlotOpt.GetDrillMarksType() != PCB_PLOT_PARAMS::NO_DRILL_SHAPE )
        itemplotter.PlotDrillMarks();
}


// Seems like we want to plot from back to front?
static const PCB_LAYER_ID plot_seq[] = {

    B_Adhes,        // 32
    F_Adhes,
    B_Paste,
    F_Paste,
    B_SilkS,
    B_Mask,
    F_Mask,
    Dwgs_User,
    Cmts_User,
    Eco1_User,
    Eco2_User,
    Edge_Cuts,
    Margin,

    F_CrtYd,        // CrtYd & Body are footprint only
    B_CrtYd,
    F_Fab,
    B_Fab,

    B_Cu,
    In30_Cu,
    In29_Cu,
    In28_Cu,
    In27_Cu,
    In26_Cu,
    In25_Cu,
    In24_Cu,
    In23_Cu,
    In22_Cu,
    In21_Cu,
    In20_Cu,
    In19_Cu,
    In18_Cu,
    In17_Cu,
    In16_Cu,
    In15_Cu,
    In14_Cu,
    In13_Cu,
    In12_Cu,
    In11_Cu,
    In10_Cu,
    In9_Cu,
    In8_Cu,
    In7_Cu,
    In6_Cu,
    In5_Cu,
    In4_Cu,
    In3_Cu,
    In2_Cu,
    In1_Cu,
    F_Cu,

    F_SilkS,
};


/*
 * Plot outlines of copper, for copper layer
 */
void PlotLayerOutlines( BOARD* aBoard, PLOTTER* aPlotter, LSET aLayerMask,
                        const PCB_PLOT_PARAMS& aPlotOpt )
{
    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    itemplotter.SetLayerSet( aLayerMask );

    SHAPE_POLY_SET outlines;

    for( LSEQ seq = aLayerMask.Seq( plot_seq, arrayDim( plot_seq ) );  seq;  ++seq )
    {
        PCB_LAYER_ID layer = *seq;

        outlines.RemoveAllContours();
        aBoard->ConvertBrdLayerToPolygonalContours( layer, outlines );

        outlines.Simplify( SHAPE_POLY_SET::PM_FAST );

        // Plot outlines
        std::vector<wxPoint> cornerList;

        // Now we have one or more basic polygons: plot each polygon
        for( int ii = 0; ii < outlines.OutlineCount(); ii++ )
        {
            for(int kk = 0; kk <= outlines.HoleCount (ii); kk++ )
            {
                cornerList.clear();
                const SHAPE_LINE_CHAIN& path = (kk == 0) ? outlines.COutline( ii ) : outlines.CHole( ii, kk - 1 );

                for( int jj = 0; jj < path.PointCount(); jj++ )
                    cornerList.emplace_back( (wxPoint) path.CPoint( jj ) );

                // Ensure the polygon is closed
                if( cornerList[0] != cornerList[cornerList.size() - 1] )
                    cornerList.push_back( cornerList[0] );

                aPlotter->PlotPoly( cornerList, NO_FILL );
            }
        }

        // Plot pad holes
        if( aPlotOpt.GetDrillMarksType() != PCB_PLOT_PARAMS::NO_DRILL_SHAPE )
        {
            int smallDrill = (aPlotOpt.GetDrillMarksType() == PCB_PLOT_PARAMS::SMALL_DRILL_SHAPE)
                                  ? SMALL_DRILL : INT_MAX;

            for( MODULE* module : aBoard->Modules() )
            {
                for( D_PAD* pad : module->Pads() )
                {
                    wxSize hole = pad->GetDrillSize();

                    if( hole.x == 0 || hole.y == 0 )
                        continue;

                    if( hole.x == hole.y )
                    {
                        hole.x = std::min( smallDrill, hole.x );
                        aPlotter->Circle( pad->GetPosition(), hole.x, NO_FILL );
                    }
                    else
                    {
                        // Note: small drill marks have no significance when applied to slots
                        const std::shared_ptr<SHAPE_SEGMENT>& seg = pad->GetEffectiveHoleShape();
                        aPlotter->ThickSegment( (wxPoint) seg->GetSeg().A,
                                                (wxPoint) seg->GetSeg().B,
                                                seg->GetWidth(), SKETCH, NULL );
                    }
                }
            }
        }

        // Plot vias holes
        for( TRACK* track : aBoard->Tracks() )
        {
            const VIA* via = dyn_cast<const VIA*>( track );

            if( via && via->IsOnLayer( layer ) )    // via holes can be not through holes
            {
                aPlotter->Circle( via->GetPosition(), via->GetDrillValue(), NO_FILL );
            }
        }
    }
}


/* Plot a solder mask layer.
 * Solder mask layers have a minimum thickness value and cannot be drawn like standard layers,
 * unless the minimum thickness is 0.
 * Currently the algo is:
 * 1 - build all pad shapes as polygons with a size inflated by
 *      mask clearance + (min width solder mask /2)
 * 2 - Merge shapes
 * 3 - deflate result by (min width solder mask /2)
 * 4 - ORing result by all pad shapes as polygons with a size inflated by
 *      mask clearance only (because deflate sometimes creates shape artifacts)
 * 5 - draw result as polygons
 *
 * We have 2 algos:
 * the initial algo, that create polygons for every shape, inflate and deflate polygons
 * with Min Thickness/2, and merges the result.
 * Drawback: pads attributes are lost (annoying in Gerber)
 * the new algo:
 * create initial polygons for every shape (pad or polygon),
 * inflate and deflate polygons
 * with Min Thickness/2, and merges the result (like initial algo)
 * remove all initial polygons.
 * The remaining polygons are areas with thickness < min thickness
 * plot all initial shapes by flashing (or using regions) for pad and polygons
 * (shapes will be better) and remaining polygons to
 * remove areas with thickness < min thickness from final mask
 *
 * TODO: remove old code after more testing.
 */
#define NEW_ALGO 1

void PlotSolderMaskLayer( BOARD *aBoard, PLOTTER* aPlotter, LSET aLayerMask,
                          const PCB_PLOT_PARAMS& aPlotOpt, int aMinThickness )
{
    PCB_LAYER_ID    layer = aLayerMask[B_Mask] ? B_Mask : F_Mask;

    // Set the current arc to segment max approx error
    int currMaxError = aBoard->GetDesignSettings().m_MaxError;
    aBoard->GetDesignSettings().m_MaxError = Millimeter2iu( 0.005 );

    // We remove 1nm as we expand both sides of the shapes, so allowing for
    // a strictly greater than or equal comparison in the shape separation (boolean add)
    // means that we will end up with separate shapes that then are shrunk
    int             inflate = aMinThickness/2 - 1;

    BRDITEMS_PLOTTER itemplotter( aPlotter, aBoard, aPlotOpt );
    itemplotter.SetLayerSet( aLayerMask );

    // Plot edge layer and graphic items.
    // They do not have a solder Mask margin, because they are graphic items
    // on this layer (like logos), not actually areas around pads.

    // Normal mode to generate polygons from shapes with arcs, if any:
    DisableArcRadiusCorrection( false );

    itemplotter.PlotBoardGraphicItems();

    for( auto module : aBoard->Modules() )
    {
        for( auto item : module->GraphicalItems() )
        {
            itemplotter.PlotFootprintTextItems( module );

            if( item->Type() == PCB_MODULE_EDGE_T && item->GetLayer() == layer )
                itemplotter.PlotFootprintGraphicItem((EDGE_MODULE*) item );
        }
    }

    // Build polygons for each pad shape.  The size of the shape on solder mask should be size
    // of pad + clearance around the pad, where clearance = solder mask clearance + extra margin.
    // Extra margin is half the min width for solder mask, which is used to merge too-close shapes
    // (distance < aMinThickness), and will be removed when creating the actual shapes.

    // Will contain shapes inflated by inflate value that will be merged and deflated by
    // inflate value to build final polygons
    // After calculations the remaining polygons are polygons to plot
    SHAPE_POLY_SET areas;
    // Will contain exact shapes of all items on solder mask
    SHAPE_POLY_SET initialPolys;

#if NEW_ALGO
    // Generate polygons with arcs inside the shape or exact shape
    // to minimize shape changes created by arc to segment size correction.
    DisableArcRadiusCorrection( true );
#endif

    // Plot pads
    for( auto module : aBoard->Modules() )
    {
        // add shapes with their exact mask layer size in initialPolys
        module->TransformPadsShapesWithClearanceToPolygon( layer, initialPolys, 0 );
        // add shapes inflated by aMinThickness/2 in areas
        module->TransformPadsShapesWithClearanceToPolygon( layer, areas, inflate );
    }

    // Plot vias on solder masks, if aPlotOpt.GetPlotViaOnMaskLayer() is true,
    if( aPlotOpt.GetPlotViaOnMaskLayer() )
    {
        // The current layer is a solder mask, use the global mask clearance for vias
        int via_clearance = aBoard->GetDesignSettings().m_SolderMaskMargin;
        int via_margin = via_clearance + inflate;

        for( auto track : aBoard->Tracks() )
        {
            const VIA* via = dyn_cast<const VIA*>( track );

            if( !via )
                continue;

            // vias are plotted only if they are on the corresponding external copper layer
            LSET via_set = via->GetLayerSet();

            if( via_set[B_Cu] )
                via_set.set( B_Mask );

            if( via_set[F_Cu] )
                via_set.set( F_Mask );

            if( !( via_set & aLayerMask ).any() )
                continue;

            // add shapes with their exact mask layer size in initialPolys
            via->TransformShapeWithClearanceToPolygon( initialPolys, via_clearance );
            // add shapes inflated by aMinThickness/2 in areas
            via->TransformShapeWithClearanceToPolygon( areas, via_margin );
        }
    }

    // Add filled zone areas.
#if 0   // Set to 1 if a solder mask margin must be applied to zones on solder mask
    int zone_margin = aBoard->GetDesignSettings().m_SolderMaskMargin;
#else
    int zone_margin = 0;
#endif

    for( ZONE_CONTAINER* zone : aBoard->Zones() )
    {
        if( zone->GetLayer() != layer )
            continue;

        // Some intersecting zones, despite being on the same layer, cannot be
        // merged due to other parameters such as fillet radius. The filled areas will end up
        // effectively merged though, so we want to keep the corners of such intersections sharp.
        std::set<VECTOR2I> colinearCorners;
        zone->GetColinearCorners( aBoard, colinearCorners );

        // add shapes inflated by aMinThickness/2 in areas
        zone->TransformOutlinesShapeWithClearanceToPolygon( areas, inflate + zone_margin,
                                                            &colinearCorners );
        // add shapes with their exact mask layer size in initialPolys
        zone->TransformOutlinesShapeWithClearanceToPolygon( initialPolys, zone_margin,
                                                            &colinearCorners );
    }

    int maxError = aBoard->GetDesignSettings().m_MaxError;
    int numSegs = std::max( GetArcToSegmentCount( inflate, maxError, 360.0 ), 12 );

    // Merge all polygons: After deflating, not merged (not overlapping) polygons
    // will have the initial shape (with perhaps small changes due to deflating transform)
    areas.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );
    areas.Deflate( inflate, numSegs );

    // Restore initial settings:
    aBoard->GetDesignSettings().m_MaxError = currMaxError;

    // Restore normal option to build polygons from item shapes:
    DisableArcRadiusCorrection( false );

#if !NEW_ALGO
    // To avoid a lot of code, use a ZONE_CONTAINER to handle and plot polygons, because our
    // polygons look exactly like filled areas in zones.
    // Note, also this code is not optimized: it creates a lot of copy/duplicate data.
    // However it is not complex, and fast enough for plot purposes (copy/convert data is only a
    // very small calculation time for these calculations).
    ZONE_CONTAINER zone( aBoard );
    zone.SetMinThickness( 0 );      // trace polygons only
    zone.SetLayer( layer );
    // Combine the current areas to initial areas. This is mandatory because inflate/deflate
    // transform is not perfect, and we want the initial areas perfectly kept
    areas.BooleanAdd( initialPolys, SHAPE_POLY_SET::PM_FAST );
    areas.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    itemplotter.PlotFilledAreas( &zone, areas );
#else

    // Remove initial shapes: each shape will be added later, as flashed item or region
    // with a suitable attribute.
    // Do not merge pads is mandatory in Gerber files: They must be indentified as pads

    // we deflate areas in polygons, to avoid after subtracting initial shapes
    // having small artifacts due to approximations during polygon transforms
    areas.BooleanSubtract( initialPolys, SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    // Slightly inflate polygons to avoid any gap between them and other shapes,
    // These gaps are created by arc to segments approximations
    areas.Inflate( Millimeter2iu( 0.002 ),6 );

    // Now, only polygons with a too small thickness are stored in areas.
    areas.Fracture( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

    // Plot each initial shape (pads and polygons on mask layer), with suitable attributes:
    PlotStandardLayer( aBoard, aPlotter, aLayerMask, aPlotOpt );

    // Add shapes corresponding to areas having too small thickness.
    std::vector<wxPoint> cornerList;

    for( int ii = 0; ii < areas.OutlineCount(); ii++ )
    {
        cornerList.clear();
        const SHAPE_LINE_CHAIN& path = areas.COutline( ii );

        // polygon area in mm^2 :
        double curr_area = path.Area() / ( IU_PER_MM * IU_PER_MM );

        // Skip very small polygons: they are certainly artifacts created by
        // arc approximations and polygon transforms
        // (inflate/deflate transforms)
        constexpr double poly_min_area_mm2 = 0.01;     // 0.01 mm^2 gives a good filtering

        if( curr_area < poly_min_area_mm2 )
            continue;

        for( int jj = 0; jj < path.PointCount(); jj++ )
            cornerList.emplace_back( (wxPoint) path.CPoint( jj ) );

        // Ensure the polygon is closed
        if( cornerList[0] != cornerList[cornerList.size() - 1] )
            cornerList.push_back( cornerList[0] );

        aPlotter->PlotPoly( cornerList, FILLED_SHAPE );
    }
#endif
}


/**
 * Set up most plot options for plotting a board (especially the viewport)
 * Important thing:
 *      page size is the 'drawing' page size,
 *      paper size is the physical page size
 */
static void initializePlotter( PLOTTER *aPlotter, BOARD * aBoard,
                               PCB_PLOT_PARAMS *aPlotOpts )
{
    PAGE_INFO pageA4( wxT( "A4" ) );
    const PAGE_INFO& pageInfo = aBoard->GetPageSettings();
    const PAGE_INFO* sheet_info;
    double paperscale; // Page-to-paper ratio
    wxSize paperSizeIU;
    wxSize pageSizeIU( pageInfo.GetSizeIU() );
    bool autocenter = false;

    // Special options: to fit the sheet to an A4 sheet replace the paper size. However there
    // is a difference between the autoscale and the a4paper option:
    //  - Autoscale fits the board to the paper size
    //  - A4paper fits the original paper size to an A4 sheet
    //  - Both of them fit the board to an A4 sheet
    if( aPlotOpts->GetA4Output() )
    {
        sheet_info  = &pageA4;
        paperSizeIU = pageA4.GetSizeIU();
        paperscale  = (double) paperSizeIU.x / pageSizeIU.x;
        autocenter  = true;
    }
    else
    {
        sheet_info  = &pageInfo;
        paperSizeIU = pageSizeIU;
        paperscale  = 1;

        // Need autocentering only if scale is not 1:1
        autocenter  = (aPlotOpts->GetScale() != 1.0);
    }

    EDA_RECT bbox = aBoard->ComputeBoundingBox();
    wxPoint boardCenter = bbox.Centre();
    wxSize boardSize = bbox.GetSize();

    double compound_scale;

    // Fit to 80% of the page if asked; it could be that the board is empty, in this case
    // regress to 1:1 scale
    if( aPlotOpts->GetAutoScale() && boardSize.x > 0 && boardSize.y > 0 )
    {
        double xscale = (paperSizeIU.x * 0.8) / boardSize.x;
        double yscale = (paperSizeIU.y * 0.8) / boardSize.y;

        compound_scale = std::min( xscale, yscale ) * paperscale;
    }
    else
        compound_scale = aPlotOpts->GetScale() * paperscale;


    // For the plot offset we have to keep in mind the auxiliary origin too: if autoscaling is
    // off we check that plot option (i.e. autoscaling overrides auxiliary origin)
    wxPoint offset( 0, 0);

    if( autocenter )
    {
        offset.x = KiROUND( boardCenter.x - ( paperSizeIU.x / 2.0 ) / compound_scale );
        offset.y = KiROUND( boardCenter.y - ( paperSizeIU.y / 2.0 ) / compound_scale );
    }
    else
    {
        if( aPlotOpts->GetUseAuxOrigin() )
            offset = aBoard->GetAuxOrigin();
    }

    aPlotter->SetPageSettings( *sheet_info );

    aPlotter->SetViewport( offset, IU_PER_MILS/10, compound_scale, aPlotOpts->GetMirror() );
    // Has meaning only for gerber plotter. Must be called only after SetViewport
    aPlotter->SetGerberCoordinatesFormat( aPlotOpts->GetGerberPrecision() );
    // Has meaning only for SVG plotter. Must be called only after SetViewport
    aPlotter->SetSvgCoordinatesFormat( aPlotOpts->GetSvgPrecision(), aPlotOpts->GetSvgUseInch() );

    aPlotter->SetCreator( wxT( "PCBNEW" ) );
    aPlotter->SetColorMode( false );        // default is plot in Black and White.
    aPlotter->SetTextMode( aPlotOpts->GetTextMode() );
}


/**
 * Prefill in black an area a little bigger than the board to prepare for the negative plot
 */
static void FillNegativeKnockout( PLOTTER *aPlotter, const EDA_RECT &aBbbox )
{
    const int margin = 5 * IU_PER_MM;   // Add a 5 mm margin around the board
    aPlotter->SetNegative( true );
    aPlotter->SetColor( WHITE );        // Which will be plotted as black
    EDA_RECT area = aBbbox;
    area.Inflate( margin );
    aPlotter->Rect( area.GetOrigin(), area.GetEnd(), FILLED_SHAPE );
    aPlotter->SetColor( BLACK );
}


/**
 * Calculate the effective size of HPGL pens and set them in the plotter object
 */
static void ConfigureHPGLPenSizes( HPGL_PLOTTER *aPlotter, PCB_PLOT_PARAMS *aPlotOpts )
{
    // Compute penDiam (the value is given in mils) in pcb units, with plot scale (if Scale is 2,
    // penDiam value is always m_HPGLPenDiam so apparent penDiam is actually penDiam / Scale
    int penDiam = KiROUND( aPlotOpts->GetHPGLPenDiameter() * IU_PER_MILS / aPlotOpts->GetScale() );

    // Set HPGL-specific options and start
    aPlotter->SetPenSpeed( aPlotOpts->GetHPGLPenSpeed() );
    aPlotter->SetPenNumber( aPlotOpts->GetHPGLPenNum() );
    aPlotter->SetPenDiameter( penDiam );
}


/**
 * Open a new plotfile using the options (and especially the format) specified in the options
 * and prepare the page for plotting.
 * Return the plotter object if OK, NULL if the file is not created (or has a problem)
 */
PLOTTER* StartPlotBoard( BOARD *aBoard, PCB_PLOT_PARAMS *aPlotOpts, int aLayer,
                         const wxString& aFullFileName, const wxString& aSheetDesc )
{
    // Create the plotter driver and set the few plotter specific options
    PLOTTER*    plotter = NULL;

    switch( aPlotOpts->GetFormat() )
    {
    case PLOT_FORMAT::DXF:
        DXF_PLOTTER* DXF_plotter;
        DXF_plotter = new DXF_PLOTTER();
        DXF_plotter->SetUnits(
                static_cast<DXF_PLOTTER::DXF_UNITS>( aPlotOpts->GetDXFPlotUnits() ) );

        plotter = DXF_plotter;
        break;

    case PLOT_FORMAT::POST:
        PS_PLOTTER* PS_plotter;
        PS_plotter = new PS_PLOTTER();
        PS_plotter->SetScaleAdjust( aPlotOpts->GetFineScaleAdjustX(),
                                    aPlotOpts->GetFineScaleAdjustY() );
        plotter = PS_plotter;
        break;

    case PLOT_FORMAT::PDF:
        plotter = new PDF_PLOTTER();
        break;

    case PLOT_FORMAT::HPGL:
        HPGL_PLOTTER* HPGL_plotter;
        HPGL_plotter = new HPGL_PLOTTER();

        // HPGL options are a little more convoluted to compute, so they get their own function
        ConfigureHPGLPenSizes( HPGL_plotter, aPlotOpts );
        plotter = HPGL_plotter;
        break;

    case PLOT_FORMAT::GERBER:
        plotter = new GERBER_PLOTTER();
        break;

    case PLOT_FORMAT::SVG:
        plotter = new SVG_PLOTTER();
        break;

    default:
        wxASSERT( false );
        return NULL;
    }

    KIGFX::PCB_RENDER_SETTINGS* renderSettings = new KIGFX::PCB_RENDER_SETTINGS();
    renderSettings->LoadColors( aPlotOpts->ColorSettings() );
    plotter->SetRenderSettings( renderSettings );

    // Compute the viewport and set the other options

    // page layout is not mirrored, so temporarily change mirror option for the page layout
    PCB_PLOT_PARAMS plotOpts = *aPlotOpts;

    if( plotOpts.GetPlotFrameRef() && plotOpts.GetMirror() )
        plotOpts.SetMirror( false );

    initializePlotter( plotter, aBoard, &plotOpts );

    if( plotter->OpenFile( aFullFileName ) )
    {
        plotter->ClearHeaderLinesList();

        // For the Gerber "file function" attribute, set the layer number
        if( plotter->GetPlotterType() == PLOT_FORMAT::GERBER )
        {
            bool useX2mode = plotOpts.GetUseGerberX2format();

            GERBER_PLOTTER* gbrplotter = static_cast <GERBER_PLOTTER*> ( plotter );
            gbrplotter->UseX2format( useX2mode );
            gbrplotter->UseX2NetAttributes( plotOpts.GetIncludeGerberNetlistInfo() );

            // Attributes can be added using X2 format or as comment (X1 format)
            AddGerberX2Attribute( plotter, aBoard, aLayer, not useX2mode );
        }

        plotter->StartPlot();

        // Plot the frame reference if requested
        if( aPlotOpts->GetPlotFrameRef() )
        {
            PlotWorkSheet( plotter, aBoard->GetProject(), aBoard->GetTitleBlock(),
                           aBoard->GetPageSettings(), 1, 1, aSheetDesc, aBoard->GetFileName() );

            if( aPlotOpts->GetMirror() )
                initializePlotter( plotter, aBoard, aPlotOpts );
        }

        // When plotting a negative board: draw a black rectangle (background for plot board
        // in white) and switch the current color to WHITE; note the color inversion is actually
        // done in the driver (if supported)
        if( aPlotOpts->GetNegative() )
        {
            EDA_RECT bbox = aBoard->ComputeBoundingBox();
            FillNegativeKnockout( plotter, bbox );
        }

        return plotter;
    }

    delete plotter->RenderSettings();
    delete plotter;
    return NULL;
}
