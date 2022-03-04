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
        SWITCH,

        DIODE,
        NPN,
        PNP,

        NJF,
        PJF,

        NMES,
        PMES,
        NMOS,
        PMOS,

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
        RESISTOR_BEHAVIORAL,

        CAPACITOR_IDEAL,
        CAPACITOR_ADVANCED,
        CAPACITOR_BEHAVIORAL,

        INDUCTOR_IDEAL,
        INDUCTOR_ADVANCED,
        INDUCTOR_BEHAVIORAL,

        TLINE_LOSSY,
        TLINE_LOSSLESS,
        TLINE_UNIFORM_RC,
        TLINE_KSPICE,

        SWITCH_VCTRL,
        SWITCH_ICTRL,

        DIODE,

        NPN_GUMMEL_POON,
        PNP_GUMMEL_POON,
        NPN_VBIC,
        PNP_VBIC,
        //NPN_MEXTRAM,
        //PNP_MEXTRAM,
        NPN_HICUM_L2,
        PNP_HICUM_L2,
        //NPN_HICUM_L0,
        //PNP_HICUM_L0,

        NJF_SHICHMAN_HODGES,
        PJF_SHICHMAN_HODGES,

        NJF_PARKER_SKELLERN,
        PJF_PARKER_SKELLERN,


        NMES_STATZ,
        PMES_STATZ,

        NMES_YTTERDAL,
        PMES_YTTERDAL,

        NMES_HFET1,
        PMES_HFET1,

        NMES_HFET2,
        PMES_HFET2,


        NMOS_MOS1,
        PMOS_MOS1,

        NMOS_MOS2,
        PMOS_MOS2,

        NMOS_MOS3,
        PMOS_MOS3,

        NMOS_BSIM1,
        PMOS_BSIM1,

        NMOS_BSIM2,
        PMOS_BSIM2,

        NMOS_MOS6,
        PMOS_MOS6,

        NMOS_MOS9,
        PMOS_MOS9,

        NMOS_BSIM3,
        PMOS_BSIM3,

        NMOS_B4SOI,
        PMOS_B4SOI,

        NMOS_BSIM4,
        PMOS_BSIM4,

        //NMOS_EKV2_6,
        //PMOS_EKV2_6,

        //NMOS_PSP,
        //PMOS_PSP,

        NMOS_B3SOIFD,
        PMOS_B3SOIFD,

        NMOS_B3SOIDD,
        PMOS_B3SOIDD,

        NMOS_B3SOIPD,
        PMOS_B3SOIPD,

        //NMOS_STAG,
        //PMOS_STAG,

        NMOS_HISIM2,
        PMOS_HISIM2,

        NMOS_HISIM_HV1,
        PMOS_HISIM_HV1,

        NMOS_HISIM_HV2,
        PMOS_HISIM_HV2,


        VSOURCE_PULSE,
        VSOURCE_SIN,
        VSOURCE_EXP,
        VSOURCE_SFAM,
        VSOURCE_SFFM,
        VSOURCE_PWL,
        VSOURCE_NOISE,
        VSOURCE_RANDOM_UNIFORM,
        VSOURCE_RANDOM_GAUSSIAN,
        VSOURCE_RANDOM_EXPONENTIAL,
        VSOURCE_RANDOM_POISSON,
        VSOURCE_BEHAVIORAL,

        ISOURCE_PULSE,
        ISOURCE_SIN,
        ISOURCE_EXP,
        ISOURCE_SFAM,
        ISOURCE_SFFM,
        ISOURCE_PWL,
        ISOURCE_NOISE,
        ISOURCE_RANDOM_UNIFORM,
        ISOURCE_RANDOM_GAUSSIAN,
        ISOURCE_RANDOM_EXPONENTIAL,
        ISOURCE_RANDOM_POISSON,
        ISOURCE_BEHAVIORAL,


        SUBCIRCUIT,
        CODEMODEL,
        RAWSPICE
    )

    struct TYPE_INFO
    {
        DEVICE_TYPE deviceType;
        NGSPICE::MODEL_TYPE ngspiceModelType;
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
