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

#ifndef TEXT_ATTRIBUTES_H
#define TEXT_ATTRIBUTES_H

#include <iostream>
#include <wx/log.h>
#include <cmath>
#include <math/vector2d.h>
#include <eda_angle.h>
#include <eda_text.h>

#ifdef EDA_TEXT_OVERHAUL
// Graphic Text justify:
// Values -1,0,1 are used in computations, do not change them
enum EDA_TEXT_HJUSTIFY_T
{
    GR_TEXT_HJUSTIFY_LEFT = -1,
    GR_TEXT_HJUSTIFY_CENTER = 0,
    GR_TEXT_HJUSTIFY_RIGHT = 1
};
#endif //EDA_TEXT_OVERHAUL

inline std::ostream& operator<<( std::ostream& os, EDA_TEXT_HJUSTIFY_T hj )
{
    switch( hj )
    {
    case GR_TEXT_HJUSTIFY_LEFT: os << "LEFT"; break;
    case GR_TEXT_HJUSTIFY_CENTER: os << "CENTER"; break;
    default: os << "RIGHT";
    }
    return os;
}


#ifdef EDA_TEXT_OVERHAUL
enum EDA_TEXT_VJUSTIFY_T
{
    GR_TEXT_VJUSTIFY_TOP = -1,
    GR_TEXT_VJUSTIFY_CENTER = 0,
    GR_TEXT_VJUSTIFY_BOTTOM = 1
};
#endif //EDA_TEXT_OVERHAUL

inline std::ostream& operator<<( std::ostream& os, EDA_TEXT_VJUSTIFY_T vj )
{
    switch( vj )
    {
    case GR_TEXT_VJUSTIFY_TOP: os << "TOP"; break;
    case GR_TEXT_VJUSTIFY_CENTER: os << "CENTER"; break;
    default: os << "BOTTOM";
    }
    return os;
}


#ifdef EDA_TEXT_OVERHAUL
/*
 * Legacy code:
 * Spin style for text items of all kinds on schematics
 * Basically a higher level abstraction of rotation and justification of text
 */
class LABEL_SPIN_STYLE
{
public:
    enum SPIN : int
    {
        LEFT = 0,
        UP = 1,
        RIGHT = 2,
        BOTTOM = 3
    };


    LABEL_SPIN_STYLE() = default;
    constexpr LABEL_SPIN_STYLE( SPIN aSpin ) : m_spin( aSpin ) {}

    constexpr bool operator==( SPIN a ) const { return m_spin == a; }

    constexpr bool operator!=( SPIN a ) const { return m_spin != a; }

    operator int() const { return static_cast<int>( m_spin ); }

    LABEL_SPIN_STYLE RotateCW()
    {
        SPIN newSpin = m_spin;

        switch( m_spin )
        {
        case LABEL_SPIN_STYLE::LEFT: newSpin = LABEL_SPIN_STYLE::UP; break;
        case LABEL_SPIN_STYLE::UP: newSpin = LABEL_SPIN_STYLE::RIGHT; break;
        case LABEL_SPIN_STYLE::RIGHT: newSpin = LABEL_SPIN_STYLE::BOTTOM; break;
        case LABEL_SPIN_STYLE::BOTTOM: newSpin = LABEL_SPIN_STYLE::LEFT; break;
        default: wxLogWarning( "RotateCCW encountered unknown current spin style" ); break;
        }

        return LABEL_SPIN_STYLE( newSpin );
    }

    LABEL_SPIN_STYLE RotateCCW()
    {
        SPIN newSpin = m_spin;

        switch( m_spin )
        {
        case LABEL_SPIN_STYLE::LEFT: newSpin = LABEL_SPIN_STYLE::BOTTOM; break;
        case LABEL_SPIN_STYLE::BOTTOM: newSpin = LABEL_SPIN_STYLE::RIGHT; break;
        case LABEL_SPIN_STYLE::RIGHT: newSpin = LABEL_SPIN_STYLE::UP; break;
        case LABEL_SPIN_STYLE::UP: newSpin = LABEL_SPIN_STYLE::LEFT; break;
        default: wxLogWarning( "RotateCCW encountered unknown current spin style" ); break;
        }

        return LABEL_SPIN_STYLE( newSpin );
    }

    /*
     * Mirrors the label spin style across the X axis or simply swaps up and bottom
     */
    LABEL_SPIN_STYLE MirrorX()
    {
        SPIN newSpin = m_spin;

        switch( m_spin )
        {
        case LABEL_SPIN_STYLE::UP: newSpin = LABEL_SPIN_STYLE::BOTTOM; break;
        case LABEL_SPIN_STYLE::BOTTOM: newSpin = LABEL_SPIN_STYLE::UP; break;
        case LABEL_SPIN_STYLE::LEFT: break;
        case LABEL_SPIN_STYLE::RIGHT: break;
        default: wxLogWarning( "MirrorX encountered unknown current spin style" ); break;
        }

        return LABEL_SPIN_STYLE( newSpin );
    }

    /*
     * Mirrors the label spin style across the Y axis or simply swaps left and right
     */
    LABEL_SPIN_STYLE MirrorY()
    {
        SPIN newSpin = m_spin;

        switch( m_spin )
        {
        case LABEL_SPIN_STYLE::LEFT: newSpin = LABEL_SPIN_STYLE::RIGHT; break;
        case LABEL_SPIN_STYLE::RIGHT: newSpin = LABEL_SPIN_STYLE::LEFT; break;
        case LABEL_SPIN_STYLE::UP: break;
        case LABEL_SPIN_STYLE::BOTTOM: break;
        default: wxLogWarning( "MirrorY encountered unknown current spin style" ); break;
        }

        return LABEL_SPIN_STYLE( newSpin );
    }

private:
    SPIN m_spin;
};
#endif //EDA_TEXT_OVERHAUL


class TEXT_ATTRIBUTES
{
public:
    enum ORIENTATION : int
    {
        ANGLE_0 = 0,
        ANGLE_90 = 90,
        ANGLE_180 = 180,
        ANGLE_270 = 270,
        FREE_ANGLE
    };

    enum HORIZONTAL_ALIGNMENT : int
    {
        H_LEFT = -1,
        H_CENTER = 0,
        H_RIGHT = 1
    };

    enum VERTICAL_ALIGNMENT : int
    {
        V_TOP = -1,
        V_CENTER = 0,
        V_BOTTOM = 1
    };

    // Constructors

    TEXT_ATTRIBUTES( const EDA_ANGLE& aAngle, EDA_TEXT_HJUSTIFY_T aHorizontalJustify,
                     EDA_TEXT_VJUSTIFY_T aVerticalJustify );


    TEXT_ATTRIBUTES() :
            m_orientation( TEXT_ATTRIBUTES::ANGLE_0 ),
            m_horizontal_alignment( TEXT_ATTRIBUTES::H_LEFT ),
            m_vertical_alignment( TEXT_ATTRIBUTES::V_BOTTOM )
    {
    }


#ifdef EDA_TEXT_OVERHAUL
    // LABEL_SPIN_STYLE only used in eeschema
    TEXT_ATTRIBUTES( LABEL_SPIN_STYLE aSpin );
#endif //EDA_TEXT_OVERHAUL

    TEXT_ATTRIBUTES( const TEXT_ATTRIBUTES& aAttributes ) :
            m_orientation( aAttributes.GetOrientation() ),
            m_horizontal_alignment( aAttributes.GetHorizontalAlignment() ),
            m_vertical_alignment( aAttributes.GetVerticalAlignment() )
    {
    }


    TEXT_ATTRIBUTES( ORIENTATION aOrientation, HORIZONTAL_ALIGNMENT aHorizontalAlignment,
                     VERTICAL_ALIGNMENT aVerticalAlignment ) :
            m_orientation( aOrientation ),
            m_horizontal_alignment( aHorizontalAlignment ),
            m_vertical_alignment( aVerticalAlignment )
    {
    }


    TEXT_ATTRIBUTES( ORIENTATION aOrientation ) :
            m_orientation( aOrientation ), m_horizontal_alignment( TEXT_ATTRIBUTES::H_LEFT ),
            m_vertical_alignment( TEXT_ATTRIBUTES::V_BOTTOM )
    {
    }


    TEXT_ATTRIBUTES( HORIZONTAL_ALIGNMENT aHorizontalAlignment ) :
            m_orientation( TEXT_ATTRIBUTES::ANGLE_0 ),
            m_horizontal_alignment( aHorizontalAlignment ),
            m_vertical_alignment( TEXT_ATTRIBUTES::V_BOTTOM )
    {
    }


    TEXT_ATTRIBUTES( VERTICAL_ALIGNMENT aVerticalAlignment ) :
            m_orientation( TEXT_ATTRIBUTES::ANGLE_0 ),
            m_horizontal_alignment( TEXT_ATTRIBUTES::H_LEFT ),
            m_vertical_alignment( aVerticalAlignment )
    {
    }


    TEXT_ATTRIBUTES( HORIZONTAL_ALIGNMENT aHorizontalAlignment,
                     VERTICAL_ALIGNMENT   aVerticalAlignment ) :
            m_orientation( TEXT_ATTRIBUTES::ANGLE_0 ),
            m_horizontal_alignment( aHorizontalAlignment ),
            m_vertical_alignment( aVerticalAlignment )
    {
    }


    TEXT_ATTRIBUTES( ORIENTATION aOrientation, HORIZONTAL_ALIGNMENT aHorizontalAlignment ) :
            m_orientation( aOrientation ), m_horizontal_alignment( aHorizontalAlignment ),
            m_vertical_alignment( TEXT_ATTRIBUTES::V_BOTTOM )
    {
    }


    TEXT_ATTRIBUTES( ORIENTATION aOrientation, VERTICAL_ALIGNMENT aVerticalAlignment ) :
            m_orientation( aOrientation ), m_horizontal_alignment( TEXT_ATTRIBUTES::H_LEFT ),
            m_vertical_alignment( aVerticalAlignment )
    {
    }


    void SetFrom( const TEXT_ATTRIBUTES& aAttributes )
    {
        m_orientation = aAttributes.GetOrientation();
        m_horizontal_alignment = aAttributes.GetHorizontalAlignment();
        m_vertical_alignment = aAttributes.GetVerticalAlignment();
    }

    /**
     * Rotate counterclockwise.
     * @return the attributes after rotation
     */
    TEXT_ATTRIBUTES& RotateCCW();


    /**
     * Rotate clockwise.
     * @return the attributes after rotation
     */
    TEXT_ATTRIBUTES& RotateCW();

    /**
     * Spin counterclockwise.
     * Flip-flops between 0 and 90 degrees.
     * Changes text alignment to opposite when going from vertical to horizontal.
     * This replicates the LABEL_SPIN_STYLE sequence of
     * horizontal-left-aligned -> vertical-left-aligned -> horizontal-right-aligned
     * -> vertical-right-aligned -> back to beginning
     * @return the attributes after spin
     */
    TEXT_ATTRIBUTES& SpinCCW();


    /**
     * Spin clockwise.
     * Flip-flops between 0 and 90 degrees.
     * Changes text alignment to opposite when going from vertical to horizontal.
     * This replicates the LABEL_SPIN_STYLE sequence of
     * horizontal-left-aligned -> vertical-right-aligned -> horizontal-right-aligned
     * -> vertical-left-aligned -> back to beginning
     * @return the attributes after spin
     */
    TEXT_ATTRIBUTES& SpinCW();


    TEXT_ATTRIBUTES& Align( HORIZONTAL_ALIGNMENT aHorizontalAlignment );

    TEXT_ATTRIBUTES& Align( VERTICAL_ALIGNMENT aVerticalAlignment );

    inline TEXT_ATTRIBUTES& Align( HORIZONTAL_ALIGNMENT aHorizontalAlignment,
                                   VERTICAL_ALIGNMENT   aVerticalAlignment )
    {
        Align( aHorizontalAlignment );
        Align( aVerticalAlignment );
        return *this;
    }


    static HORIZONTAL_ALIGNMENT
    HorizontalJustifyToAlignment( EDA_TEXT_HJUSTIFY_T aHorizontalJustify )
    {
        switch( aHorizontalJustify )
        {
        case GR_TEXT_HJUSTIFY_LEFT: return TEXT_ATTRIBUTES::H_LEFT; break;
        case GR_TEXT_HJUSTIFY_CENTER: return TEXT_ATTRIBUTES::H_CENTER; break;
        case GR_TEXT_HJUSTIFY_RIGHT: return TEXT_ATTRIBUTES::H_RIGHT; break;
        default:
            wxASSERT_MSG( 0 == 1, "Unknown horizontal justification value" );
            return TEXT_ATTRIBUTES::H_CENTER;
        }
    }

    static VERTICAL_ALIGNMENT VerticalJustifyToAlignment( EDA_TEXT_VJUSTIFY_T aVerticalJustify )
    {
        switch( aVerticalJustify )
        {
        case GR_TEXT_VJUSTIFY_TOP: return TEXT_ATTRIBUTES::V_TOP; break;
        case GR_TEXT_VJUSTIFY_CENTER: return TEXT_ATTRIBUTES::V_CENTER; break;
        case GR_TEXT_VJUSTIFY_BOTTOM: return TEXT_ATTRIBUTES::V_BOTTOM; break;
        default:
            wxASSERT_MSG( 0 == 1, "Unknown vertical justification value" );
            return TEXT_ATTRIBUTES::V_CENTER;
        }
    }

    HORIZONTAL_ALIGNMENT GetHorizontalAlignment() const { return m_horizontal_alignment; }

    HORIZONTAL_ALIGNMENT OppositeHorizontalAlignment() const
    {
        if( m_horizontal_alignment == TEXT_ATTRIBUTES::H_LEFT )
            return TEXT_ATTRIBUTES::H_RIGHT;
        if( m_horizontal_alignment == TEXT_ATTRIBUTES::H_RIGHT )
            return TEXT_ATTRIBUTES::H_LEFT;
        return m_horizontal_alignment;
    }

    EDA_TEXT_HJUSTIFY_T GetHorizJustify() const;

    void SetHorizJustify( EDA_TEXT_HJUSTIFY_T aType );

    VERTICAL_ALIGNMENT GetVerticalAlignment() const { return m_vertical_alignment; }

    VERTICAL_ALIGNMENT OppositeVerticalAlignment() const
    {
        if( m_vertical_alignment == TEXT_ATTRIBUTES::V_TOP )
            return TEXT_ATTRIBUTES::V_BOTTOM;
        if( m_vertical_alignment == TEXT_ATTRIBUTES::V_BOTTOM )
            return TEXT_ATTRIBUTES::V_TOP;
        return m_vertical_alignment;
    }

    EDA_TEXT_VJUSTIFY_T GetVertJustify() const;

    void SetVertJustify( EDA_TEXT_VJUSTIFY_T aType );


    ORIENTATION GetOrientation() const { return m_orientation; }

    inline TEXT_ATTRIBUTES& SetOrientation( ORIENTATION aOrientation )
    {
        m_orientation = aOrientation;
        return *this;
    }

    TEXT_ATTRIBUTES& SetHorizontal() { return SetOrientation( TEXT_ATTRIBUTES::ANGLE_0 ); }

    TEXT_ATTRIBUTES& SetVertical() { return SetOrientation( TEXT_ATTRIBUTES::ANGLE_90 ); }

    bool IsHorizontal() const
    {
        return m_orientation == TEXT_ATTRIBUTES::ANGLE_0
               or m_orientation == TEXT_ATTRIBUTES::ANGLE_180;
    }

    bool IsVertical() const
    {
        return m_orientation == TEXT_ATTRIBUTES::ANGLE_90
               or m_orientation == TEXT_ATTRIBUTES::ANGLE_270;
    }

    /**
     * @return angle
     */
    EDA_ANGLE GetAngle() const
    {
        if( m_orientation == TEXT_ATTRIBUTES::FREE_ANGLE )
        {
            return EDA_ANGLE( m_angle );
        }
        else
        {
            return EDA_ANGLE( m_orientation, EDA_ANGLE::DEGREES );
        }
    }

    /**
     * Set orientation/angle from aAngle:
     * iff aAngle represents one of the four cardinal directions, orientation is set correspondingly.
     * Otherwise orientation is set to TEXT_ATTRIBUTES::FREE_ANGLE and angle is set.
     *
     * @param aAngle new angle
     * @param aAngleType type of angle represented by aAngle
     * @return attributes with new angle
     */
    TEXT_ATTRIBUTES& SetAngle( double aAngle, EDA_ANGLE::ANGLE_TYPE aAngleType )
    {
        return SetAngle( EDA_ANGLE( aAngle, aAngleType ) );
    }

    TEXT_ATTRIBUTES& SetAngle( int aAngle, EDA_ANGLE::ANGLE_TYPE aAngleType )
    {
        return SetAngle( EDA_ANGLE( aAngle, aAngleType ) );
    }

    TEXT_ATTRIBUTES& SetAngle( const EDA_ANGLE& aAngle );

    /**
     * Converts an EDA_ANGLE into the corresponding orientation.
     * Defaults to TEXT_ATTRIBUTES::FREE_ANGLE if aAngle does not
     * represent a valid orientation (0, 90, 180, or 270 degrees).
     *
     * @param aAngle angle in degrees
     * @return corresponding orientation, or FREE_ANGLE
     */
    static ORIENTATION ReadOrientation( const EDA_ANGLE& aAngle );

    /**
     * Converts one of the following strings: "0", "90", "180", "270"
     * into the corresponding orientation. Defaults to
     * TEXT_ATTRIBUTES::FREE_ANGLE if string can't be read as any of the
     * above.
     *
     * @return corresponding orientation, or FREE_ANGLE
     */
    static ORIENTATION ReadOrientation( std::string aString )
    {
        return ReadOrientation( EDA_ANGLE( std::stoi( aString ), EDA_ANGLE::DEGREES ) );
    }

    /**
     * Converts one of the following strings: "left", "center", "right"
     * into the corresponding horizontal alignment. Defaults to H_LEFT if
     * string can't be read as any of the above.
     *
     * Comparison is done by case-insensitive testing of first letter
     * only.  These strings are code words and any translations should
     * not be passed here.
     *
     * @return corresponding alignment
     */
    static HORIZONTAL_ALIGNMENT ReadHorizontalAlignment( std::string aString );

    static HORIZONTAL_ALIGNMENT ReadHorizontalAlignment( wxString aString )
    {
        return ReadHorizontalAlignment( std::string( aString ) );
    }

    /**
     * Converts one of the following strings: "top", "center", "bottom"
     * into the corresponding vertical alignment. Defaults to V_BOTTOM if
     * string can't be read as any of the above.
     *
     * Comparison is done by case-insensitive testing of first letter
     * only.  These strings are code words and any translations should
     * not be passed here.
     *
     * @return corresponding alignment
     */
    static VERTICAL_ALIGNMENT ReadVerticalAlignment( std::string aString );

    static VERTICAL_ALIGNMENT ReadVerticalAlignment( wxString aString )
    {
        return ReadVerticalAlignment( std::string( aString ) );
    }

    static inline std::string ToString( HORIZONTAL_ALIGNMENT aHorizontalAlignment,
                                        bool                 aIsToken = false )
    {
        switch( aHorizontalAlignment )
        {
        case TEXT_ATTRIBUTES::H_LEFT: return aIsToken ? "left" : "Left"; break;
        case TEXT_ATTRIBUTES::H_CENTER: return aIsToken ? "center" : "Center"; break;
        default:
        case TEXT_ATTRIBUTES::H_RIGHT: return aIsToken ? "right" : "Right"; break;
        }
    }

    static inline std::string ToString( VERTICAL_ALIGNMENT aVerticalAlignment,
                                        bool               aIsToken = false )
    {
        switch( aVerticalAlignment )
        {
        case TEXT_ATTRIBUTES::V_TOP: return aIsToken ? "top" : "Top"; break;
        case TEXT_ATTRIBUTES::V_CENTER: return aIsToken ? "center" : "Center"; break;
        default:
        case TEXT_ATTRIBUTES::V_BOTTOM: return aIsToken ? "bottom" : "Bottom"; break;
        }
    }

    template <typename T>
    static std::string ToToken( T aHorizontalOrVerticalAlignment )
    {
        return TEXT_ATTRIBUTES::ToString( aHorizontalOrVerticalAlignment, true );
    }

    std::string HorizontalAlignmentToken() const { return ToToken( GetHorizontalAlignment() ); }

    std::string VerticalAlignmentToken() const { return ToToken( GetVerticalAlignment() ); }

    void SetFlipY( bool aFlipY ) { m_flip_y = aFlipY; }

    bool IsFlipY() const { return m_flip_y; }

    /**
     * Set stroke width.
     * @param aStrokeWidth new stroke width
     * @return old stroke width
     */
    int SetStrokeWidth( int aStrokeWidth )
    {
        std::swap( aStrokeWidth, m_stroke_width );
        return aStrokeWidth;
    }

    /**
     * Get stroke width.
     * @return stroke width
     */
    int GetStrokeWidth() const { return m_stroke_width; }

    /**
     * Set italic state.
     * @param aItalic true if italic, otherwise false
     * @return old italic state
     */
    bool SetItalic( bool aItalic )
    {
        std::swap( aItalic, m_italic );
        return aItalic;
    }

    /**
     * Get italic state.
     * @return true if italic, otherwise false
     */
    bool IsItalic() const { return m_italic; }

    /**
     * Set bold state.
     * @param aBold true if bold, otherwise false
     * @return old bold state
     */
    bool SetBold( bool aBold )
    {
        std::swap( aBold, m_bold );
        return aBold;
    }

    /**
     * Get bold state.
     * @return true if bold, otherwise false
     */
    bool IsBold() const { return m_bold; }

    /**
     * Set mirrored state.
     * @param aMirrored true if mirrored, otherwise false
     * @return old mirrored state
     */
    bool SetMirrored( bool aMirrored )
    {
        std::swap( aMirrored, m_mirrored );
        return aMirrored;
    }

    /**
     * Get mirrored state.
     * @return true if mirrored, otherwise false
     */
    bool IsMirrored() const { return m_mirrored; }

    /**
     * Set font size.
     * @param aSize new font size.
     * @return old font size.
     */
    VECTOR2D SetSize( const VECTOR2D& aSize )
    {
        VECTOR2D oldSize( m_size );
        m_size = VECTOR2D( aSize );
        return oldSize;
    }

    wxSize SetSize( const wxSize& aSize )
    {
        VECTOR2D oldSize = SetSize( VECTOR2D( aSize.x, aSize.y ) );
        return wxSize( oldSize.x, oldSize.y );
    }

    int SetWidth( int aWidth )
    {
        VECTOR2D oldSize( m_size );
        m_size = VECTOR2D( aWidth, oldSize.y );
        return oldSize.x;
    }

    int SetHeight( int aHeight )
    {
        VECTOR2D oldSize( m_size );
        m_size = VECTOR2D( oldSize.x, aHeight );
        return oldSize.y;
    }


    /**
     * Get font size.
     * @return font size.
     */
    VECTOR2D GetSize() const { return VECTOR2D( m_size ); }

private:
    ORIENTATION          m_orientation;
    HORIZONTAL_ALIGNMENT m_horizontal_alignment;
    VERTICAL_ALIGNMENT   m_vertical_alignment;
    EDA_ANGLE            m_angle;
    int                  m_stroke_width;
    bool                 m_italic;
    bool                 m_bold;
    bool                 m_mirrored;
    VECTOR2D             m_size;
    bool                 m_flip_y;
};


inline std::ostream& operator<<( std::ostream&                                os,
                                 const TEXT_ATTRIBUTES::HORIZONTAL_ALIGNMENT& aHorizontalAlignment )
{
    os << TEXT_ATTRIBUTES::ToString( aHorizontalAlignment );

    return os;
}


inline std::ostream& operator<<( std::ostream&                              os,
                                 const TEXT_ATTRIBUTES::VERTICAL_ALIGNMENT& aVerticalAlignment )
{
    os << TEXT_ATTRIBUTES::ToString( aVerticalAlignment );

    return os;
}


inline std::ostream& operator<<( std::ostream& os, const TEXT_ATTRIBUTES& attr )
{
    os << "(attributes " << attr.GetHorizontalAlignment() << " " << attr.GetVerticalAlignment()
       << " " << attr.GetAngle() << "(size " << attr.GetSize() << ") "
       << "(stroke_width " << attr.GetStrokeWidth() << ")" << ( attr.IsBold() ? " bold" : "" )
       << ( attr.IsItalic() ? " italic" : "" ) << ( attr.IsMirrored() ? " mirrored" : "" )
       << ( attr.IsFlipY() ? " flipY" : "" ) << ")";

    return os;
}


#endif //TEXT_ATTRIBUTES_H
