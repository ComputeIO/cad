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

#include <map>

#include <pin_type.h>
#include <sch_pin.h>
#include <sch_shape.h>
#include <sch_text.h>

// OpenOrcadParser Headers
#include "Enums/LineStyle.hpp"
#include "Enums/LineWidth.hpp"
#include "PinShape.hpp"
#include "Primitives/PrimArc.hpp"
#include "Primitives/PrimBezier.hpp"
#include "Primitives/PrimCommentText.hpp"
#include "Primitives/PrimEllipse.hpp"
#include "Primitives/PrimLine.hpp"
#include "Primitives/PrimPolygon.hpp"
#include "Primitives/PrimPolyline.hpp"
#include "Primitives/PrimRect.hpp"
#include "Streams/StreamPackage.hpp"
#include "Structures/StructSymbolPin.hpp"


SCH_SHAPE* convPrimArc2ARC(const PrimArc* aObj);
std::vector<SCH_SHAPE*> convPrimBezier2BEZIER(const PrimBezier* aOrcadObj);
SCH_TEXT* convPrimCommentText2SCH_TEXT(const PrimCommentText* aOrcadObj);
SCH_SHAPE* convPrimEllipse2POLY(const PrimEllipse* aOrcadObj);
SCH_SHAPE* convPrimLine2POLY(const PrimLine* aOrcadObj);
SCH_SHAPE* convPrimPolyline2POLY(const PrimPolyline* aOrcadObj);
SCH_SHAPE* convPrimPolygon2POLY(const PrimPolygon* aOrcadObj);
SCH_SHAPE* convPrimRect2RECTANGLE(const PrimRect* aOrcadObj);
int32_t convDimIU(int32_t aOrcadIU);
PIN_ORIENTATION getPinOrientation(int32_t aStartX, int32_t aStartY, int32_t aHotptX, int32_t aHotptY);
bool isCircle(const VECTOR2I& aP1, const VECTOR2I& aP2);
SCH_SHAPE*      createEllipse( const VECTOR2I& aP1, const VECTOR2I& aP2,
                               FILL_T aFill = FILL_T::NO_FILL );
SCH_SHAPE*      createEllipse( const VECTOR2I& aP1, const VECTOR2I& aP2, const VECTOR2I& aStart,
                               const VECTOR2I& aEnd );
SCH_PIN* StreamPackage2SCH_PIN( const StreamPackage* aOrcadStream, LIB_SYMBOL* aSymbol, uint32_t aIdx );


const std::map<ShapeType, GRAPHIC_PINSHAPE> OrCad2Kicad_PinShape = {
    {ShapeType::Clock,         GRAPHIC_PINSHAPE::CLOCK},
    {ShapeType::Dot,           GRAPHIC_PINSHAPE::INVERTED},
    {ShapeType::DotClock,      GRAPHIC_PINSHAPE::INVERTED_CLOCK},
    {ShapeType::Line,          GRAPHIC_PINSHAPE::LINE},
    {ShapeType::Short,         GRAPHIC_PINSHAPE::LINE},
    {ShapeType::ShortClock,    GRAPHIC_PINSHAPE::CLOCK},
    {ShapeType::ShortDot,      GRAPHIC_PINSHAPE::INVERTED},
    {ShapeType::ShortDotClock, GRAPHIC_PINSHAPE::INVERTED_CLOCK},
    {ShapeType::ZeroLength,    GRAPHIC_PINSHAPE::LINE}
};


const std::map<PortType, ELECTRICAL_PINTYPE> OrCad2Kicad_PinType = {
    {PortType::Input,         ELECTRICAL_PINTYPE::PT_INPUT},
    {PortType::Bidirectional, ELECTRICAL_PINTYPE::PT_BIDI},
    {PortType::Output,        ELECTRICAL_PINTYPE::PT_OUTPUT},
    {PortType::OpenCollector, ELECTRICAL_PINTYPE::PT_OPENCOLLECTOR},
    {PortType::Passive,       ELECTRICAL_PINTYPE::PT_PASSIVE},
    {PortType::ThreeState,    ELECTRICAL_PINTYPE::PT_TRISTATE},
    {PortType::OpenEmitter,   ELECTRICAL_PINTYPE::PT_OPENEMITTER},
    {PortType::Power,         ELECTRICAL_PINTYPE::PT_UNSPECIFIED} // We can't know whether to use PT_POWER_IN or PT_POWER_OUT
};


const std::map<LineWidth, int> LineWidth2Width = {
    {LineWidth::Thin,     3 * 254},
    {LineWidth::Medium,   9 * 254},
    {LineWidth::Wide,    12 * 254},
    {LineWidth::Default,  0} // Defaults to 6 * 254
};


const std::map<LineStyle, LINE_STYLE> LineStyle2LINE_STYLE = {
    {LineStyle::Solid,      LINE_STYLE::SOLID},
    {LineStyle::Dash,       LINE_STYLE::DASH},
    {LineStyle::Dot,        LINE_STYLE::DOT},
    {LineStyle::DashDot,    LINE_STYLE::DASHDOT},
    {LineStyle::DashDotDot, LINE_STYLE::DASHDOTDOT},
    {LineStyle::Default,    LINE_STYLE::DEFAULT}
};
