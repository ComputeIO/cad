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

#include <iterator>
#include <sim/spice_model.h>
#include <pegtl.hpp>
#include <pegtl/contrib/parse_tree.hpp>
#include <locale_io.h>
#include <lib_symbol.h>

using DEVICE_TYPE = SPICE_MODEL::DEVICE_TYPE;
using DEVICE_TYPE_INFO = SPICE_MODEL::DEVICE_TYPE_INFO;
using TYPE = SPICE_MODEL::TYPE;
using TYPE_INFO = SPICE_MODEL::TYPE_INFO;

/*namespace SPICE_MODEL_PARSER
{
    using namespace tao::pegtl;

    struct directive : sor<TAO_PEGTL_ISTRING( ".model" ),
                           TAO_PEGTL_ISTRING( ".param" ),
                           TAO_PEGTL_ISTRING( ".subckt" )> {};*//*

    struct spaces : star<space> {};
    struct identifierNotFirstChar : sor<alnum, one<'!', '#', '$', '%', '[', ']', '_'>> {};
    struct identifier : seq<alpha, star<identifierNotFirstChar>> {};
    struct digits : plus<digit> {};

    struct sign : opt<one<'+', '-'>> {};
    struct significand : sor<seq<digits, one<'.'>, opt<digits>>, seq<one<'.'>, digits>> {};
    struct exponent : opt<one<'e', 'E'>, sign, digits> {};
    struct metricSuffix : sor<TAO_PEGTL_ISTRING( "T" ),
                              TAO_PEGTL_ISTRING( "G" ),
                              TAO_PEGTL_ISTRING( "Meg" ),
                              TAO_PEGTL_ISTRING( "K" ),
                              TAO_PEGTL_ISTRING( "mil" ),
                              TAO_PEGTL_ISTRING( "m" ),
                              TAO_PEGTL_ISTRING( "u" ),
                              TAO_PEGTL_ISTRING( "n" ),
                              TAO_PEGTL_ISTRING( "p" ),
                              TAO_PEGTL_ISTRING( "f" )> {};
    struct number : seq<sign, significand, exponent, metricSuffix> {};

    struct modelModelType : sor<TAO_PEGTL_ISTRING( "R" ),
                                TAO_PEGTL_ISTRING( "C" ),
                                TAO_PEGTL_ISTRING( "L" ),
                                TAO_PEGTL_ISTRING( "SW" ),
                                TAO_PEGTL_ISTRING( "CSW" ),
                                TAO_PEGTL_ISTRING( "URC" ),
                                TAO_PEGTL_ISTRING( "LTRA" ),
                                TAO_PEGTL_ISTRING( "D" ),
                                TAO_PEGTL_ISTRING( "NPN" ),
                                TAO_PEGTL_ISTRING( "PNP" ),
                                TAO_PEGTL_ISTRING( "NJF" ),
                                TAO_PEGTL_ISTRING( "PJF" ),
                                TAO_PEGTL_ISTRING( "NMOS" ),
                                TAO_PEGTL_ISTRING( "PMOS" ),
                                TAO_PEGTL_ISTRING( "NMF" ),
                                TAO_PEGTL_ISTRING( "PMF" ),
                                TAO_PEGTL_ISTRING( "VDMOS" )> {};
    struct paramValuePair : seq<alnum, spaces, one<'='>, spaces, number> {};
    struct paramValuePairs : opt<paramValuePair, star<spaces, paramValuePair>> {};
    struct modelModelSpec : seq<modelModelType,
                                spaces,
                                one<'('>,
                                spaces,

                                paramValuePairs,

                                spaces,
                                one<')'>,
                                spaces> {};
    struct modelModel : seq<TAO_PEGTL_ISTRING( ".model" ), identifier, modelModelSpec> {};

    struct model : modelModel {};
    //struct model : sor<modelModel, paramModel, subcircuitModel> {};
}*/

namespace SPICE_MODEL_PARSER
{
    using namespace tao::pegtl;

    struct spaces : star<space> {};
    struct digits : plus<digit> {};

    struct sign : opt<one<'+', '-'>> {};
    struct significand : sor<seq<digits, opt<one<'.'>, opt<digits>>>, seq<one<'.'>, digits>> {};
    struct exponent : opt<one<'e', 'E'>, sign, digits> {};
    struct metricSuffix : opt<sor<TAO_PEGTL_ISTRING( "T" ),
                                  TAO_PEGTL_ISTRING( "G" ),
                                  TAO_PEGTL_ISTRING( "Meg" ),
                                  TAO_PEGTL_ISTRING( "K" ),
                                  TAO_PEGTL_ISTRING( "mil" ),
                                  TAO_PEGTL_ISTRING( "m" ),
                                  TAO_PEGTL_ISTRING( "u" ),
                                  TAO_PEGTL_ISTRING( "n" ),
                                  TAO_PEGTL_ISTRING( "p" ),
                                  TAO_PEGTL_ISTRING( "f" )>> {};

    // TODO: Move the `number` grammar to the SPICE_VALUE class.
    struct number : seq<sign, significand, exponent, metricSuffix> {};

    struct param : seq<alnum> {};

    struct paramValuePair : seq<param, spaces, one<'='>, spaces, number> {};
    struct paramValuePairs : opt<paramValuePair, star<spaces, paramValuePair>> {};

    template <typename Rule> struct paramValuePairsSelector : std::false_type {};
    template <> struct paramValuePairsSelector<param> : std::true_type {};
    template <> struct paramValuePairsSelector<number> : std::true_type {};
}


DEVICE_TYPE_INFO SPICE_MODEL::DeviceTypeInfo( DEVICE_TYPE aDeviceType )
{
    switch( aDeviceType )
    {
    case DEVICE_TYPE::NONE:       return {"",           ""};
    case DEVICE_TYPE::RESISTOR:   return {"RESISTOR",   "Resistor"};
    case DEVICE_TYPE::CAPACITOR:  return {"CAPACITOR",  "Capacitor"};
    case DEVICE_TYPE::INDUCTOR:   return {"INDUCTOR",   "Inductor"};
    case DEVICE_TYPE::TLINE:      return {"TLINE",      "Transmission Line"};
    case DEVICE_TYPE::DIODE:      return {"DIODE",      "Diode"};
    case DEVICE_TYPE::BJT:        return {"BJT",        "BJT"};
    case DEVICE_TYPE::JFET:       return {"JFET",       "JFET"};
    case DEVICE_TYPE::MOSFET:     return {"MOSFET",     "MOSFET"};
    case DEVICE_TYPE::MESFET:     return {"MESFET",     "MESFET"};
    case DEVICE_TYPE::VSOURCE:    return {"VSOURCE",    "Voltage Source"};
    case DEVICE_TYPE::ISOURCE:    return {"ISOURCE",    "Current Source"};
    case DEVICE_TYPE::SUBCIRCUIT: return {"SUBCIRCUIT", "Subcircuit"};
    case DEVICE_TYPE::CODEMODEL:  return {"CODEMODEL",  "Code Model"};
    case DEVICE_TYPE::RAWSPICE:   return {"RAWSPICE",   "Raw Spice Element"};
    case DEVICE_TYPE::_ENUM_END:  break;
    }

    wxFAIL;
    return {};
}


TYPE_INFO SPICE_MODEL::TypeInfo( TYPE aType )
{
    switch( aType )
    { 
    case TYPE::NONE:                   return { DEVICE_TYPE::NONE,       NGSPICE::MODEL_TYPE::NONE,      "",                ""                                                             };

    case TYPE::RESISTOR_IDEAL:         return { DEVICE_TYPE::RESISTOR,   NGSPICE::MODEL_TYPE::NONE,      "IDEAL",           "Ideal model"                                                  };
    case TYPE::RESISTOR_ADVANCED:      return { DEVICE_TYPE::RESISTOR,   NGSPICE::MODEL_TYPE::RESISTOR,  "ADVANCED",        "Advanced model"                                               };

    case TYPE::CAPACITOR_IDEAL:        return { DEVICE_TYPE::CAPACITOR,  NGSPICE::MODEL_TYPE::NONE,      "IDEAL",           "Ideal model"                                                  };
    case TYPE::CAPACITOR_ADVANCED:     return { DEVICE_TYPE::CAPACITOR,  NGSPICE::MODEL_TYPE::CAPACITOR, "ADVANCED",        "Advanced model"                                               };

    case TYPE::INDUCTOR_IDEAL:         return { DEVICE_TYPE::INDUCTOR,   NGSPICE::MODEL_TYPE::NONE,      "IDEAL",           "Ideal model"                                                  };
    case TYPE::INDUCTOR_LOSSLESS_COIL: return { DEVICE_TYPE::INDUCTOR,   NGSPICE::MODEL_TYPE::INDUCTOR,  "LOSSLESS_COIL",   "Lossless coil model"                                          };

    case TYPE::TLINE_LOSSY:            return { DEVICE_TYPE::TLINE,      NGSPICE::MODEL_TYPE::LTRA,      "LOSSY",           "Lossy model"                                                  };
    case TYPE::TLINE_LOSSLESS:         return { DEVICE_TYPE::TLINE,      NGSPICE::MODEL_TYPE::TRANLINE,  "LOSSLESS",        "Lossless model"                                               };
    case TYPE::TLINE_DISTRIBUTED_RC:   return { DEVICE_TYPE::TLINE,      NGSPICE::MODEL_TYPE::URC,       "DISTRIBUTED_RC",  "Uniformly distributed RC model"                               };
    case TYPE::TLINE_KSPICE_LOSSY:     return { DEVICE_TYPE::TLINE,      NGSPICE::MODEL_TYPE::TRANSLINE, "KSPICE_LOSSY",    "KSPICE lossy model"                                           };

    case TYPE::DIODE:                  return { DEVICE_TYPE::DIODE,      NGSPICE::MODEL_TYPE::DIODE,     "",                ""                                                             };
    
    case TYPE::BJT_GUMMEL_POON:        return { DEVICE_TYPE::BJT,        NGSPICE::MODEL_TYPE::BJT,       "GUMMEL_POON",     "Gummel-Poon model"                                            };
    case TYPE::BJT_VBIC:               return { DEVICE_TYPE::BJT,        NGSPICE::MODEL_TYPE::VBIC,      "VBIC",            "VBIC model"                                                   };
    //case TYPE::BJT_MEXTRAM:             return { DEVICE_TYPE::BJT,        "Q", 6,  "MEXTRAM",         "MEXTRAM model" };
    case TYPE::BJT_HICUM_L2:           return { DEVICE_TYPE::BJT,        NGSPICE::MODEL_TYPE::HICUM2,    "HICUM_L2",        "HICUM Level 2 model"                                          };
    //case TYPE::BJT_HICUM_L0:            return { DEVICE_TYPE::BJT,        "Q", 7,  "HICUM_L0",        "HICUM Level 0 model" };

    case TYPE::JFET_SHICHMAN_HODGES:   return { DEVICE_TYPE::JFET,       NGSPICE::MODEL_TYPE::JFET,      "SHICHMAN_HODGES", "Shichman-Hodges model"                                        };
    case TYPE::JFET_PARKER_SKELLERN:   return { DEVICE_TYPE::JFET,       NGSPICE::MODEL_TYPE::JFET2,     "PARKER_SKELLERN", "Parker-Skellern model"                                        };

    case TYPE::MESFET_STATZ:           return { DEVICE_TYPE::MESFET,     NGSPICE::MODEL_TYPE::MES,       "STATZ",           "Statz model"                                                  };
    case TYPE::MESFET_YTTERDAL:        return { DEVICE_TYPE::MESFET,     NGSPICE::MODEL_TYPE::MESA,      "YTTERDAL",        "Ytterdal model"                                               };
    case TYPE::MESFET_HFET1:           return { DEVICE_TYPE::MESFET,     NGSPICE::MODEL_TYPE::HFET1,     "HFET1",           "HFET1 model"                                                  };
    case TYPE::MESFET_HFET2:           return { DEVICE_TYPE::MESFET,     NGSPICE::MODEL_TYPE::HFET2,     "HFET2",           "HFET2 model"                                                  };

    case TYPE::MOSFET_MOS1:            return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::MOS1,      "MOS1",            "Classical quadratic model (MOS1)"                             };
    case TYPE::MOSFET_MOS2:            return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::MOS2,      "MOS2",            "Grove-Frohman model (MOS2)"                                   };
    case TYPE::MOSFET_MOS3:            return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::MOS3,      "MOS3",            "MOS3 model"                                                   };
    case TYPE::MOSFET_BSIM1:           return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::BSIM1,     "BSIM1",           "BSIM1 model"                                                  };
    case TYPE::MOSFET_BSIM2:           return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::BSIM2,     "BSIM2",           "BSIM2 model"                                                  };
    case TYPE::MOSFET_MOS6:            return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::MOS6,      "MOS6",            "MOS6 model"                                                   };
    case TYPE::MOSFET_BSIM3:           return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::BSIM3,     "BSIM3",           "BSIM3 model"                                                  };
    case TYPE::MOSFET_MOS9:            return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::MOS9,      "MOS9",            "MOS9 model"                                                   };
    case TYPE::MOSFET_B4SOI:           return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::B4SOI,     "B4SOI",           "BSIM4 SOI model (B4SOI)"                                      };
    case TYPE::MOSFET_BSIM4:           return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::BSIM4,     "BSIM4",           "BSIM4 model"                                                  };
    //case TYPE::MOSFET_EKV2_6:           return { DEVICE_TYPE::MOSFET,     "M", 44, "EKV2.6",          "EKV2.6 model" };
    //case TYPE::MOSFET_PSP:              return { DEVICE_TYPE::MOSFET,     "M", 45, "PSP",             "PSP model" };
    case TYPE::MOSFET_B3SOIFD:         return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::B3SOIFD,   "B3SOIFD",         "B3SOIFD (BSIM3 fully depleted SOI) model"                     };
    case TYPE::MOSFET_B3SOIDD:         return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::B3SOIDD,   "B3SOIDD",         "B3SOIDD (BSIM3 SOI, both fully and partially depleted) model" };
    case TYPE::MOSFET_B3SOIPD:         return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::B3SOIPD,   "B3SOIPD",         "B3SOIPD (BSIM3 partially depleted SOI) model"                 };
    //case TYPE::MOSFET_STAG:             return { DEVICE_TYPE::MOSFET,     "M", 60, "STAG",            "STAG model" };
    case TYPE::MOSFET_HISIM2:          return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::HISIM2,    "HISIM2",          "HiSIM2 model"                                                 };
    case TYPE::MOSFET_HISIM_HV1:       return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::HISIMHV1,  "HISIM_HV1",       "HiSIM_HV1 model"                                              };
    case TYPE::MOSFET_HISIM_HV2:       return { DEVICE_TYPE::MOSFET,     NGSPICE::MODEL_TYPE::HISIMHV2,  "HISIM_HV2",       "HiSIM_HV2 model"                                              };

    case TYPE::VSOURCE:                return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "",                ""                                                             };
    case TYPE::ISOURCE:                return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "",                ""                                                             };

    case TYPE::SUBCIRCUIT:             return { DEVICE_TYPE::SUBCIRCUIT, NGSPICE::MODEL_TYPE::NONE,      "",                ""                                                             };
    case TYPE::CODEMODEL:              return { DEVICE_TYPE::CODEMODEL,  NGSPICE::MODEL_TYPE::NONE,      "",                ""                                                             };
    case TYPE::RAWSPICE:               return { DEVICE_TYPE::RAWSPICE,   NGSPICE::MODEL_TYPE::NONE,      "",                ""                                                             };

    case TYPE::_ENUM_END:              break;
    }

    wxFAIL;
    return {  };
}


NGSPICE::MODEL_INFO SPICE_MODEL::TypeModelInfo( TYPE aType )
{
    if( TypeInfo( aType ).ngspiceModelType != NGSPICE::MODEL_TYPE::NONE )
        return NGSPICE::ModelInfo( TypeInfo( aType ).ngspiceModelType );
    else
    {
        NGSPICE::MODEL_INFO modelInfo;

        switch( aType )
        {
        case TYPE::RESISTOR_IDEAL:
            modelInfo.name = "R";
            modelInfo.variant1 = "";
            modelInfo.variant2 = "";
            modelInfo.description = "Ideal resistor";
            modelInfo.modelParams = {};

            modelInfo.instanceParams["r"] = {};
            modelInfo.instanceParams.at( "r" ).dir = NGSPICE::PARAM_DIR::INOUT;
            modelInfo.instanceParams.at( "r" ).type = NGSPICE::PARAM_TYPE::REAL;
            modelInfo.instanceParams.at( "r" ).flags = {};
            modelInfo.instanceParams.at( "r" ).unit = "ohm";
            modelInfo.instanceParams.at( "r" ).category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            modelInfo.instanceParams.at( "r" ).defaultValueOfVariant1 = "";
            modelInfo.instanceParams.at( "r" ).defaultValueOfVariant2 = "";
            modelInfo.instanceParams.at( "r" ).description = "Resistance";
            break;

        case TYPE::CAPACITOR_IDEAL:
            modelInfo.name = "c";
            modelInfo.variant1 = "";
            modelInfo.variant2 = "";
            modelInfo.description = "Ideal capacitor";
            modelInfo.modelParams = {};

            modelInfo.instanceParams["c"] = {};
            modelInfo.instanceParams.at( "c" ).dir = NGSPICE::PARAM_DIR::INOUT;
            modelInfo.instanceParams.at( "c" ).type = NGSPICE::PARAM_TYPE::REAL;
            modelInfo.instanceParams.at( "c" ).flags = {};
            modelInfo.instanceParams.at( "c" ).unit = "F";
            modelInfo.instanceParams.at( "c" ).category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            modelInfo.instanceParams.at( "c" ).defaultValueOfVariant1 = "";
            modelInfo.instanceParams.at( "c" ).defaultValueOfVariant2 = "";
            modelInfo.instanceParams.at( "c" ).description = "Capacitance";
            break;

        case TYPE::INDUCTOR_IDEAL:
            modelInfo.name = "l";
            modelInfo.variant1 = "";
            modelInfo.variant2 = "";
            modelInfo.description = "Ideal inductor";
            modelInfo.modelParams = {};

            modelInfo.instanceParams["ind"] = {};
            modelInfo.instanceParams.at( "ind" ).dir = NGSPICE::PARAM_DIR::INOUT;
            modelInfo.instanceParams.at( "ind" ).type = NGSPICE::PARAM_TYPE::REAL;
            modelInfo.instanceParams.at( "ind" ).flags = {};
            modelInfo.instanceParams.at( "ind" ).unit = "H";
            modelInfo.instanceParams.at( "ind" ).category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            modelInfo.instanceParams.at( "ind" ).defaultValueOfVariant1 = "";
            modelInfo.instanceParams.at( "ind" ).defaultValueOfVariant2 = "";
            modelInfo.instanceParams.at( "ind" ).description = "Inductance";
            break;

        default:
            wxFAIL;
        }

        return modelInfo;
    }
}


template TYPE SPICE_MODEL::ReadTypeFromFields( const std::vector<SCH_FIELD>* aFields );
template TYPE SPICE_MODEL::ReadTypeFromFields( const std::vector<LIB_FIELD>* aFields );

template <typename T>
TYPE SPICE_MODEL::ReadTypeFromFields( const std::vector<T>* aFields )
{
    wxString typeFieldValue = getFieldValue( aFields, TYPE_FIELD );
    wxString deviceTypeFieldValue = getFieldValue( aFields, DEVICE_TYPE_FIELD );
    bool typeFound = false;

    for( TYPE type : TYPE_ITERATOR() )
    {
        if( typeFieldValue == TypeInfo( type ).fieldValue )
        {
            typeFound = true;

            if( deviceTypeFieldValue == DeviceTypeInfo( TypeInfo( type ).deviceType ).fieldValue )
                return type;
        }
    }

    if( !typeFound )
        throw KI_PARAM_ERROR( wxString::Format( _( "Invalid \"%s\" field value: \"%s\"" ),
                                                TYPE_FIELD, typeFieldValue ) );

    throw KI_PARAM_ERROR( wxString::Format( _( "Invalid \"%s\" field value: \"%s\"" ),
                                            DEVICE_TYPE_FIELD, deviceTypeFieldValue ) );
}


SPICE_MODEL::SPICE_MODEL( TYPE aType ) : m_type( aType )
{
}


template SPICE_MODEL::SPICE_MODEL( const std::vector<SCH_FIELD>* aFields );
template SPICE_MODEL::SPICE_MODEL( const std::vector<LIB_FIELD>* aFields );

template <typename T>
SPICE_MODEL::SPICE_MODEL( const std::vector<T>* aFields )
    : m_type( ReadTypeFromFields( aFields ) )
{
    SetFile( getFieldValue( aFields, "Model_File" ) );
    parseParamValuePairs( getFieldValue( aFields, "Model_Params" ) );
}


SPICE_MODEL::SPICE_MODEL( const wxString& aCode )
{
}


template void SPICE_MODEL::WriteFields( std::vector<SCH_FIELD>* aFields );
template void SPICE_MODEL::WriteFields( std::vector<LIB_FIELD>* aFields );

template <typename T>
void SPICE_MODEL::WriteFields( std::vector<T>* aFields )
{
    setFieldValue( aFields, DEVICE_TYPE_FIELD,
                   DeviceTypeInfo( TypeInfo( m_type ).deviceType ).fieldValue );
    setFieldValue( aFields, TYPE_FIELD, TypeInfo( m_type ).fieldValue );
    setFieldValue( aFields, FILE_FIELD, GetFile() );
    setFieldValue( aFields, PARAMS_FIELD, generateParamValuePairs() );
}


void SPICE_MODEL::WriteCode( wxString& aCode )
{
}


void SPICE_MODEL::parseParamValuePairs( const wxString& aParamValuePairs )
{
    LOCALE_IO toggle;
    
    tao::pegtl::string_input<> in( aParamValuePairs.ToStdString(), "from_input" );
    auto root = tao::pegtl::parse_tree::parse<SPICE_MODEL_PARSER::paramValuePairs,
                                              SPICE_MODEL_PARSER::paramValuePairsSelector>( in );

    wxString paramName = "";

    for( const auto& node : root->children )
    {
        if( node->is_type<SPICE_MODEL_PARSER::param>() )
            paramName = node->string();
        else if( node->is_type<SPICE_MODEL_PARSER::number>() )
        {
            wxASSERT( paramName != "" );

            try
            {
                SPICE_VALUE value( node->string() );

                /*if( !SetParamValue( paramName, value ) )
                {
                    m_params.clear();
                    throw KI_PARAM_ERROR( wxString::Format( _( "Unknown parameter \"%s\"" ),
                                                            paramName ) );
                }*/
            }
            catch( KI_PARAM_ERROR& e )
            {
                m_params.clear();
                throw KI_PARAM_ERROR( wxString::Format( _( "Invalid \"%s\" parameter value: \"%s\"" ),
                                                        paramName, e.What() ) );
            }
        }
        else
            wxFAIL;
    }
}


wxString SPICE_MODEL::generateParamValuePairs()
{
    wxString result = "";

    /*for( auto it = GetParams().cbegin(); it != GetParams().cend(); it++ )
    {
        result += it->first;
        result += "=";
        result += it->second.value.ToString();

        if( std::next( it ) != GetParams().cend() )
            result += " ";
    }*/

    return result;
}


template <typename T>
wxString SPICE_MODEL::getFieldValue( const std::vector<T>* aFields, const wxString& aFieldName )
{
    static_assert( std::is_same<T, SCH_FIELD>::value || std::is_same<T, LIB_FIELD>::value );

    auto fieldIt = std::find_if( aFields->begin(), aFields->end(),
                                 [&]( const T& f )
                                 {
                                     return f.GetName() == aFieldName;
                                 } );

    if( fieldIt != aFields->end() )
        return fieldIt->GetText();

    return wxEmptyString;
}


template <typename T>
void SPICE_MODEL::setFieldValue( std::vector<T>* aFields, const wxString& aFieldName,
                                 const wxString& aValue )
{
    static_assert( std::is_same<T, SCH_FIELD>::value || std::is_same<T, LIB_FIELD>::value );

    auto fieldIt = std::find_if( aFields->begin(), aFields->end(),
                                 [&]( const T& f )
                                 {
                                    return f.GetName() == aFieldName;
                                 } );

    if( fieldIt != aFields->end() )
    {
        fieldIt->SetText( aValue );
        return;
    }


    if constexpr( std::is_same<T, SCH_FIELD>::value )
    {
        wxASSERT( aFields->size() >= 1 );

        SCH_ITEM* parent = static_cast<SCH_ITEM*>( aFields->at( 0 ).GetParent() );
        aFields->emplace_back( wxPoint(), aFields->size(), parent, aFieldName );
    }
    else if constexpr( std::is_same<T, LIB_FIELD>::value )
        aFields->emplace_back( aFields->size(), aFieldName );

    aFields->back().SetText( aValue );
}
