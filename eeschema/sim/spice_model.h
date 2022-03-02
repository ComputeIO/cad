/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SPICE_MODEL_H
#define SPICE_MODEL_H

#include <map>
#include <stdexcept>
#include <enum_vector.h>
#include <sch_field.h>
#include <lib_field.h>
#include <sim/ngspice.h>
#include <sim/spice_value.h>
#include <wx/string.h>

class SPICE_MODEL
{
public:
    static constexpr auto DEVICE_TYPE_FIELD = "Model_Device";
    static constexpr auto TYPE_FIELD = "Model_Type";
    static constexpr auto FILE_FIELD = "Model_File";
    static constexpr auto PARAMS_FIELD = "Model_Params";


    DEFINE_ENUM_CLASS_WITH_ITERATOR( DEVICE_TYPE, 
        NONE,

        RESISTOR,
        CAPACITOR,
        INDUCTOR,
        TLINE,

        DIODE,
        BJT,
        JFET,
        MESFET,
        MOSFET,

        VSOURCE,
        ISOURCE,

        SUBCIRCUIT,
        CODEMODEL,
        RAWSPICE
    )

    struct DEVICE_TYPE_INFO
    {
        wxString fieldValue;
        wxString description;
    };

    DEFINE_ENUM_CLASS_WITH_ITERATOR( TYPE,
        NONE,

        RESISTOR_IDEAL,
        RESISTOR_ADVANCED,

        CAPACITOR_IDEAL,
        CAPACITOR_ADVANCED,

        INDUCTOR_IDEAL,
        INDUCTOR_LOSSLESS_COIL,

        TLINE_LOSSY,
        TLINE_LOSSLESS,
        TLINE_DISTRIBUTED_RC,
        TLINE_KSPICE_LOSSY,

        DIODE,

        BJT_GUMMEL_POON,
        BJT_VBIC,
        //BJT_MEXTRAM,
        BJT_HICUM_L2,
        //BJT_HICUM_L0,

        JFET_SHICHMAN_HODGES,
        JFET_PARKER_SKELLERN,

        MESFET_STATZ,
        MESFET_YTTERDAL,
        MESFET_HFET1,
        MESFET_HFET2,

        MOSFET_MOS1,
        MOSFET_MOS2,
        MOSFET_MOS3,
        MOSFET_BSIM1,
        MOSFET_BSIM2,
        MOSFET_MOS6,
        MOSFET_MOS9,
        MOSFET_BSIM3,
        MOSFET_B4SOI,
        MOSFET_BSIM4,
        //MOSFET_EKV2_6,
        //MOSFET_PSP,
        MOSFET_B3SOIFD,
        MOSFET_B3SOIDD,
        MOSFET_B3SOIPD,
        //MOSFET_STAG,
        MOSFET_HISIM2,
        MOSFET_HISIM_HV1,
        MOSFET_HISIM_HV2,

        VSOURCE,
        ISOURCE,

        SUBCIRCUIT,
        CODEMODEL,
        RAWSPICE
    )

    struct TYPE_INFO
    {
        DEVICE_TYPE deviceType;
        NGSPICE::MODEL_TYPE ngspiceModelType;
        bool secondVariant;
        wxString fieldValue;
        wxString description;
    };


    static DEVICE_TYPE_INFO DeviceTypeInfo( DEVICE_TYPE aDeviceType );
    static TYPE_INFO TypeInfo( TYPE aType );
    static NGSPICE::MODEL_INFO TypeModelInfo( TYPE aType );

    template <typename T>
    static TYPE ReadTypeFromFields( const std::vector<T>* aFields );


    SPICE_MODEL( TYPE aType );

    template <typename T>
    SPICE_MODEL( const std::vector<T>* aFields );

    SPICE_MODEL( const wxString& aCode );


    template <typename T>
    void WriteFields( std::vector<T>* aFields );

    void WriteCode( wxString& aCode );


    wxString GetFile() { return m_file; }
    void SetFile( const wxString& aFile ) { m_file = aFile; }


private:
    TYPE m_type;
    wxString m_file;
    std::map<wxString, double> m_params;


    template <typename T>
    static wxString getFieldValue( const std::vector<T>* aFields, const wxString& aFieldName );

    template <typename T>
    static void setFieldValue( std::vector<T>* aFields, const wxString& aFieldName,
                               const wxString& aValue );


    wxString generateParamValuePairs();
    void parseParamValuePairs( const wxString& aParamValuePairs );
};

#endif /* SPICE_MODEL_H */
