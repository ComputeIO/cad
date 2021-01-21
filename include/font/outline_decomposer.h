/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * Outline font class
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

#ifndef OUTLINE_DECOMPOSER_H_
#define OUTLINE_DECOMPOSER_H_

#include <vector>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include <math/box2.h>
#include <math/vector2d.h>

typedef std::vector<VECTOR2D> POINTS;
typedef std::vector<POINTS>   POINTS_LIST;

typedef std::vector<std::vector<VECTOR2D>*> GLYPH;
typedef std::vector<GLYPH*>                 GLYPH_LIST;
typedef std::vector<BOX2D>                  GLYPH_BOUNDING_BOX_LIST;

typedef struct
{
    POINTS         points;
    int            winding;
    FT_Orientation orientation;
} CONTOUR;


typedef std::vector<CONTOUR> CONTOURS;


class OUTLINE_DECOMPOSER
{
public:
    OUTLINE_DECOMPOSER( FT_Outline& aOutline );

    void OutlineToSegments( CONTOURS* aContours );

private:
    FT_Outline& m_outline;
    CONTOURS*   m_contours;

    void contourToSegmentsAndArcs( CONTOUR& aResult, unsigned int aContourIndex ) const;

    void newContour();

    void addContourPoint( const VECTOR2D& p );

    int approximateContour( const POINTS& aPoints, const std::vector<bool>& aPointOnCurve,
                            POINTS& aResult ) const;

    bool approximateBezierCurve( POINTS& result, const POINTS& bezier ) const;
    bool approximateQuadraticBezierCurve( POINTS& result, const POINTS& bezier ) const;
    bool approximateCubicBezierCurve( POINTS& result, const POINTS& bezier ) const;

    /**
     * @return 1 if aContour is in clockwise order, -1 if it is in
     *     counterclockwise order, or 0 if the winding can't be
     *     determined.
     */
    int winding( const POINTS& aContour ) const;

    inline static const unsigned int onCurve( char aTags ) { return aTags & 0x1; }

    inline static const unsigned int thirdOrderBezierPoint( char aTags )
    {
        return onCurve( aTags ) ? 0 : aTags & 0x2;
    }

    inline static const unsigned int secondOrderBezierPoint( char aTags )
    {
        return onCurve( aTags ) ? 0 : !thirdOrderBezierPoint( aTags );
    }

    inline static const unsigned int hasDropout( char aTags ) { return aTags & 0x4; }

    inline static const unsigned int dropoutMode( char aTags )
    {
        return hasDropout( aTags ) ? ( aTags & 0x38 ) : 0;
    }

    VECTOR2D m_lastEndPoint;

    // FT_Outline_Decompose callbacks
    static int moveTo( const FT_Vector* aEndPoint, void* aCallbackData );
    static int lineTo( const FT_Vector* aEndPoint, void* aCallbackData );
    static int quadraticTo( const FT_Vector* aControlPoint, const FT_Vector* aEndPoint,
                            void* aCallbackData );
    static int cubicTo( const FT_Vector* aFirstControlPoint, const FT_Vector* aSecondControlPoint,
                        const FT_Vector* aEndPoint, void* aCallbackData );
};

#endif // OUTLINE_DECOMPOSER_H_
