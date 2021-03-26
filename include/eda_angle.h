/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski.
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

#ifndef EDA_ANGLE_H
#define EDA_ANGLE_H

#include <cassert>
#include <cmath>
#include <iostream>

class EDA_ANGLE
{
public:
    enum ANGLE_TYPE
    {
        TENTHS_OF_A_DEGREE = 1,
        DEGREES = 10,
        RADIANS //< enum value does not matter
    };

    // Angles can be created in degrees, 1/10ths of a degree, and radians,
    // and read as any of the angle types
    //
    // Angle type must be explicitly specified at creation, because
    // there is no other way of knowing what an int or a double represents
    EDA_ANGLE( int aValue, ANGLE_TYPE aAngleType ) : m_initial_type( aAngleType )
    {
        switch( aAngleType )
        {
        case RADIANS:
            m_radians = aValue;
            m_value = int( aValue / TENTHS_OF_A_DEGREE_TO_RADIANS );
            break;
        default: m_value = aValue * aAngleType;
        }
    }

    EDA_ANGLE( double aValue, ANGLE_TYPE aAngleType ) : m_initial_type( aAngleType )
    {
        switch( aAngleType )
        {
        case RADIANS:
            m_radians = aValue;
            m_value = int( aValue / TENTHS_OF_A_DEGREE_TO_RADIANS );
            break;
        default: m_value = int( aValue * aAngleType );
        }
    }

    EDA_ANGLE() : m_value( 0 ), m_radians( 0.0 ), m_initial_type( EDA_ANGLE::RADIANS ) {}

    inline int AsDegrees() const { return m_value / EDA_ANGLE::DEGREES; }

    inline int AsTenthsOfADegree() const { return m_value; }

    inline double AsRadians() const
    {
        if( m_initial_type == EDA_ANGLE::RADIANS )
        {
            // if this was initialized with radians, return exact initial value
            return m_radians;
        }
        else
        {
            // otherwise compute from value stored as 1/10ths of a degree
            return m_value * TENTHS_OF_A_DEGREE_TO_RADIANS;
        }
    }

    inline double AsAngleType( ANGLE_TYPE aAngleType ) const
    {
        switch( aAngleType )
        {
        case TENTHS_OF_A_DEGREE: return AsTenthsOfADegree();
        case DEGREES: return AsDegrees();
        case RADIANS: return AsRadians();
        default: assert( 1 == 0 );
        }
    }

    static constexpr double TENTHS_OF_A_DEGREE_TO_RADIANS = M_PI / 1800;

    /**
     * @return true if angle is one of the four cardinal directions (0/90/180/270 degrees),
     *         otherwise false
     */
    bool IsCardinal() const { return AsTenthsOfADegree() % 900 == 0; }

    bool IsHorizontal() const { return AsDegrees() == 0 || AsDegrees() == 180; }

    bool IsVertical() const { return AsDegrees() == 90 || AsDegrees() == 270; }

    EDA_ANGLE RotateCW() const { return EDA_ANGLE( AsDegrees() - 90, EDA_ANGLE::DEGREES ); }

    EDA_ANGLE RotateCCW() const { return EDA_ANGLE( AsDegrees() + 90, EDA_ANGLE::DEGREES ); }

    EDA_ANGLE Add( const EDA_ANGLE& aAngle ) const
    {
        ANGLE_TYPE initialType = GetInitialAngleType();
        // if both were given in radians, addition is exact
        if( initialType == EDA_ANGLE::RADIANS
            && aAngle.GetInitialAngleType() == EDA_ANGLE::RADIANS )
        {
            //double newAngle = normalize( AsRadians() + aAngle.AsRadians(), EDA_ANGLE::RADIANS );
            double newAngle = AsRadians() + aAngle.AsRadians();
            return EDA_ANGLE( newAngle, EDA_ANGLE::RADIANS );
        }

        // if both were not given in radians, addition is done using
        // 1/10ths of a degree, then converted to original angle type
        // of this angle
        //int newAngle = normalize( AsTenthsOfADegree() + aAngle.AsTenthsOfADegree(),
        //EDA_ANGLE::TENTHS_OF_A_DEGREE );
        int newAngle = AsTenthsOfADegree() + aAngle.AsTenthsOfADegree();
        switch( initialType )
        {
        case DEGREES: return EDA_ANGLE( newAngle / EDA_ANGLE::DEGREES, EDA_ANGLE::DEGREES );
        case RADIANS:
            return EDA_ANGLE( newAngle / TENTHS_OF_A_DEGREE_TO_RADIANS, EDA_ANGLE::RADIANS );
        default:
        case TENTHS_OF_A_DEGREE: return EDA_ANGLE( newAngle, EDA_ANGLE::TENTHS_OF_A_DEGREE );
        }
    }

    EDA_ANGLE Invert() const
    {
        switch( GetInitialAngleType() )
        {
        case RADIANS: return EDA_ANGLE( -m_radians, EDA_ANGLE::RADIANS );
        default: return EDA_ANGLE( -m_value / GetInitialAngleType(), GetInitialAngleType() );
        }
    }

    EDA_ANGLE Subtract( const EDA_ANGLE& aAngle ) const { return Add( aAngle.Invert() ); }

    inline ANGLE_TYPE GetInitialAngleType() const { return m_initial_type; }

    double Sin() const { return sin( AsRadians() ); }

    double Cos() const { return cos( AsRadians() ); }

    double Tan() const { return tan( AsRadians() ); }

    static EDA_ANGLE Arccos( double x ) { return EDA_ANGLE( acos( x ), EDA_ANGLE::RADIANS ); }

    static EDA_ANGLE Arcsin( double x ) { return EDA_ANGLE( asin( x ), EDA_ANGLE::RADIANS ); }

    static EDA_ANGLE Arctan( double x ) { return EDA_ANGLE( atan( x ), EDA_ANGLE::RADIANS ); }

    static EDA_ANGLE Arctan2( double y, double x )
    {
        return EDA_ANGLE( atan2( y, x ), EDA_ANGLE::RADIANS );
    }

    inline void Normalize() { normalize( false ); }

    inline void Normalize720() { normalize( true ); }

private:
    // value is always stored in 1/10ths of a degree
    int m_value;

    double m_radians; //< only used with as-radians constructor

    ANGLE_TYPE m_initial_type;

    void normalize( bool n720 = false )
    {
        if( GetInitialAngleType() == EDA_ANGLE::RADIANS )
        {
            m_radians = normalize( m_radians, EDA_ANGLE::RADIANS, n720 );
            m_value = int( m_radians / TENTHS_OF_A_DEGREE_TO_RADIANS );
        }
        else
        {
            m_value = normalize( m_value, GetInitialAngleType(), n720 );
        }
    }

    int normalize( int aValue, ANGLE_TYPE aAngleType, bool n720 = false ) const
    {
        int full_circle_upper;

        switch( aAngleType )
        {
        case DEGREES: full_circle_upper = DEGREES_FULL_CIRCLE; break;
        case TENTHS_OF_A_DEGREE: full_circle_upper = TENTHS_OF_A_DEGREE_FULL_CIRCLE; break;
        case RADIANS:
            /* ?? should not get here */
            assert( 1 == 0 );
        }

        /* if n720 == false, clamp between 0..full_circle_upper
         * if n720 == true, clamp between +/- full_circle_upper
         */
        int full_circle_lower = n720 ? 0 : -full_circle_upper;

        while( aValue < full_circle_lower )
        {
            aValue += full_circle_upper;
        }
        while( aValue > full_circle_upper )
        {
            aValue -= full_circle_upper;
        }
        return aValue;
    }

    double normalize( double aValue, ANGLE_TYPE aAngleType, bool n720 = false ) const
    {
        double full_circle_upper;
        switch( aAngleType )
        {
        case DEGREES: full_circle_upper = DEGREES_FULL_CIRCLE; break;
        case TENTHS_OF_A_DEGREE: full_circle_upper = TENTHS_OF_A_DEGREE_FULL_CIRCLE; break;
        case RADIANS: full_circle_upper = RADIANS_FULL_CIRCLE;
        }

        double full_circle_lower = n720 ? 0 : -full_circle_upper;

        while( aValue < full_circle_lower )
        {
            aValue += full_circle_upper;
        }
        while( aValue > full_circle_upper )
        {
            aValue -= full_circle_upper;
        }
        return aValue;
    }

    static constexpr int    TENTHS_OF_A_DEGREE_FULL_CIRCLE = 3600;
    static constexpr int    DEGREES_FULL_CIRCLE = 360;
    static constexpr double RADIANS_FULL_CIRCLE = 2 * M_PI;

    static EDA_ANGLE m_horizontal;  //< 0 degrees
    static EDA_ANGLE m_vertical;    //< 90 degrees
    static EDA_ANGLE m_full_circle; //< 360 degrees

public:
    static constexpr EDA_ANGLE& HORIZONTAL = m_horizontal;

    static constexpr EDA_ANGLE& VERTICAL = m_vertical;

    static constexpr EDA_ANGLE& FULL_CIRCLE = m_full_circle;
};


inline EDA_ANGLE operator-( const EDA_ANGLE& aAngle )
{
    return aAngle.Invert();
}


inline bool operator<( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsTenthsOfADegree() < aAngleB.AsTenthsOfADegree();
}


inline bool operator>( const EDA_ANGLE& aAngleA, const EDA_ANGLE& aAngleB )
{
    return aAngleA.AsTenthsOfADegree() > aAngleB.AsTenthsOfADegree();
}


inline std::ostream& operator<<( std::ostream& os, const EDA_ANGLE& aAngle )
{
    EDA_ANGLE::ANGLE_TYPE initialType = aAngle.GetInitialAngleType();
    os << "(angle " << aAngle.AsAngleType( initialType ) << " " << initialType << ")";

    return os;
}


inline std::ostream& operator<<( std::ostream& os, const EDA_ANGLE::ANGLE_TYPE& aAngleType )
{
    switch( aAngleType )
    {
    case EDA_ANGLE::TENTHS_OF_A_DEGREE: os << "tenths-of-a-degree"; return os;
    case EDA_ANGLE::DEGREES: os << "degrees"; return os;
    case EDA_ANGLE::RADIANS: os << "radians"; return os;
    default: os << "unknown angle type?"; return os;
    }
}

#endif // EDA_ANGLE_H
