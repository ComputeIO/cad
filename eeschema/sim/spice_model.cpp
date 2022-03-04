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
    case DEVICE_TYPE::NONE:       return { "",           "" };
    case DEVICE_TYPE::RESISTOR:   return { "RESISTOR",   "Resistor" };
    case DEVICE_TYPE::CAPACITOR:  return { "CAPACITOR",  "Capacitor" };
    case DEVICE_TYPE::INDUCTOR:   return { "INDUCTOR",   "Inductor" };
    case DEVICE_TYPE::TLINE:      return { "TLINE",      "Transmission Line" };
    case DEVICE_TYPE::SWITCH:     return { "SWITCH",     "Switch" };

    case DEVICE_TYPE::DIODE:      return { "DIODE",      "Diode" };
    case DEVICE_TYPE::NPN:        return { "NPN",        "NPN BJT" };
    case DEVICE_TYPE::PNP:        return { "PNP",        "PNP BJT" };

    case DEVICE_TYPE::NJF:        return { "NJF",        "N-Channel JFET" };
    case DEVICE_TYPE::PJF:        return { "PJF",        "P-Channel JFET" };

    case DEVICE_TYPE::NMOS:       return { "NMOS",       "N-Channel MOSFET" };
    case DEVICE_TYPE::PMOS:       return { "PMOS",       "P-Channel MOSFET" };
    case DEVICE_TYPE::NMES:       return { "NMES",       "N-Channel MESFET" };
    case DEVICE_TYPE::PMES:       return { "PMES",       "P-Channel MESFET" };

    case DEVICE_TYPE::VSOURCE:    return { "VSOURCE",    "Voltage Source" };
    case DEVICE_TYPE::ISOURCE:    return { "ISOURCE",    "Current Source" };

    case DEVICE_TYPE::SUBCIRCUIT: return { "SUBCIRCUIT", "Subcircuit" };
    case DEVICE_TYPE::CODEMODEL:  return { "CODEMODEL",  "Code Model" };
    case DEVICE_TYPE::RAWSPICE:   return { "RAWSPICE",   "Raw Spice Element" };
    case DEVICE_TYPE::_ENUM_END:  break;
    }

    wxFAIL;
    return {};
}


TYPE_INFO SPICE_MODEL::TypeInfo( TYPE aType )
{
    switch( aType )
    { 
    case TYPE::NONE:                       return { DEVICE_TYPE::NONE,       NGSPICE::MODEL_TYPE::NONE,      "",                   ""                                                       };

    case TYPE::RESISTOR_IDEAL:             return { DEVICE_TYPE::RESISTOR,   NGSPICE::MODEL_TYPE::NONE,      "IDEAL",              "Ideal"                                                  };
    case TYPE::RESISTOR_ADVANCED:          return { DEVICE_TYPE::RESISTOR,   NGSPICE::MODEL_TYPE::RESISTOR,  "ADVANCED",           "Advanced"                                               };
    case TYPE::RESISTOR_BEHAVIORAL:        return { DEVICE_TYPE::RESISTOR,   NGSPICE::MODEL_TYPE::NONE,      "BEHAVIORAL",         "Behavioral"                                             };

    case TYPE::CAPACITOR_IDEAL:            return { DEVICE_TYPE::CAPACITOR,  NGSPICE::MODEL_TYPE::NONE,      "IDEAL",              "Ideal"                                                  };
    case TYPE::CAPACITOR_ADVANCED:         return { DEVICE_TYPE::CAPACITOR,  NGSPICE::MODEL_TYPE::CAPACITOR, "ADVANCED",           "Advanced"                                               };
    case TYPE::CAPACITOR_BEHAVIORAL:       return { DEVICE_TYPE::CAPACITOR,  NGSPICE::MODEL_TYPE::NONE,      "BEHAVIORAL",         "Behavioral"                                             };

    case TYPE::INDUCTOR_IDEAL:             return { DEVICE_TYPE::INDUCTOR,   NGSPICE::MODEL_TYPE::NONE,      "IDEAL",              "Ideal"                                                  };
    case TYPE::INDUCTOR_ADVANCED:          return { DEVICE_TYPE::INDUCTOR,   NGSPICE::MODEL_TYPE::INDUCTOR,  "ADVANCED",           "Advanced"                                               };
    case TYPE::INDUCTOR_BEHAVIORAL:        return { DEVICE_TYPE::INDUCTOR,   NGSPICE::MODEL_TYPE::NONE,      "BEHAVIORAL",         "Behavioral"                                             };

    case TYPE::TLINE_LOSSY:                return { DEVICE_TYPE::TLINE,      NGSPICE::MODEL_TYPE::LTRA,      "LOSSY",              "Lossy"                                                  };
    case TYPE::TLINE_LOSSLESS:             return { DEVICE_TYPE::TLINE,      NGSPICE::MODEL_TYPE::TRANLINE,  "LOSSLESS",           "Lossless"                                               };
    case TYPE::TLINE_UNIFORM_RC:           return { DEVICE_TYPE::TLINE,      NGSPICE::MODEL_TYPE::URC,       "UNIFORM_RC",         "Uniform RC"                                             };
    case TYPE::TLINE_KSPICE:               return { DEVICE_TYPE::TLINE,      NGSPICE::MODEL_TYPE::TRANSLINE, "KSPICE",             "KSPICE"                                                 };

    case TYPE::SWITCH_VCTRL:               return { DEVICE_TYPE::SWITCH,     NGSPICE::MODEL_TYPE::SWITCH,    "VCTRL",              "Voltage-controlled"                                     };
    case TYPE::SWITCH_ICTRL:               return { DEVICE_TYPE::SWITCH,     NGSPICE::MODEL_TYPE::CSWITCH,   "ICTRL",              "Current-controlled"                                     };

    case TYPE::DIODE:                      return { DEVICE_TYPE::DIODE,      NGSPICE::MODEL_TYPE::DIODE,     "",                   ""                                                       };
    
    case TYPE::NPN_GUMMEL_POON:            return { DEVICE_TYPE::NPN,        NGSPICE::MODEL_TYPE::BJT,       "GUMMEL_POON",        "Gummel-Poon"                                            };
    case TYPE::PNP_GUMMEL_POON:            return { DEVICE_TYPE::PNP,        NGSPICE::MODEL_TYPE::BJT,       "GUMMEL_POON",        "Gummel-Poon"                                            };
    case TYPE::NPN_VBIC:                   return { DEVICE_TYPE::NPN,        NGSPICE::MODEL_TYPE::VBIC,      "VBIC",               "VBIC"                                                   };
    case TYPE::PNP_VBIC:                   return { DEVICE_TYPE::PNP,        NGSPICE::MODEL_TYPE::VBIC,      "VBIC",               "VBIC"                                                   };
    //case TYPE::BJT_MEXTRAM:             return { DEVICE_TYPE::BJT,        "Q", 6,  "MEXTRAM",         "MEXTRAM" };
    case TYPE::NPN_HICUM_L2:               return { DEVICE_TYPE::NPN,        NGSPICE::MODEL_TYPE::HICUM2,    "HICUM_L2",           "HICUM Level 2"                                          };
    case TYPE::PNP_HICUM_L2:               return { DEVICE_TYPE::PNP,        NGSPICE::MODEL_TYPE::HICUM2,    "HICUM_L2",           "HICUM Level 2"                                          };
    //case TYPE::BJT_HICUM_L0:            return { DEVICE_TYPE::BJT,        "Q", 7,  "HICUM_L0",        "HICUM Level 0" };

    case TYPE::NJF_SHICHMAN_HODGES:        return { DEVICE_TYPE::NJF,        NGSPICE::MODEL_TYPE::JFET,      "SHICHMAN_HODGES",    "Shichman-Hodges"                                        };
    case TYPE::PJF_SHICHMAN_HODGES:        return { DEVICE_TYPE::PJF,        NGSPICE::MODEL_TYPE::JFET,      "SHICHMAN_HODGES",    "Shichman-Hodges"                                        };
    case TYPE::NJF_PARKER_SKELLERN:        return { DEVICE_TYPE::NJF,        NGSPICE::MODEL_TYPE::JFET2,     "PARKER_SKELLERN",    "Parker-Skellern"                                        };
    case TYPE::PJF_PARKER_SKELLERN:        return { DEVICE_TYPE::PJF,        NGSPICE::MODEL_TYPE::JFET2,     "PARKER_SKELLERN",    "Parker-Skellern"                                        };

    case TYPE::NMES_STATZ:                 return { DEVICE_TYPE::NMES,       NGSPICE::MODEL_TYPE::MES,       "STATZ",              "Statz"                                                  };
    case TYPE::PMES_STATZ:                 return { DEVICE_TYPE::PMES,       NGSPICE::MODEL_TYPE::MES,       "STATZ",              "Statz"                                                  };
    case TYPE::NMES_YTTERDAL:              return { DEVICE_TYPE::NMES,       NGSPICE::MODEL_TYPE::MESA,      "YTTERDAL",           "Ytterdal"                                               };
    case TYPE::PMES_YTTERDAL:              return { DEVICE_TYPE::PMES,       NGSPICE::MODEL_TYPE::MESA,      "YTTERDAL",           "Ytterdal"                                               };
    case TYPE::NMES_HFET1:                 return { DEVICE_TYPE::NMES,       NGSPICE::MODEL_TYPE::HFET1,     "HFET1",              "HFET1"                                                  };
    case TYPE::PMES_HFET1:                 return { DEVICE_TYPE::PMES,       NGSPICE::MODEL_TYPE::HFET1,     "HFET1",              "HFET1"                                                  };
    case TYPE::PMES_HFET2:                 return { DEVICE_TYPE::NMES,       NGSPICE::MODEL_TYPE::HFET2,     "HFET2",              "HFET2"                                                  };
    case TYPE::NMES_HFET2:                 return { DEVICE_TYPE::PMES,       NGSPICE::MODEL_TYPE::HFET2,     "HFET2",              "HFET2"                                                  };

    case TYPE::NMOS_MOS1:                  return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::MOS1,      "MOS1",               "Classical quadratic (MOS1)"                             };
    case TYPE::PMOS_MOS1:                  return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::MOS1,      "MOS1",               "Classical quadratic (MOS1)"                             };
    case TYPE::NMOS_MOS2:                  return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::MOS2,      "MOS2",               "Grove-Frohman (MOS2)"                                   };
    case TYPE::PMOS_MOS2:                  return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::MOS2,      "MOS2",               "Grove-Frohman (MOS2)"                                   };
    case TYPE::NMOS_MOS3:                  return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::MOS3,      "MOS3",               "MOS3"                                                   };
    case TYPE::PMOS_MOS3:                  return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::MOS3,      "MOS3",               "MOS3"                                                   };
    case TYPE::NMOS_BSIM1:                 return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::BSIM1,     "BSIM1",              "BSIM1"                                                  };
    case TYPE::PMOS_BSIM1:                 return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::BSIM1,     "BSIM1",              "BSIM1"                                                  };
    case TYPE::NMOS_BSIM2:                 return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::BSIM2,     "BSIM2",              "BSIM2"                                                  };
    case TYPE::PMOS_BSIM2:                 return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::BSIM2,     "BSIM2",              "BSIM2"                                                  };
    case TYPE::NMOS_MOS6:                  return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::MOS6,      "MOS6",               "MOS6"                                                   };
    case TYPE::PMOS_MOS6:                  return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::MOS6,      "MOS6",               "MOS6"                                                   };
    case TYPE::NMOS_BSIM3:                 return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::BSIM3,     "BSIM3",              "BSIM3"                                                  };
    case TYPE::PMOS_BSIM3:                 return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::BSIM3,     "BSIM3",              "BSIM3"                                                  };
    case TYPE::NMOS_MOS9:                  return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::MOS9,      "MOS9",               "MOS9"                                                   };
    case TYPE::PMOS_MOS9:                  return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::MOS9,      "MOS9",               "MOS9"                                                   };
    case TYPE::NMOS_B4SOI:                 return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::B4SOI,     "B4SOI",              "BSIM4 SOI (B4SOI)"                                      };
    case TYPE::PMOS_B4SOI:                 return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::B4SOI,     "B4SOI",              "BSIM4 SOI (B4SOI)"                                      };
    case TYPE::NMOS_BSIM4:                 return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::BSIM4,     "BSIM4",              "BSIM4"                                                  };
    case TYPE::PMOS_BSIM4:                 return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::BSIM4,     "BSIM4",              "BSIM4"                                                  };
    //case TYPE::NMOS_EKV2_6:           return { DEVICE_TYPE::NMOS,     "M", 44, "EKV2.6",          "EKV2.6" };
    //case TYPE::PMOS_EKV2_6:           return { DEVICE_TYPE::PMOS,     "M", 44, "EKV2.6",          "EKV2.6" };
    //case TYPE::NMOS_PSP:              return { DEVICE_TYPE::NMOS,     "M", 45, "PSP",             "PSP" };
    //case TYPE::PMOS_PSP:              return { DEVICE_TYPE::PMOS,     "M", 45, "PSP",             "PSP" };
    case TYPE::NMOS_B3SOIFD:               return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::B3SOIFD,   "B3SOIFD",            "B3SOIFD (BSIM3 fully depleted SOI)"                     };
    case TYPE::PMOS_B3SOIFD:               return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::B3SOIFD,   "B3SOIFD",            "B3SOIFD (BSIM3 fully depleted SOI)"                     };
    case TYPE::NMOS_B3SOIDD:               return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::B3SOIDD,   "B3SOIDD",            "B3SOIDD (BSIM3 SOI, both fully and partially depleted)" };
    case TYPE::PMOS_B3SOIDD:               return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::B3SOIDD,   "B3SOIDD",            "B3SOIDD (BSIM3 SOI, both fully and partially depleted)" };
    case TYPE::NMOS_B3SOIPD:               return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::B3SOIPD,   "B3SOIPD",            "B3SOIPD (BSIM3 partially depleted SOI)"                 };
    case TYPE::PMOS_B3SOIPD:               return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::B3SOIPD,   "B3SOIPD",            "B3SOIPD (BSIM3 partially depleted SOI)"                 };
    //case TYPE::NMOS_STAG:             return { DEVICE_TYPE::NMOS,     "M", 60, "STAG",            "STAG" };
    //case TYPE::PMOS_STAG:             return { DEVICE_TYPE::PMOS,     "M", 60, "STAG",            "STAG" };
    case TYPE::NMOS_HISIM2:                return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::HISIM2,    "HISIM2",             "HiSIM2"                                                 };
    case TYPE::PMOS_HISIM2:                return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::HISIM2,    "HISIM2",             "HiSIM2"                                                 };
    case TYPE::NMOS_HISIM_HV1:             return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::HISIMHV1,  "HISIM_HV1",          "HiSIM_HV1"                                              };
    case TYPE::PMOS_HISIM_HV1:             return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::HISIMHV1,  "HISIM_HV1",          "HiSIM_HV1"                                              };
    case TYPE::NMOS_HISIM_HV2:             return { DEVICE_TYPE::NMOS,       NGSPICE::MODEL_TYPE::HISIMHV2,  "HISIM_HV2",          "HiSIM_HV2"                                              };
    case TYPE::PMOS_HISIM_HV2:             return { DEVICE_TYPE::PMOS,       NGSPICE::MODEL_TYPE::HISIMHV2,  "HISIM_HV2",          "HiSIM_HV2"                                              };

    case TYPE::VSOURCE_PULSE:              return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "PULSE",              "Pulse"                                                  };
    case TYPE::VSOURCE_SIN:                return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "SINCE",              "Sine"                                                   };
    case TYPE::VSOURCE_EXP:                return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "EXP",                "Exponential"                                            };
    case TYPE::VSOURCE_SFAM:               return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "SFAM",               "Single-frequency AM"                                    };
    case TYPE::VSOURCE_SFFM:               return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "SFFM",               "Single-frequency FM"                                    };
    case TYPE::VSOURCE_PWL:                return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "PWL",                "Piecewise linear"                                       };
    case TYPE::VSOURCE_NOISE:              return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "NOISE",              "Noise"                                                  };
    case TYPE::VSOURCE_RANDOM_UNIFORM:     return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "RANDOM_UNIFORM",     "Random uniform"                                         };
    case TYPE::VSOURCE_RANDOM_GAUSSIAN:    return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "RANDOM_GAUSSIAN",    "Random Gaussian"                                        };
    case TYPE::VSOURCE_RANDOM_EXPONENTIAL: return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "RANDOM_EXPONENTIAL", "Random exponential"                                     };
    case TYPE::VSOURCE_RANDOM_POISSON:     return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "RANDOM_POISSON",     "Random Poisson"                                         };
    case TYPE::VSOURCE_BEHAVIORAL:         return { DEVICE_TYPE::VSOURCE,    NGSPICE::MODEL_TYPE::NONE,      "BEHAVIORAL",         "Behavioral"                                             };

    case TYPE::ISOURCE_PULSE:              return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "PULSE",              "Pulse"                                                  };
    case TYPE::ISOURCE_SIN:                return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "SINCE",              "Sine"                                                   };
    case TYPE::ISOURCE_EXP:                return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "EXP",                "Exponential"                                            };
    case TYPE::ISOURCE_SFAM:               return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "SFAM",               "Single-frequency AM"                                    };
    case TYPE::ISOURCE_SFFM:               return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "SFFM",               "Single-frequency FM"                                    };
    case TYPE::ISOURCE_PWL:                return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "PWL",                "Piecewise linear"                                       };
    case TYPE::ISOURCE_NOISE:              return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "NOISE",              "Noise"                                                  };
    case TYPE::ISOURCE_RANDOM_UNIFORM:     return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "RANDOM_UNIFORM",     "Random uniform"                                         };
    case TYPE::ISOURCE_RANDOM_GAUSSIAN:    return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "RANDOM_GAUSSIAN",    "Random Gaussian"                                        };
    case TYPE::ISOURCE_RANDOM_EXPONENTIAL: return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "RANDOM_EXPONENTIAL", "Random exponential"                                     };
    case TYPE::ISOURCE_RANDOM_POISSON:     return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "RANDOM_POISSON",     "Random Poisson"                                         };
    case TYPE::ISOURCE_BEHAVIORAL:         return { DEVICE_TYPE::ISOURCE,    NGSPICE::MODEL_TYPE::NONE,      "BEHAVIORAL",         "Behavioral"                                             };

    case TYPE::SUBCIRCUIT:                 return { DEVICE_TYPE::SUBCIRCUIT, NGSPICE::MODEL_TYPE::NONE,      "",                   ""                                                       };
    case TYPE::CODEMODEL:                  return { DEVICE_TYPE::CODEMODEL,  NGSPICE::MODEL_TYPE::NONE,      "",                   ""                                                       };
    case TYPE::RAWSPICE:                   return { DEVICE_TYPE::RAWSPICE,   NGSPICE::MODEL_TYPE::NONE,      "",                   ""                                                       };

    case TYPE::_ENUM_END:                  break;
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

        modelInfo.name = TypeInfo( aType ).fieldValue;
        modelInfo.variant1 = "";
        modelInfo.variant2 = "";
        modelInfo.description = TypeInfo( aType ).description;
        modelInfo.modelParams = {};

        NGSPICE::PARAM_INFO paramInfo;
        paramInfo.dir = NGSPICE::PARAM_DIR::IN;
        paramInfo.flags = {};
        paramInfo.defaultValueOfVariant2 = "";

        switch( aType )
        {
        case TYPE::RESISTOR_IDEAL:
        case TYPE::CAPACITOR_IDEAL:
        case TYPE::INDUCTOR_IDEAL:
            paramInfo.name = ( aType == TYPE::RESISTOR_IDEAL )  ? "r" :
                             ( aType == TYPE::CAPACITOR_IDEAL ) ? "c" :
                                                                  "l";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::RESISTOR_IDEAL )  ? "ohm" :
                             ( aType == TYPE::CAPACITOR_IDEAL ) ? "F" :
                                                                  "H";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = ( aType == TYPE::RESISTOR_IDEAL )  ? "Resistance" :
                                    ( aType == TYPE::CAPACITOR_IDEAL ) ? "Capacitance" :
                                                                         "Inductance";
            modelInfo.instanceParams.push_back( paramInfo );
            break;

        case TYPE::RESISTOR_BEHAVIORAL:
        case TYPE::CAPACITOR_BEHAVIORAL:
        case TYPE::INDUCTOR_BEHAVIORAL:
        case TYPE::VSOURCE_BEHAVIORAL:
        case TYPE::ISOURCE_BEHAVIORAL:
            paramInfo.name = ( aType == TYPE::RESISTOR_BEHAVIORAL ) ?  "r" :
                             ( aType == TYPE::CAPACITOR_BEHAVIORAL ) ? "c" :
                             ( aType == TYPE::INDUCTOR_BEHAVIORAL ) ?  "l" :
                             ( aType == TYPE::VSOURCE_BEHAVIORAL ) ?   "v" :
                                                                       "i";
            paramInfo.type = NGSPICE::PARAM_TYPE::STRING;
            paramInfo.unit = ( aType == TYPE::RESISTOR_BEHAVIORAL )  ? "ohm" :
                             ( aType == TYPE::CAPACITOR_BEHAVIORAL ) ? "F" :
                             ( aType == TYPE::INDUCTOR_BEHAVIORAL )  ? "H" :
                             ( aType == TYPE::VSOURCE_BEHAVIORAL )   ? "V" :
                                                                       "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            /*paramInfo.description = ( aType == TYPE::RESISTOR_BEHAVIORAL )  ? "Resistance" :
                                    ( aType == TYPE::CAPACITOR_BEHAVIORAL ) ? "Capacitance" :
                                    ( aType == TYPE::INDUCTOR_BEHAVIORAL )  ? "Inductance" :
                                    ( aType == TYPE::VSOURCE_BEHAVIORAL )   ? "Voltage" :
                                                                              "Current";*/
            paramInfo.description = "Expression";
            modelInfo.instanceParams.push_back( paramInfo );

            // Not sure if tc1, tc2 should be exposed.

            break;
        
        case TYPE::VSOURCE_PULSE:
        case TYPE::ISOURCE_PULSE:
            paramInfo.name = ( aType == TYPE::VSOURCE_PULSE ) ? "v1" : "i1";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_PULSE ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Initial value";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = ( aType == TYPE::VSOURCE_PULSE ) ? "v2" : "i2";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_PULSE ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Initial value";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "td";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Delay";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "tr";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "tstep";
            paramInfo.description = "Rise time";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "tf";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "tstep";
            paramInfo.description = "Fall time";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "pw";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "tstop";
            paramInfo.description = "Pulse width";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "per";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "tstop";
            paramInfo.description = "Period";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "phase";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "deg";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Phase";
            modelInfo.instanceParams.push_back( paramInfo );
            break;

        case TYPE::VSOURCE_SIN:
        case TYPE::ISOURCE_SIN:
            paramInfo.name = ( aType == TYPE::VSOURCE_SIN ) ? "vo" : "io";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_SIN ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "DC offset";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = ( aType == TYPE::VSOURCE_SIN ) ? "va" : "ia";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_SIN ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Amplitude";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "freq";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "Hz";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "1/tstop";
            paramInfo.description = "Frequency";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "td";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Delay";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "theta";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "1/s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Damping factor";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "phase";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "deg";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Phase";
            modelInfo.instanceParams.push_back( paramInfo );
            break;

        case TYPE::VSOURCE_EXP:
        case TYPE::ISOURCE_EXP:
            paramInfo.name = ( aType == TYPE::VSOURCE_EXP ) ? "v1" : "i1";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_EXP ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Initial value";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = ( aType == TYPE::VSOURCE_EXP ) ? "v2" : "i2";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_EXP ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Pulsed value";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "td1";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Rise delay time";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "tau1";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "tstep";
            paramInfo.description = "Rise time constant";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "td2";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "td1+tstep";
            paramInfo.description = "Fall delay time";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "tau2";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "tstep";
            paramInfo.description = "Fall time constant";
            modelInfo.instanceParams.push_back( paramInfo );
            break;

        case TYPE::VSOURCE_SFAM:
        case TYPE::ISOURCE_SFAM:
            paramInfo.name = ( aType == TYPE::VSOURCE_SFAM ) ? "vo" : "io";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_SFAM ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "DC offset";

            paramInfo.name = ( aType == TYPE::VSOURCE_SFAM ) ? "va" : "ia";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_SFAM ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Amplitude";

            modelInfo.instanceParams.push_back( paramInfo );
            paramInfo.name = "mo";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Modulating signal offset";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "fc";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "Hz";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Carrier frequency";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "mf";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "Hz";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Modulating frequency";
            modelInfo.instanceParams.push_back( paramInfo );
            break;

        case TYPE::VSOURCE_SFFM:
        case TYPE::ISOURCE_SFFM:
            paramInfo.name = ( aType == TYPE::VSOURCE_SFFM ) ? "vo" : "io";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_SFFM ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "DC offset";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = ( aType == TYPE::VSOURCE_SFFM ) ? "va" : "ia";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_SFFM ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Amplitude";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "fc";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "Hz";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "1/tstop";
            paramInfo.description = "Carrier frequency";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "mdi";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Modulation index";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "fs";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "Hz";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "1/tstop";
            paramInfo.description = "Signal frequency";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "phasec";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "deg";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Carrier phase";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "phases";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "deg";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Signal phase";
            modelInfo.instanceParams.push_back( paramInfo );
            break;

        case TYPE::VSOURCE_PWL:
        case TYPE::ISOURCE_PWL:
            paramInfo.name = "t";
            paramInfo.type = NGSPICE::PARAM_TYPE::REALVEC;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Time vector";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = ( aType == TYPE::VSOURCE_PWL ) ? "v" : "i";
            paramInfo.type = NGSPICE::PARAM_TYPE::REALVEC;
            paramInfo.unit = ( aType == TYPE::VSOURCE_PWL ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = ( aType == TYPE::VSOURCE_PWL ) ?
                "Voltage vector" : "Current vector";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "repeat";
            paramInfo.type = NGSPICE::PARAM_TYPE::FLAG;
            paramInfo.unit = "";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "Repeat forever";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "td";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Delay";
            modelInfo.instanceParams.push_back( paramInfo );
            break;

        case TYPE::VSOURCE_NOISE:
        case TYPE::ISOURCE_NOISE:
            paramInfo.name = ( aType == TYPE::VSOURCE_NOISE ) ? "vo" : "io";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_NOISE ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "";
            paramInfo.description = "DC offset";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "na";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_NOISE ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "White noise RMS amplitude";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "nt";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Time step";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "nalpha";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "1/f noise exponent";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "namp";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "1/f noise RMS amplitude";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "rtsam";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_NOISE ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Burst noise amplitude";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "rtscapt";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Burst noise trap capture time";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "rtsemt";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Burst noise trap emission time";
            modelInfo.instanceParams.push_back( paramInfo );
            break;

        case TYPE::VSOURCE_RANDOM_UNIFORM:
        case TYPE::ISOURCE_RANDOM_UNIFORM:
            paramInfo.name = "min";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_RANDOM_UNIFORM ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "-0.5";
            paramInfo.description = "Min. value";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "max";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_RANDOM_UNIFORM ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0.5";
            paramInfo.description = "Max. value";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "td";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Delay";
            modelInfo.instanceParams.push_back( paramInfo );
            break;

        case TYPE::VSOURCE_RANDOM_GAUSSIAN:
        case TYPE::ISOURCE_RANDOM_GAUSSIAN:
            paramInfo.name = "mean";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_RANDOM_GAUSSIAN ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Mean";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "stddev";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_RANDOM_GAUSSIAN ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "1";
            paramInfo.description = "Standard deviation";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "td";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Delay";
            modelInfo.instanceParams.push_back( paramInfo );
            break;

        case TYPE::VSOURCE_RANDOM_EXPONENTIAL:
        case TYPE::ISOURCE_RANDOM_EXPONENTIAL:
            paramInfo.name = "offset";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_RANDOM_EXPONENTIAL ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Offset";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "mean";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_RANDOM_EXPONENTIAL ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "1";
            paramInfo.description = "Mean";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "td";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Delay";
            modelInfo.instanceParams.push_back( paramInfo );
            break;

        case TYPE::VSOURCE_RANDOM_POISSON:
        case TYPE::ISOURCE_RANDOM_POISSON:
            paramInfo.name = "offset";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_RANDOM_POISSON ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Offset";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "lambda";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = ( aType == TYPE::VSOURCE_RANDOM_POISSON ) ? "V" : "A";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "1";
            paramInfo.description = "Mean";
            modelInfo.instanceParams.push_back( paramInfo );

            paramInfo.name = "td";
            paramInfo.type = NGSPICE::PARAM_TYPE::REAL;
            paramInfo.unit = "s";
            paramInfo.category = NGSPICE::PARAM_CATEGORY::PRINCIPAL;
            paramInfo.defaultValueOfVariant1 = "0";
            paramInfo.description = "Delay";
            modelInfo.instanceParams.push_back( paramInfo );
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
