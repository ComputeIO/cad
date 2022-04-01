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

#ifndef SIM_MODEL_H
#define SIM_MODEL_H

#include <enum_vector.h>
#include <sch_field.h>
#include <lib_field.h>
#include <sim/sim_value.h>
#include <wx/string.h>
#include <map>
#include <stdexcept>

class SIM_LIBRARY;


namespace SIM_MODEL_PARSER
{
    using namespace SIM_VALUE_PARSER;


    struct eolComment : seq<one<';'>, until<eol>> {};
    struct commentLine : seq<one<'*'>, until<eol>> {};


    struct linespaces : plus<not_at<eol>,
                             space> {};
    struct newline : seq<sor<eol,
                             eolComment>,
                         not_at<one<'+'>>> {};
                         
    struct continuation : seq<opt<linespaces>,
                              sor<eol,
                                  eolComment>,
                              star<commentLine>,
                              one<'+'>,
                              opt<linespaces>> {};

    struct sep : sor<continuation,
                     linespaces> {};


    struct pinNumber : sor<digits, one<'X'>> {};
    struct pinSequence : seq<opt<pinNumber,
                                 star<sep,
                                      pinNumber>>> {};

    struct pinSequenceGrammar : must<opt<sep>,
                                     pinSequence,
                                     opt<sep>,
                                     eof> {};


    template <typename Rule> struct pinSequenceSelector : std::false_type {};
    template <> struct pinSequenceSelector<pinNumber> : std::true_type {};


    struct param : plus<alnum> {};

    template <SIM_VALUE_BASE::TYPE Type, NOTATION Notation>
    struct paramValuePair : seq<param,
                                opt<sep>,
                                one<'='>,
                                opt<sep>,
                                number<Type, Notation>> {};

    template <NOTATION Notation>
    struct paramValuePairs : seq<opt<paramValuePair<SIM_VALUE_BASE::TYPE::FLOAT,
                                                    Notation>,
                                     star<sep,
                                          paramValuePair<SIM_VALUE_BASE::TYPE::FLOAT,
                                                         Notation>>>> {};

    template <NOTATION Notation>
    struct paramValuePairsGrammar : must<opt<sep>,
                                         paramValuePairs<Notation>,
                                         opt<sep>,
                                         eof> {};


    template <typename Rule> struct paramValuePairsSelector : std::false_type {};
    template <> struct paramValuePairsSelector<param> : std::true_type {};
    template <> struct paramValuePairsSelector<number<SIM_VALUE_BASE::TYPE::INT, NOTATION::SI>>
        : std::true_type {};
    template <> struct paramValuePairsSelector<number<SIM_VALUE_BASE::TYPE::FLOAT, NOTATION::SI>>
        : std::true_type {};
    template <> struct paramValuePairsSelector<number<SIM_VALUE_BASE::TYPE::INT, NOTATION::SPICE>>
        : std::true_type {};
    template <> struct paramValuePairsSelector<number<SIM_VALUE_BASE::TYPE::FLOAT, NOTATION::SPICE>>
        : std::true_type {};




    struct spiceName : plus<alnum,
                            star<sor<alnum,
                                     one<'!', '#', '$', '%', '[', ']', '_'>>>> {};
                     /*seq<alpha,
                           star<sor<alnum,
                                    one<'!', '#', '$', '%', '[', ']', '_'>>>> {};*/


    struct spiceModelType : sor<TAO_PEGTL_ISTRING( "R" ),
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
    
    struct spiceModel : seq<TAO_PEGTL_ISTRING( ".model" ),
                            sep,
                            spiceName,
                            sep,
                            spiceModelType,
                            sor<seq<opt<sep>,
                                    one<'('>,
                                    opt<sep>,
                                    paramValuePairs<NOTATION::SPICE>,
                                    opt<sep>,
                                    // Ngspice doesn't require the parentheses to match, though.
                                    one<')'>>,
                                seq<sep,
                                    paramValuePairs<NOTATION::SPICE>>>,
                            opt<sep>,
                            newline> {};


    struct spicePinNumber : digits {};
    struct spicePinSequence : seq<opt<sep>,
                                  opt<spicePinNumber,
                                      star<sep,
                                           spicePinNumber>>,
                                  opt<sep>> {};

    struct spiceSubcktEnd : seq<TAO_PEGTL_ISTRING( ".ends" ),
                                opt<sep>,
                                newline> {};

    struct spiceSubckt : seq<TAO_PEGTL_ISTRING( ".subckt" ),
                             sep,
                             spiceName,
                             sep,
                             spicePinSequence,
                             opt<sep>,
                             newline,
                             until<spiceSubcktEnd>> {};


    struct spiceUnit : sor<spiceModel,
                           spiceSubckt> {};

    struct spiceUnitGrammar : must<spiceUnit, eof> {};


    template <typename Rule> struct spiceUnitSelector : std::false_type {};

    template <> struct spiceUnitSelector<spiceModel> : std::true_type {};
    template <> struct spiceUnitSelector<spiceName> : std::true_type {};
    template <> struct spiceUnitSelector<spiceModelType> : std::true_type {};
    template <> struct spiceUnitSelector<param> : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE_BASE::TYPE::INT, NOTATION::SI>>
        : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE_BASE::TYPE::FLOAT, NOTATION::SI>>
        : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE_BASE::TYPE::INT, NOTATION::SPICE>>
        : std::true_type {};
    template <> struct spiceUnitSelector<number<SIM_VALUE_BASE::TYPE::FLOAT, NOTATION::SPICE>>
        : std::true_type {};

    template <> struct spiceUnitSelector<spiceSubckt> : std::true_type {};
}


class SIM_MODEL
{
public:
    static constexpr auto DEVICE_TYPE_FIELD = "Model_Device";
    static constexpr auto TYPE_FIELD = "Model_Type";
    static constexpr auto PINS_FIELD = "Model_Pins";
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

    struct DEVICE_INFO
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
        VSOURCE_WHITE_NOISE,
        VSOURCE_PINK_NOISE,
        VSOURCE_BURST_NOISE,
        VSOURCE_RANDOM_UNIFORM,
        VSOURCE_RANDOM_NORMAL,
        VSOURCE_RANDOM_EXP,
        VSOURCE_RANDOM_POISSON,
        VSOURCE_BEHAVIORAL,

        ISOURCE_PULSE,
        ISOURCE_SIN,
        ISOURCE_EXP,
        ISOURCE_SFAM,
        ISOURCE_SFFM,
        ISOURCE_PWL,
        ISOURCE_WHITE_NOISE,
        ISOURCE_PINK_NOISE,
        ISOURCE_BURST_NOISE,
        ISOURCE_RANDOM_UNIFORM,
        ISOURCE_RANDOM_NORMAL,
        ISOURCE_RANDOM_EXP,
        ISOURCE_RANDOM_POISSON,
        ISOURCE_BEHAVIORAL,


        SUBCIRCUIT,
        CODEMODEL,
        RAWSPICE
    )

    struct INFO
    {
        DEVICE_TYPE deviceType;
        wxString fieldValue;
        wxString description;
    };


    struct SPICE_INFO
    {
        wxString itemType;
        wxString typeString = "";
        int level = 0;
        bool hasExpression = false;
        wxString version = "";
    };


    struct PIN
    {
        static constexpr auto NOT_CONNECTED = 0;

        int symbolPinNumber;
        const wxString name;
    };


    struct PARAM
    {
        enum class DIR
        {
            IN,
            OUT,
            INOUT
        };

        enum class CATEGORY
        {
            PRINCIPAL,
            DC,
            CAPACITANCE,
            TEMPERATURE,
            NOISE,
            DISTRIBUTED_QUANTITIES,
            GEOMETRY,
            LIMITING_VALUES,
            ADVANCED,
            FLAGS,
            INITIAL_CONDITIONS,
            SUPERFLUOUS
        };

        struct FLAGS {}; // Legacy.

        struct INFO
        {
            wxString name;
            unsigned int id = 0; // Legacy.
            DIR dir = DIR::INOUT;
            SIM_VALUE_BASE::TYPE type;
            FLAGS flags = {}; // Legacy
            wxString unit = "";
            CATEGORY category = CATEGORY::PRINCIPAL;
            wxString defaultValue = "";
            wxString defaultValueOfOtherVariant = ""; // Legacy.
            wxString description = "";
        };

        std::unique_ptr<SIM_VALUE_BASE> value;
        const INFO& info;
        bool isOtherVariant = false; // Legacy.
        
        PARAM( const INFO& aInfo, bool aIsOtherVariant = false )
            : value( SIM_VALUE_BASE::Create( aInfo.type ) ),
              info( aInfo ),
              isOtherVariant( aIsOtherVariant )
        {}
    };


    static DEVICE_INFO DeviceTypeInfo( DEVICE_TYPE aDeviceType );
    static INFO TypeInfo( TYPE aType );
    static SPICE_INFO SpiceInfo( TYPE aType );


    static TYPE ReadTypeFromSpiceCode( const std::string& aSpiceCode );

    template <typename T>
    static TYPE ReadTypeFromFields( const std::vector<T>& aFields );


    static std::unique_ptr<SIM_MODEL> Create( TYPE aType, int aSymbolPinCount = 0 );
    static std::unique_ptr<SIM_MODEL> Create( const std::string& aSpiceCode );
    static std::unique_ptr<SIM_MODEL> Create( const SIM_MODEL& aBaseModel );

    template <typename T>
    static std::unique_ptr<SIM_MODEL> Create( int aSymbolPinCount, const std::vector<T>& aFields );

    template <typename T>
    static wxString GetFieldValue( const std::vector<T>* aFields, const wxString& aFieldName );

    template <typename T>
    static void SetFieldValue( std::vector<T>& aFields, const wxString& aFieldName,
                               const wxString& aValue );


    // Move semantics.
    // Rule of five.
    virtual ~SIM_MODEL() = default;
    SIM_MODEL() = delete;
    SIM_MODEL( const SIM_MODEL& aOther ) = delete;
    SIM_MODEL( SIM_MODEL&& aOther ) = default;
    SIM_MODEL& operator=(SIM_MODEL&& aOther ) = delete;


    virtual bool ReadSpiceCode( const std::string& aSpiceCode );

    template <typename T>
    void ReadDataFields( int aSymbolPinCount, const std::vector<T>* aFields );

    // C++ doesn't allow virtual template methods, so we do this:
    virtual void ReadDataSchFields( int aSymbolPinCount, const std::vector<SCH_FIELD>* aFields );
    virtual void ReadDataLibFields( int aSymbolPinCount, const std::vector<LIB_FIELD>* aFields );


    template <typename T>
    void WriteFields( std::vector<T>& aFields );

    // C++ doesn't allow virtual template methods, so we do this:
    virtual void WriteDataSchFields( std::vector<SCH_FIELD>& aFields );
    virtual void WriteDataLibFields( std::vector<LIB_FIELD>& aFields );


    virtual void WriteCode( wxString& aCode ) = 0;

    void AddParam( const PARAM::INFO& aInfo, bool aIsOtherVariant = false );


    virtual wxString GetSpiceTypeString() { return SpiceInfo( GetType() ).typeString; }

    TYPE GetType() const { return m_type; }

    const SIM_MODEL* GetBaseModel() const { return m_baseModel; }
    void SetBaseModel( const SIM_MODEL& aBaseModel ) { m_baseModel = &aBaseModel; }

    int GetPinCount() const { return static_cast<int>( m_pins.size() ); }
    const PIN& GetPin( int aIndex ) const { return m_pins.at( aIndex ); }

    void SetPinSymbolPinNumber( int aIndex, int aSymbolPinNumber )
    {
        m_pins.at( aIndex ).symbolPinNumber = aSymbolPinNumber;
    }


    int GetParamCount() const { return static_cast<int>( m_params.size() ); }
    const PARAM& GetParam( int aIndex ) const;
    SIM_VALUE_BASE& ParamValue( int aIndex ) { return *m_params.at( aIndex ).value; }

    bool IsNonPrincipalParamOverridden() const;

protected:
    SIM_MODEL( TYPE aType );


private:
    static std::unique_ptr<SIM_MODEL> create( TYPE aType );

    static TYPE readTypeFromSpiceTypeString( const std::string& aTypeString );

    const SIM_MODEL* m_baseModel;
    wxString m_name;

    const TYPE m_type;
    std::vector<PIN> m_pins;
    std::vector<PARAM> m_params;


    template <typename T>
    void doReadDataFields( int aSymbolPinCount, const std::vector<T>* aFields );

    template <typename T>
    void doWriteFields( std::vector<T>& aFields );


    virtual std::vector<wxString> getPinNames() { return {}; }

    wxString generateDeviceTypeField();
    wxString generateTypeField();

    wxString generatePinsField();
    void parsePinsField( int aSymbolPinCount, const wxString& aPinsField );

    
    wxString generateParamsField();
    void parseParamsField( const wxString& aParamsField );

    virtual bool setParamFromSpiceCode( const wxString& aParamName, const wxString& aParamValue );
};

#endif // SIM_MODEL_H
