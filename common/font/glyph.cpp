/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>
#include <font/glyph.h>

using namespace KIFONT;


STROKE_GLYPH::STROKE_GLYPH( const STROKE_GLYPH& aGlyph )
{
    for( auto pointList : aGlyph.m_pointLists )
    {
        m_pointLists.push_back( pointList );
    }
}


void STROKE_GLYPH::AddPoint( const VECTOR2D& aPoint )
{
    if( !m_penIsDown )
    {
        std::vector<VECTOR2D> v;
        m_pointLists.push_back( v );
        m_penIsDown = true;
    }

    m_pointLists.back().push_back( aPoint );
}


void STROKE_GLYPH::RaisePen()
{
    if( m_penIsDown )
        m_pointLists.back().shrink_to_fit();
    m_penIsDown = false;
}


void STROKE_GLYPH::Finalize()
{
    if( !m_pointLists.empty() && !m_pointLists.back().empty() )
    {
        m_pointLists.back().shrink_to_fit();
    }
}


BOX2D STROKE_GLYPH::BoundingBox() const
{
    VECTOR2D min;
    VECTOR2D max;

    bool firstPoint = true;
    for( auto pointList : m_pointLists )
    {
        for( const VECTOR2D& point : pointList )
        {
            if( firstPoint )
            {
                min = point;
                max = point;
                firstPoint = false;
            }
            min.x = std::min( min.x, point.x );
            min.y = std::min( min.y, point.y );
            max.x = std::max( max.x, point.x );
            max.y = std::max( max.y, point.y );
        }
    }

    return BOX2D( min, max - min );
}


std::shared_ptr<GLYPH> STROKE_GLYPH::Resize( const VECTOR2D& aGlyphSize ) const
{
    auto glyph = std::make_shared<STROKE_GLYPH>( *this );
    for( std::vector<VECTOR2D>& pointList : glyph->m_pointLists )
    {
        for( VECTOR2D& point : pointList )
        {
#ifdef DEBUG
            std::cerr << point << "->";
#endif
            point.x = point.x * aGlyphSize.x;
            point.y = point.y * aGlyphSize.y;
#ifdef DEBUG
            std::cerr << point << ",";
#endif
        }
    }

    return glyph;
}


std::shared_ptr<GLYPH> STROKE_GLYPH::Translate( const VECTOR2D& aOffset ) const
{
    auto glyph = std::make_shared<STROKE_GLYPH>( *this );
    for( std::vector<VECTOR2D>& pointList : glyph->m_pointLists )
    {
        for( VECTOR2D& point : pointList )
        {
#ifdef DEBUG
            std::cerr << point << "=>";
#endif
            point.x += aOffset.x;
            point.y += aOffset.y;
#ifdef DEBUG
            std::cerr << point << ";";
#endif
        }
    }

    return glyph;
}
