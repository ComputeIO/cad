/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Reece R. Pollack <reece@his.com>
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

#ifndef PCB_UNIT_BINDER_H_
#define PCB_UNIT_BINDER_H_

#include <widgets/unit_binder.h>
#include <pcb_origin_transforms.h>

class wxStaticText;
class wxWindow;

class PCB_BASE_FRAME;

class PCB_UNIT_BINDER : public UNIT_BINDER
{
public:
    /**
     */
    enum ORIGIN_TRANSFORM_TYPES_T {
        NULL_XFORM,
        ABS_X_COORD,
        ABS_Y_COORD,
        REL_X_COORD,
        REL_Y_COORD,
    };

private:
    const char* TransformType() const
    {
        switch( m_transformType )
        {
        case NULL_XFORM:    return "NULL_XFORM";
        case ABS_X_COORD:   return "ABS_X_COORD";
        case ABS_Y_COORD:   return "ABS_Y_COORD";
        case REL_X_COORD:   return "REL_X_COORD";
        case REL_Y_COORD:   return "REL_Y_COORD";
        }
        return "Unknown";
    }

public:
    /**
     * Constructor.
     * @param aParent is the parent PCB_BASE_FRAME.
     * @param aLabel is the static text used to label the text input widget (note: the label
     *               text, trimmed of its colon, will also be used in error messages)
     * @param aValue is the control used to edit or display the given value (wxTextCtrl,
     *               wxComboBox, wxStaticText, etc.).
     * @param aUnitLabel is the units label displayed after the text input widget
     * @param aUseMils specifies the use of mils for imperial units (instead of inches)
     * @param aAllowEval indicates \a aTextInput's content should be eval'ed before storing
     */
    PCB_UNIT_BINDER( PCB_BASE_FRAME* aParent,
                     wxStaticText* aLabel, wxWindow* aValue, wxStaticText* aUnitLabel,
                     bool aUseMils = false, bool aAllowEval = true );

    ~PCB_UNIT_BINDER() override;


    ORIGIN_TRANSFORM_TYPES_T GetOriginTransform() const
    {
        return m_transformType;
    }

    void SetOriginTransform( ORIGIN_TRANSFORM_TYPES_T aTransformType )
    {
        m_transformType = aTransformType;
    }


    using UNIT_BINDER::SetValue;
    void SetValue( int aValue ) override;

    void SetDoubleValue( double aValue ) override;

    using UNIT_BINDER::ChangeValue;
    void ChangeValue( int aValue ) override;

    long long int GetValue() override;

    double GetDoubleValue() override;


protected:
    long long xformForDisplay( long long aValue ) const;
    double xformForDisplay( double aValue ) const;

    long long xformFromInput( long long aValue ) const;
    double xformFromInput( double aValue ) const;

protected:
    /// Type of transform for this instantiation
    ORIGIN_TRANSFORM_TYPES_T  m_transformType;
    PCB_ORIGIN_TRANSFORMS     m_originTransforms;

};

#endif /* PCB_UNIT_BINDER_H_ */
