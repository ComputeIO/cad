/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
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

#include <eda_text.h>
#include <vector>
#include <font/font.h>


KIFONT::FONT* TEXT_ATTRIBUTES::GetFont() const {
    if (m_font)
        return m_font;

    // default to newstroke
    return KIFONT::FONT::GetFont();
}


TEXT_ATTRIBUTES::ORIENTATION TEXT_ATTRIBUTES::ReadOrientation( const EDA_ANGLE& aAngle )
{
    int         angle = aAngle.AsDegrees();
    ORIENTATION orientation = TEXT_ATTRIBUTES::FREE_ANGLE;

    switch( angle )
    {
    case 0: orientation = TEXT_ATTRIBUTES::ANGLE_0; break;
    case 90: orientation = TEXT_ATTRIBUTES::ANGLE_90; break;
    case 180: orientation = TEXT_ATTRIBUTES::ANGLE_180; break;
    case 270: orientation = TEXT_ATTRIBUTES::ANGLE_270; break;
    default: break;
    }

    return orientation;
}


TEXT_ATTRIBUTES::TEXT_ATTRIBUTES( const EDA_TEXT& aText )
{
    EDA_ANGLE angle( aText.GetTextAngleRadians(), EDA_ANGLE::RADIANS );
    m_orientation = ReadOrientation( angle );
    if( m_orientation == TEXT_ATTRIBUTES::FREE_ANGLE )
    {
        m_angle = angle;
    }
    SetHorizJustify( aText.GetHorizJustify() );
    SetVertJustify( aText.GetVertJustify() );
    SetBold( aText.IsBold() );
    SetItalic( aText.IsItalic() );
    SetMirrored( aText.IsMirrored() );
    SetLineSpacing( aText.GetLineSpacing() );
}


TEXT_ATTRIBUTES::TEXT_ATTRIBUTES( const EDA_ANGLE& aAngle, EDA_TEXT_HJUSTIFY_T aHorizontalJustify,
                                  EDA_TEXT_VJUSTIFY_T aVerticalJustify )
{
    m_orientation = ReadOrientation( aAngle );
    if( m_orientation == TEXT_ATTRIBUTES::FREE_ANGLE )
    {
        m_angle = EDA_ANGLE( aAngle );
    }

    m_horizontal_alignment = HorizontalJustifyToAlignment( aHorizontalJustify );
    m_vertical_alignment = VerticalJustifyToAlignment( aVerticalJustify );
}


#ifdef EDA_TEXT_OVERHAUL
TEXT_ATTRIBUTES::TEXT_ATTRIBUTES( LABEL_SPIN_STYLE aSpin ) :
        m_vertical_alignment( TEXT_ATTRIBUTES::V_BOTTOM )
{
    switch( aSpin )
    {
    case LABEL_SPIN_STYLE::LEFT:
        SetOrientation( TEXT_ATTRIBUTES::ANGLE_0 );
        Align( TEXT_ATTRIBUTES::H_LEFT );
        break;
    case LABEL_SPIN_STYLE::UP:
        SetOrientation( TEXT_ATTRIBUTES::ANGLE_90 );
        Align( TEXT_ATTRIBUTES::H_LEFT );
        break;
    case LABEL_SPIN_STYLE::RIGHT:
        SetOrientation( TEXT_ATTRIBUTES::ANGLE_0 );
        Align( TEXT_ATTRIBUTES::H_RIGHT );
        break;
    case LABEL_SPIN_STYLE::BOTTOM:
        SetOrientation( TEXT_ATTRIBUTES::ANGLE_90 );
        Align( TEXT_ATTRIBUTES::H_RIGHT );
        break;
    default:
        // can't happen TODO assert
        break;
    }
}
#endif //EDA_TEXT_OVERHAUL

/**
 * Rotate counterclockwise.
 * @return the rotated attributes
 */
TEXT_ATTRIBUTES& TEXT_ATTRIBUTES::RotateCCW()
{
    switch( m_orientation )
    {
    case TEXT_ATTRIBUTES::ANGLE_0: SetOrientation( TEXT_ATTRIBUTES::ANGLE_90 ); break;
    case TEXT_ATTRIBUTES::ANGLE_90: SetOrientation( TEXT_ATTRIBUTES::ANGLE_180 ); break;
    case TEXT_ATTRIBUTES::ANGLE_180: SetOrientation( TEXT_ATTRIBUTES::ANGLE_270 ); break;
    case TEXT_ATTRIBUTES::ANGLE_270: SetOrientation( TEXT_ATTRIBUTES::ANGLE_0 ); break;
    case TEXT_ATTRIBUTES::FREE_ANGLE: SetAngle( GetAngle().RotateCCW() );
    }
    return *this;
}

/**
 * Rotate clockwise.
 * @return the rotated attributes
 */
TEXT_ATTRIBUTES& TEXT_ATTRIBUTES::RotateCW()
{
    switch( m_orientation )
    {
    case TEXT_ATTRIBUTES::ANGLE_0: SetOrientation( TEXT_ATTRIBUTES::ANGLE_270 ); break;
    case TEXT_ATTRIBUTES::ANGLE_90: SetOrientation( TEXT_ATTRIBUTES::ANGLE_0 ); break;
    case TEXT_ATTRIBUTES::ANGLE_180: SetOrientation( TEXT_ATTRIBUTES::ANGLE_90 ); break;
    case TEXT_ATTRIBUTES::ANGLE_270: SetOrientation( TEXT_ATTRIBUTES::ANGLE_180 ); break;
    case TEXT_ATTRIBUTES::FREE_ANGLE: SetAngle( GetAngle().RotateCW() );
    }
    return *this;
}


TEXT_ATTRIBUTES& TEXT_ATTRIBUTES::SpinCCW()
{
    switch( m_orientation )
    {
    case TEXT_ATTRIBUTES::ANGLE_0: SetOrientation( TEXT_ATTRIBUTES::ANGLE_90 ); break;
    case TEXT_ATTRIBUTES::ANGLE_90:
        SetOrientation( TEXT_ATTRIBUTES::ANGLE_0 );
        Align( OppositeHorizontalAlignment() );
        break;
    default:
        // should not get here, TODO log anomaly
        SetOrientation( TEXT_ATTRIBUTES::ANGLE_0 );
    }
    return *this;
}


TEXT_ATTRIBUTES& TEXT_ATTRIBUTES::SpinCW()
{
    switch( m_orientation )
    {
    case TEXT_ATTRIBUTES::ANGLE_0:
        SetOrientation( TEXT_ATTRIBUTES::ANGLE_90 );
        Align( OppositeHorizontalAlignment() );
        break;
    case TEXT_ATTRIBUTES::ANGLE_90: SetOrientation( TEXT_ATTRIBUTES::ANGLE_0 ); break;
    default:
        // should not get here, TODO log anomaly
        SetOrientation( TEXT_ATTRIBUTES::ANGLE_0 );
    }
    return *this;
}


TEXT_ATTRIBUTES& TEXT_ATTRIBUTES::Align( HORIZONTAL_ALIGNMENT aHorizontalAlignment )
{
#ifdef OUTLINEFONT_DEBUG
    std::cerr << "TEXT_ATTRIBUTES::Align( " << aHorizontalAlignment << " )" << std::endl;
#endif
    m_horizontal_alignment = aHorizontalAlignment;
    return *this;
}


TEXT_ATTRIBUTES& TEXT_ATTRIBUTES::Align( VERTICAL_ALIGNMENT aVerticalAlignment )
{
#ifdef OUTLINEFONT_DEBUG
    std::cerr << "TEXT_ATTRIBUTES::Align( " << aVerticalAlignment << " )" << std::endl;
#endif
    m_vertical_alignment = aVerticalAlignment;
    return *this;
}


TEXT_ATTRIBUTES& TEXT_ATTRIBUTES::SetAngle( const EDA_ANGLE& aAngle )
{
    m_orientation = ReadOrientation( aAngle );
    if( m_orientation == TEXT_ATTRIBUTES::FREE_ANGLE )
    {
        m_angle = EDA_ANGLE( aAngle );
    }

    return *this;
}


EDA_TEXT_HJUSTIFY_T TEXT_ATTRIBUTES::GetHorizJustify() const
{
    switch( m_horizontal_alignment )
    {
    case TEXT_ATTRIBUTES::H_LEFT: return EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT;
    case TEXT_ATTRIBUTES::H_RIGHT: return EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_RIGHT;
    case TEXT_ATTRIBUTES::H_CENTER:
    default: return EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_CENTER;
    }
}


EDA_TEXT_VJUSTIFY_T TEXT_ATTRIBUTES::GetVertJustify() const
{
    switch( m_vertical_alignment )
    {
    case TEXT_ATTRIBUTES::V_TOP: return EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_TOP;
    case TEXT_ATTRIBUTES::V_BOTTOM: return EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_BOTTOM;
    case TEXT_ATTRIBUTES::V_CENTER:
    default: return EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_CENTER;
    }
}


void TEXT_ATTRIBUTES::SetHorizJustify( EDA_TEXT_HJUSTIFY_T aHorizJustify )
{
#ifdef OUTLINEFONT_DEBUG
    std::cerr << "TEXT_ATTRIBUTES::SetHorizJustify( " << int( aHorizJustify ) << " )" << std::endl;
#endif
    switch( aHorizJustify )
    {
    case EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_LEFT: Align( TEXT_ATTRIBUTES::H_LEFT ); break;
    case EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_RIGHT: Align( TEXT_ATTRIBUTES::H_RIGHT ); break;
    case EDA_TEXT_HJUSTIFY_T::GR_TEXT_HJUSTIFY_CENTER:
    default: Align( TEXT_ATTRIBUTES::H_CENTER );
    }
}


void TEXT_ATTRIBUTES::SetVertJustify( EDA_TEXT_VJUSTIFY_T aVertJustify )
{
#ifdef OUTLINEFONT_DEBUG
    std::cerr << "TEXT_ATTRIBUTES::SetVertJustify( " << int( aVertJustify ) << " )" << std::endl;
#endif
    switch( aVertJustify )
    {
    case EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_TOP: Align( TEXT_ATTRIBUTES::V_TOP ); break;
    case EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_BOTTOM: Align( TEXT_ATTRIBUTES::V_BOTTOM ); break;
    case EDA_TEXT_VJUSTIFY_T::GR_TEXT_VJUSTIFY_CENTER:
    default: Align( TEXT_ATTRIBUTES::V_CENTER );
    }
}


TEXT_ATTRIBUTES::HORIZONTAL_ALIGNMENT
TEXT_ATTRIBUTES::ReadHorizontalAlignment( std::string aString )
{
    const char& c = aString.at( 0 );
    if( c == 'R' || c == 'r' )
        return TEXT_ATTRIBUTES::H_RIGHT;

    if( c == 'C' || c == 'c' )
        return TEXT_ATTRIBUTES::H_CENTER;

    return TEXT_ATTRIBUTES::H_LEFT;
}


TEXT_ATTRIBUTES::VERTICAL_ALIGNMENT TEXT_ATTRIBUTES::ReadVerticalAlignment( std::string aString )
{
    const char& c = aString.at( 0 );
    if( c == 'T' || c == 't' )
        return TEXT_ATTRIBUTES::V_TOP;

    if( c == 'C' || c == 'c' )
        return TEXT_ATTRIBUTES::V_CENTER;

    return TEXT_ATTRIBUTES::V_BOTTOM;
}
