/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 Dominik Wernberger <dominik.wernberger@gmx.de>
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <cmath>
#include <vector>
#include <deque>

#include "orcad2kicad.h"

#include <core/typeinfo.h>
#include <sch_shape.h>
#include <sch_text.h>


// OpenOrcadParser Headers
#include "Container.hpp"
#include "GetStreamHelper.hpp"
#include "PinShape.hpp"
#include "Primitives/PrimArc.hpp"
#include "Primitives/PrimBezier.hpp"
#include "Primitives/PrimLine.hpp"
#include "Primitives/PrimPolygon.hpp"
#include "Primitives/PrimPolyline.hpp"
#include "Primitives/PrimRect.hpp"
#include "Streams/StreamPackage.hpp"


SCH_SHAPE* convPrimArc2ARC(const PrimArc* aOrcadObj)
{
    // Kicad does not support elliptical arcs, we approximate
    // it by straight lines, however arcs are actually not yet
    // supported!

    const auto p1    = VECTOR2I( convDimIU( aOrcadObj->x1 ), convDimIU( aOrcadObj->y1 ) );
    const auto p2    = VECTOR2I( convDimIU( aOrcadObj->x2 ), convDimIU( aOrcadObj->y2 ) );
    const auto start = VECTOR2I( convDimIU( aOrcadObj->startX ), convDimIU( aOrcadObj->startY ) );
    const auto end   = VECTOR2I( convDimIU( aOrcadObj->endX ), convDimIU( aOrcadObj->endY ) );

    // if( isCircle( p1, p2 ) )
    // {
    //     std::unique_ptr<SCH_SHAPE> kicadObj = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC, LAYER_DEVICE );

    //     // Circle radius
    //     double r = std::abs( p1.x - p2.x ) / 2.0;

    //     // Point between end and start point
    //     VECTOR2D m1 = start + (end - start) / 2.0;

    //     // Center Point
    //     VECTOR2D c = VECTOR2D( p1.x + r, p1.y + r );

    //     // Vector from center to m1
    //     VECTOR2D m2 = m1 - c;

    //     double len_m2 = m2.EuclideanNorm();
    //     VECTOR2D mid_d = m2 / len_m2 * r;

    //     VECTOR2I mid = VECTOR2I( static_cast<int32_t>( std::round( mid_d.x ) ), static_cast<int32_t>( std::round( mid_d.y ) ));

    //     kicadObj->SetArcGeometry( end, mid, start );

    //     ksymbol->AddDrawItem( kicadObj.release() );
    // }

    auto segment = createEllipsoide( p1, p2, start, end );

    auto stroke = STROKE_PARAMS{};
    stroke.SetLineStyle( LineStyle2LINE_STYLE.at( aOrcadObj->getLineStyle() ) );
    stroke.SetWidth( LineWidth2Width.at( aOrcadObj->getLineWidth() ) );
    // stroke.SetColor( KIGFX::COLOR4D{0.0, 0.0, 0.0, 1.0} );

    // for( auto& segment : segments )
    {
        segment->SetStroke( stroke );
    }

    return segment;
}


std::vector<SCH_SHAPE*> convPrimBezier2BEZIER(const PrimBezier* aOrcadObj)
{
    const int segments = static_cast<int>( ( aOrcadObj->points.size() - 1 ) / 3 );

    auto stroke = STROKE_PARAMS{};
    stroke.SetLineStyle( LineStyle2LINE_STYLE.at( aOrcadObj->getLineStyle() ) );
    stroke.SetWidth( LineWidth2Width.at( aOrcadObj->getLineWidth() ) );
    // stroke.SetColor( KIGFX::COLOR4D{0.0, 0.0, 0.0, 1.0} );

    std::vector<SCH_SHAPE*> segmentsVec;

    for( int i = 0; i < segments; ++i )
    {
        const auto start = VECTOR2I( convDimIU( aOrcadObj->points.at(i + 0).x ), convDimIU( aOrcadObj->points.at(i + 0).y ) );
        const auto c1    = VECTOR2I( convDimIU( aOrcadObj->points.at(i + 1).x ), convDimIU( aOrcadObj->points.at(i + 1).y ) );
        const auto c2    = VECTOR2I( convDimIU( aOrcadObj->points.at(i + 2).x ), convDimIU( aOrcadObj->points.at(i + 2).y ) );
        const auto end   = VECTOR2I( convDimIU( aOrcadObj->points.at(i + 3).x ), convDimIU( aOrcadObj->points.at(i + 3).y ) );

        SCH_SHAPE* kicadObj = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_DEVICE );

        kicadObj->SetStroke( stroke );

        kicadObj->SetStart( start );
        kicadObj->SetBezierC1( c1 );
        kicadObj->SetBezierC2( c2 );
        kicadObj->SetEnd( end );

        segmentsVec.push_back( kicadObj );
    }

    return segmentsVec;
}


SCH_TEXT* convPrimCommentText2SCH_TEXT(const PrimCommentText* aOrcadObj)
{
    int32_t width  = aOrcadObj->x2 - aOrcadObj->x1;
    int32_t height = aOrcadObj->y2 - aOrcadObj->y1;

    VECTOR2I pos = VECTOR2I( convDimIU( aOrcadObj->locX + width / 2 ), convDimIU( aOrcadObj->locY + height / 2 ) );

    wxString text = {aOrcadObj->name.c_str(), wxConvUTF8};

    SCH_TEXT* kicadObj = new SCH_TEXT( pos, text, LAYER_NOTES, SCH_TEXT_T );

    return kicadObj;
}


SCH_SHAPE* convPrimEllipse2POLY(const PrimEllipse* aOrcadObj)
{
    FILL_T fill;

    switch( aOrcadObj->getFillStyle() )
    {
        case FillStyle::Solid:
            fill = FILL_T::FILLED_SHAPE;
            break;
        case FillStyle::None:
            fill = FILL_T::NO_FILL;
            break;
        default: // Hatching is not supported in Kicad
            fill = FILL_T::NO_FILL;
            break;
    }

    const auto p1 = VECTOR2I( convDimIU( aOrcadObj->x1 ), convDimIU( aOrcadObj->y1 ) );
    const auto p2 = VECTOR2I( convDimIU( aOrcadObj->x2 ), convDimIU( aOrcadObj->y2 ) );

    auto kicadObj = createEllipsoide( p1, p2, fill );

    auto stroke = STROKE_PARAMS{};
    stroke.SetLineStyle( LineStyle2LINE_STYLE.at( aOrcadObj->getLineStyle() ) );
    stroke.SetWidth( LineWidth2Width.at( aOrcadObj->getLineWidth() ) );
    // stroke.SetColor( KIGFX::COLOR4D{0.0, 0.0, 0.0, 1.0} );
    kicadObj->SetStroke( stroke );

    return kicadObj;
}


SCH_SHAPE* convPrimLine2POLY(const PrimLine* aOrcadObj)
{
    SCH_SHAPE* kicadObj = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

    auto stroke = STROKE_PARAMS{};
    stroke.SetLineStyle( LineStyle2LINE_STYLE.at( aOrcadObj->getLineStyle() ) );
    stroke.SetWidth( LineWidth2Width.at( aOrcadObj->getLineWidth() ) );
    // stroke.SetColor( KIGFX::COLOR4D{0.0, 0.0, 0.0, 1.0} );
    kicadObj->SetStroke( stroke );

    kicadObj->AddPoint( VECTOR2I( convDimIU( aOrcadObj->x1 ), convDimIU( aOrcadObj->y1 ) ) );
    kicadObj->AddPoint( VECTOR2I( convDimIU( aOrcadObj->x2 ), convDimIU( aOrcadObj->y2 ) ) );

    return kicadObj;
}


SCH_SHAPE* convPrimPolygon2POLY(const PrimPolygon* aOrcadObj)
{
    FILL_T fill;

    switch( aOrcadObj->fillStyle )
    {
        case FillStyle::Solid:
            fill = FILL_T::FILLED_SHAPE;
            break;
        case FillStyle::None:
            fill = FILL_T::NO_FILL;
            break;
        default: // Hatching is not supported in Kicad
            fill = FILL_T::NO_FILL;
            break;
    }

    SCH_SHAPE* kicadObj = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE, 0, fill );

    auto stroke = STROKE_PARAMS{};
    stroke.SetLineStyle( LineStyle2LINE_STYLE.at( aOrcadObj->getLineStyle() ) );
    stroke.SetWidth( LineWidth2Width.at( aOrcadObj->getLineWidth() ) );
    // stroke.SetColor( KIGFX::COLOR4D{0.0, 0.0, 0.0, 1.0} );
    kicadObj->SetStroke( stroke );

    for( const auto& point : aOrcadObj->points )
    {
        kicadObj->AddPoint( VECTOR2I( convDimIU( point.x ), convDimIU( point.y ) ) );
    }

    return kicadObj;
}


SCH_SHAPE* convPrimPolyline2POLY(const PrimPolyline* aOrcadObj)
{
    SCH_SHAPE* kicadObj = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

    auto stroke = STROKE_PARAMS{};
    stroke.SetLineStyle( LineStyle2LINE_STYLE.at( aOrcadObj->getLineStyle() ) );
    stroke.SetWidth( LineWidth2Width.at( aOrcadObj->getLineWidth() ) );
    // stroke.SetColor( KIGFX::COLOR4D{0.0, 0.0, 0.0, 1.0} );
    kicadObj->SetStroke( stroke );

    for( const auto& point : aOrcadObj->points )
    {
        kicadObj->AddPoint( VECTOR2I( convDimIU( point.x ), convDimIU( point.y ) ) );
    }

    return kicadObj;
}


SCH_SHAPE* convPrimRect2RECTANGLE(const PrimRect* aOrcadObj)
{
    SCH_SHAPE* kicadObj = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_DEVICE );

    auto stroke = STROKE_PARAMS{};
    stroke.SetLineStyle( LineStyle2LINE_STYLE.at( aOrcadObj->getLineStyle() ) );
    stroke.SetWidth( LineWidth2Width.at( aOrcadObj->getLineWidth() ) );
    // stroke.SetColor( KIGFX::COLOR4D{0.0, 0.0, 0.0, 1.0} );
    kicadObj->SetStroke( stroke );

    kicadObj->SetPosition( VECTOR2I( 0, 0 ) );
    kicadObj->SetStart( VECTOR2I( convDimIU( aOrcadObj->x1 ), convDimIU( aOrcadObj->y1 ) ) );
    kicadObj->SetEnd( VECTOR2I( convDimIU( aOrcadObj->x2 ), convDimIU( aOrcadObj->y2 ) ) );

    return kicadObj;
}


int32_t convDimIU(int32_t aOrcadIU)
{
    return aOrcadIU * 2540;
}


PIN_ORIENTATION getPinOrientation(const StructSymbolPin* aPin)
{
    if( aPin->startX == aPin->hotptX )
    {
        if( aPin->startY > aPin->hotptY)
        {
            return PIN_ORIENTATION::PIN_DOWN;
        }
        else
        {
            return PIN_ORIENTATION::PIN_UP;
        }
    }

    if( aPin->startY == aPin->hotptY )
    {
        if( aPin->startX > aPin->hotptX)
        {
            return PIN_ORIENTATION::PIN_RIGHT;
        }
        else
        {
            return PIN_ORIENTATION::PIN_LEFT;
        }
    }

    // Pin orientation is kind of diagonal,
    // return some dummy value.
    return PIN_ORIENTATION::PIN_LEFT;
}


// aP1, aP2 are the points spanning the bounding rectangle of the ellipsoid
// A circle is a special ellipsoid where the bounding rectangle is square
bool isCircle(const VECTOR2I& aP1, const VECTOR2I& aP2)
{
    int32_t width  = std::abs( aP1.x - aP2.x );
    int32_t height = std::abs( aP1.y - aP2.y );

    if( width == height )
    {
        return true;
    }

    return false;
}


SCH_SHAPE* createEllipsoide(const VECTOR2I& aP1, const VECTOR2I& aP2, FILL_T aFill )
{
    auto rect = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE, LAYER_DEVICE );
    rect->SetPosition( aP1 );
    rect->SetStart( aP1 );
    rect->SetEnd( aP2 );

    // Draw bounding rectangle
    // return rect.release();

    const double r1 = rect->GetRectangleWidth()  / 2.0; //!< Radius 1
    const double r2 = rect->GetRectangleHeight() / 2.0; //!< Radius 2

    // Ellipsoid midpoint
    const VECTOR2D mid = VECTOR2D( aP1.x + r1, aP1.y + r2 );

    // Number of linear segments we approximate the ellipsoid with
    const int numberSegments = 100;

    SCH_SHAPE* s = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE, 0, aFill );

    for(int i = 0; i < numberSegments; ++i)
    {
        const double da = 2.0 * std::numbers::pi / (numberSegments - 1);
        const double x = mid.x + r1 * std::cos(i * da);
        const double y = mid.y + r2 * std::sin(i * da);
        const auto p = VECTOR2I( static_cast<int32_t>(std::round(x)), static_cast<int32_t>(std::round(y)) );

        s->AddPoint( p );
    }

    return s;
}


SCH_SHAPE* createEllipsoide(const VECTOR2I& aP1, const VECTOR2I& aP2, const VECTOR2I& aStart, const VECTOR2I& aEnd )
{
    auto rect = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE, LAYER_DEVICE );
    rect->SetPosition( aP1 );
    rect->SetStart( aP1 );
    rect->SetEnd( aP2 );

    const double r1 = rect->GetRectangleWidth()  / 2.0; //!< Radius 1
    const double r2 = rect->GetRectangleHeight() / 2.0; //!< Radius 2

    // Ellipsoid midpoint
    const VECTOR2D mid = VECTOR2D( aP1.x + r1, aP1.y + r2 );

    const VECTOR2D startVec = aStart - mid;
    const VECTOR2D endVec   = aEnd   - mid;

    const VECTOR2D normX = VECTOR2D( 1.0, 0.0 );

    const double phi_start = std::acos( (startVec.x * normX.x + startVec.y * normX.y ) / ( startVec.EuclideanNorm() * normX.EuclideanNorm() ) );
    const double phi_end   = std::acos( (endVec.x   * normX.x + endVec.y   * normX.y ) / ( endVec.EuclideanNorm()   * normX.EuclideanNorm() ) );

    double delta_phi = ( endVec.x * startVec.x + endVec.y * startVec.y ) / ( endVec.EuclideanNorm() * startVec.EuclideanNorm() );
    delta_phi = std::acos( delta_phi );

    // Number of linear segments we approximate the ellipsoid with
    const int numberSegments = 100;

    SCH_SHAPE* s = new SCH_SHAPE( SHAPE_T::POLY, LAYER_DEVICE );

    double phi = phi_end;
    // double delta_phi = (phi_end - phi_start) - 360;

    for(int i = 0; i < numberSegments; ++i)
    {
        const double da = delta_phi / (numberSegments - 1);
        const double x = mid.x + r1 * std::cos(i * da + phi);
        const double y = mid.y + r2 * std::sin(i * da + phi);
        const auto p = VECTOR2I( static_cast<int32_t>(std::round(x)), static_cast<int32_t>(std::round(y)) );

        s->AddPoint( p );
    }

    return s;
}


// OrCAD uses trailing backslashes after each character to
// indicate a overbar over that character.
// E.g. OrCAD `R\E\S\E\T\` becomes KiCAD `~{RESET}`
wxString convPinName( const std::string& aOrCadPinName )
{
    struct CharProp
    {
        CharProp() : str{}, hasOverbar{false} { }
        CharProp(const std::string& aStr, bool aHasOverbar) : str{aStr}, hasOverbar{aHasOverbar} { }

        std::string str;
        bool hasOverbar;
    };

    std::deque<CharProp> charProps;

    for( const auto& c : aOrCadPinName )
    {
        if( c == '\\' )
        {
            if( !charProps.empty() )
            {
                charProps.back().hasOverbar = true;
            }
        }
        else
        {
            charProps.push_back( {std::string{c}, false} );
        }
    }

    std::deque<CharProp> strProps;

    // Combine multiple successive short overbars to one large
    for( const auto& charProp : charProps )
    {
        if( strProps.empty() )
        {
            strProps.push_back( charProp );
            continue;
        }

        if( (charProp.hasOverbar && strProps.back().hasOverbar) ||
            (!charProp.hasOverbar && !strProps.back().hasOverbar ) )
        {
            strProps.back().str += charProp.str;
        }
        else
        {
            strProps.push_back( charProp );
        }
    }

    std::string kicadPinName{};

    for( const auto& strProp : strProps )
    {
        if( strProp.hasOverbar )
        {
            kicadPinName += std::string{"~{"} + strProp.str + std::string{"}"};
        }
        else
        {
            kicadPinName += strProp.str;
        }
    }

    return wxString{kicadPinName.c_str(), wxConvUTF8};
}


SCH_PIN* StreamPackage2SCH_PIN( const StreamPackage* aOrcadStream, LIB_SYMBOL* aSymbol, uint32_t aIdx )
{
    if( !aOrcadStream )
    {
        return nullptr;
    }

    if( aOrcadStream->libraryParts.empty() )
    {
        return nullptr;
    }

    StructSymbolPin* pkgPin = nullptr;

    if( aIdx < aOrcadStream->libraryParts.at( 0 )->symbolPins.size() )
    {
        pkgPin = aOrcadStream->libraryParts.at( 0 )->symbolPins.at( aIdx ).get();
    }

    if( !pkgPin )
    {
        return nullptr;
    }

    SCH_PIN* pin = new SCH_PIN( aSymbol );

    pin->SetName( convPinName( pkgPin->name ) );

    const GRAPHIC_PINSHAPE shape = OrCad2Kicad_PinShape.at( ToShapeType( pkgPin->pinShape ) );
    pin->SetShape( shape );

    wxString pinNumber = wxString{"", wxConvUTF8};

    do
    {
        if( !aOrcadStream->package )
        {
            break;
        }

        if( aOrcadStream->package->devices.empty() )
        {
            break;
        }

        if( !aOrcadStream->package->devices.at( 0 ) )
        {
            break;
        }

        if( aIdx < aOrcadStream->package->devices.at( 0 )->pinMap.size() )
        {
            pinNumber = wxString{aOrcadStream->package->devices.at( 0 )->pinMap.at( aIdx ).c_str(), wxConvUTF8};
        }
    }
    while( false );

    pin->SetNumber( pinNumber );

    const auto pinOrientation = getPinOrientation( pkgPin );
    pin->SetOrientation( pinOrientation );
    pin->SetType( OrCad2Kicad_PinType.at( pkgPin->portType ) );
    pin->SetLength( convDimIU( pkgPin->getPinLength() ) );
    pin->SetPosition( VECTOR2I( convDimIU ( pkgPin->hotptX ), convDimIU( pkgPin->hotptY ) ) );

    return pin;
}