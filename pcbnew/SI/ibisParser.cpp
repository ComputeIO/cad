#include <wx/string.h>
#include <wx/filename.h>
#include <reporter.h>
//#include "common.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>


#define IBIS_MAX_VERSION 7.0      // Up to v7.0, IBIS is fully backward compatible
#define IBIS_MAX_LINE_LENGTH 2048 // official limit is 1024
#define IBIS_

class IbisHeader
{
public:
    double   m_ibisVersion = -1;
    double   m_fileRevision = -1;
    wxString m_fileName;
    wxString m_source;
    wxString m_date;
    wxString m_notes;
    wxString m_disclaimer;
    wxString m_copyright;

    bool Check();
};

class TypMinMaxValue
{
public:
    double min = std::nan( " " );
    double typ = std::nan( " " );
    double max = std::nan( " " );
};

class IbisComponentPackage
{
public:
    TypMinMaxValue m_Rpkg;
    TypMinMaxValue m_Lpkg;
    TypMinMaxValue m_Cpkg;
};

class IbisComponentPin
{
public:
    wxString           m_pinName;
    wxString           m_signalName;
    wxString           m_modelName;
    TypMinMaxValue     m_Rpin;
    TypMinMaxValue     m_Lpin;
    TypMinMaxValue     m_Cpin;

    int m_Rcol = 0;
    int m_Lcol = 0;
    int m_Ccol = 0;

    bool m_virtual = false; // The table header regiters as a virtual pin.
};

class IbisComponentPinMapping
{
public:
    wxString m_pinName;
    wxString m_PDref;
    wxString m_PUref;
    wxString m_GNDClampRef;
    wxString m_POWERClampRef;
    wxString m_extRef;

    bool m_virtual;
};

class IbisComponent
{
public:
    wxString             m_name;
    wxString             m_manufacturer;
    IbisComponentPackage m_package;
    std::vector<IbisComponentPin> m_pins;
    std::vector<IbisComponentPinMapping> m_pinMappings;
    wxString                             m_packageModel;
    wxString             m_busLabel;
    wxString             m_dieSupplyPads;
    wxString             m_diffPins;

    bool Check();
};

bool IbisComponent::Check()
{
    return true;
}


class IbisModelSelectorEntry
{
public:
    wxString m_modelName;
    wxString m_modelDescription;
};

class IbisModelSelector
{
public:
    wxString                            m_name;
    std::vector<IbisModelSelectorEntry> m_models;

    bool Check();
};

bool IbisModelSelector::Check()
{
    return true;
}


class IVtableEntry
{
public:
    double V;
    TypMinMaxValue I;
};

class IVtable
{
public:
    std::vector<IVtableEntry> m_entries;
};

class VTtableEntry
{
public:
    double t;
    TypMinMaxValue V;
};

class VTtable
{
public:
    std::vector<VTtableEntry> m_entries;
};

/*
Model_type must be one of the following: 
Input, Output, I/O, 3-state, Open_drain, I/O_open_drain, Open_sink, I/O_open_sink, 
Open_source, I/O_open_source, Input_ECL, Output_ECL, I/O_ECL, 3-state_ECL, Terminator, 
Series, and Series_switch. 
*/

enum class IBIS_MODEL_TYPE
{
    UNDEFINED,
    INPUT,
    OUTPUT,
    IO,
    THREE_STATE,
    OPEN_DRAIN,
    IO_OPEN_DRAIN,
    OPEN_SINK,
    IO_OPEN_SINK,
    OPEN_SOURCE,
    IO_OPEN_SOURCE,
    INPUT_ECL,
    OUTPUT_ECL,
    IO_ECL,
    THREE_STATE_ECL,
    TERMINATOR,
    SERIES,
    SERIES_SWITCH
};

enum class IBIS_MODEL_ENABLE
{
    UNDEFINED,
    ACTIVE_HIGH,
    ACTIVE_LOW
};

class dvdt
{
public:
    double m_dv;
    double m_dt;
};

class dvdtTypMinMax
{
public:
    dvdt m_typ;
    dvdt m_min;
    dvdt m_max;
};

class IbisRamp
{
public:
    dvdtTypMinMax m_falling;
    dvdtTypMinMax m_rising;
    double        m_Rload;
};

enum class IBIS_WAVEFORM_TYPE
{
    RISING,
    FALLING
};

class IbisWaveform
{
public:
    VTtable            m_table;
    IBIS_WAVEFORM_TYPE m_type;
    double             m_R_fixture;
    double             m_C_fixture;
    double             m_L_fixture;
    double             m_V_fixture;
    double             m_V_fixture_min;
    double             m_V_fixture_max;
};

enum class IBIS_MODEL_POLARITY
{
    UNDEFINED,
    INVERTING,
    NON_INVERTING
};

class IbisModel
{
public:
    wxString          m_name;
    IBIS_MODEL_TYPE   m_type = IBIS_MODEL_TYPE::UNDEFINED;
    IBIS_MODEL_ENABLE m_enable = IBIS_MODEL_ENABLE::UNDEFINED;
    IBIS_MODEL_ENABLE m_polarity = IBIS_MODEL_ENABLE::UNDEFINED;
    double            m_vinl, m_vinh, m_vref, m_rref, m_cref, m_vmeas;
    TypMinMaxValue    m_C_comp;
    TypMinMaxValue    m_voltageRange;
    TypMinMaxValue    m_temperatureRange;
    IVtable m_GNDClamp;
    IVtable m_POWERClamp;
    IVtable           m_pullup;
    IVtable           m_pulldown;
    IbisWaveform      m_risingWaveform;
    IbisWaveform      m_fallingWaveform;
    IbisRamp          m_ramp;

    bool Check();
};

bool IbisModel::Check()
{
    return true;
}

bool IbisHeader::Check()
{
    bool status = true;

    if( m_ibisVersion == -1 )
    {
        std::cerr << "Missing [IBIS Ver]" << std::endl;
        status = false;
    }

    if( m_ibisVersion > IBIS_MAX_VERSION )
    {
        std::cerr << "Max IBIS version :" << IBIS_MAX_VERSION << " ( This file : " << m_ibisVersion
                  << " )" << std::endl;
        status = false;
    }

    if( m_ibisVersion == -1 )
    {
        std::cerr << "Missing [File Rev]" << std::endl;
        status = false;
    }

    if( m_fileName.IsEmpty() )
    {
        std::cerr << "Missing [File Name]" << std::endl;
        status = false;
    }

    if( !( m_fileName.EndsWith( ".ibs" ) || m_fileName.EndsWith( ".pkg" )
           || m_fileName.EndsWith( ".ebd" ) || m_fileName.EndsWith( ".ims" ) ) )
    {
        std::cerr << "[File Name]: Extension is not allowed" << std::endl;
    }


    return status;
}

class IbisFile
{
public:
    IbisHeader                 m_header;
    std::vector<IbisComponent> m_components;
    std::vector<IbisModelSelector> m_modelSelectors;
    std::vector<IbisModel>         m_models;
};


enum class IBIS_PARSER_CONTINUE
{
    NONE,
    STRING,
    COMPONENT_PACKAGE,
    COMPONENT_PINMAPPING,
    COMPONENT_DIESUPPLYPADS,
    COMPONENT_PIN,
    MODELSELECTOR,
    MODEL,
    IV_TABLE,
    VT_TABLE,
    RAMP,
    WAVEFORM
};

enum class IBIS_PARSER_CONTEXT
{
    HEADER,
    COMPONENT,
    MODELSELECTOR,
    MODEL,
    END
};

class IbisParser
{
public:
    long  m_lineCounter;
    char  m_commentChar = '|';
    char* m_buffer;
    int   m_bufferIndex;
    char* m_line;
    int   m_lineIndex;
    int   m_lineLength;

    IbisFile*      m_ibisFile;
    IbisComponent* m_currentComponent;
    IbisModelSelector* m_currentModelSelector;
    IbisModel*         m_currentModel;
    IVtable*           m_currentIVtable;
    VTtable*           m_currentVTtable;
    IbisWaveform*      m_currentWaveform;


    bool parseFile( wxFileName aFileName, IbisFile* );
    bool parseHeader( wxString );
    bool parseComponent( wxString );
    bool parseModelSelector( wxString );
    bool parseModel( wxString );
    bool parseDouble( double* aDest, wxString aStr, bool aAllowModifiers = false );

private:
    wxString* m_continuingString;


    bool onNewLine(); // Gets rid of comments ( except on a comment character change command...)
    bool GetNextLine();
    void PrintLine();

    void      skipWhitespaces();
    bool      checkEndofLine(); // To be used when there cannot be any character left on the line
    wxString* getKeyword();
    bool      readDouble( double* aDest );
    bool      readWord( wxString* );
    bool      readDvdt( wxString, dvdt* );
    bool      readRamp();
    bool      readWaveform( IbisWaveform* aDest, IBIS_WAVEFORM_TYPE aType );
    bool      readString( wxString* );
    bool      StoreString( wxString* aDest, bool aMultiline );
    std::vector<wxString> ReadTableLine();

    bool readNumericSubparam( wxString aSubparam, double* aDest );
    bool readIVtableEntry( IVtable* aTable );
    bool readVTtableEntry( VTtable* aTable );
    bool readTypMinMaxValue( TypMinMaxValue* aDest );
    bool readTypMinMaxValueSubparam( wxString aSubparam, TypMinMaxValue* aDest );
    //bool ReadDieSupplyPads();

    bool readPackage();
    bool readPin();
    bool readPinMapping();
    bool readModelSelector();
    bool readModel();


    bool ChangeCommentChar();
    bool changeContext( wxString aKeyword );

    IBIS_PARSER_CONTINUE m_continue = IBIS_PARSER_CONTINUE::NONE;
    IBIS_PARSER_CONTEXT  m_context = IBIS_PARSER_CONTEXT::HEADER;

    REPORTER* m_reporter = new STDOUT_REPORTER();
};

bool IbisParser::parseFile( wxFileName aFileName, IbisFile* aFile )
{
    m_ibisFile = aFile;
    wxString err_msg;

    std::ifstream ibisFile;
    ibisFile.open( aFileName.GetFullName() );
    if( ibisFile )
    {
        std::filebuf* pbuf = ibisFile.rdbuf();
        long          size = pbuf->pubseekoff( 0, ibisFile.end );
        pbuf->pubseekoff( 0, ibisFile.beg ); // rewind

        m_buffer = new char[size];
        pbuf->sgetn( m_buffer, size );

        m_lineCounter = 0;

        for( int i = 0; i < 4000; i++ )
        {
            if( !GetNextLine() )
            {
                m_reporter->Report( "Unexpected end of file. Missing [END] ?", RPT_SEVERITY_ERROR );
                return false;
            }
            PrintLine();
            if( !onNewLine() )
            {
                err_msg = wxString( "Error at line " );
                err_msg << m_lineCounter;
                m_reporter->Report( err_msg, RPT_SEVERITY_ERROR );
                return false;
            }
            if( m_context == IBIS_PARSER_CONTEXT::END )
            {
                return true;
            }
        }
    }
    else
    {
        err_msg = wxString( "Could not open file " );
        err_msg += aFileName.GetFullName();
        m_reporter->Report( err_msg, RPT_SEVERITY_ERROR );
    }
    return true;
}

void IbisParser::skipWhitespaces()
{
    while( ( isspace( m_line[m_lineIndex] ) ) && ( m_lineIndex < m_lineLength ) )
    {
        m_lineIndex++;
    }
}

bool IbisParser::checkEndofLine()
{
    skipWhitespaces();

    if( m_lineIndex < m_lineLength )
    {
        std::cerr << "There should not be any more data one that line..." << std::endl;
        return false;
    }
    return true;
}

bool IbisParser::readDvdt( wxString aString, dvdt* aDest )
{
    bool status = true;

    int i = 0;

    for( i = 1; i < aString.length(); i++ )
    {
        if( aString.at( i ) == '/' )
        {
            break;
        }
    }

    if( aString.at( i ) == '/' )
    {
        wxString str1 = aString.SubString( 0, i - 1 );
        wxString str2 = aString.SubString( i + 1, aString.size() );

        if( parseDouble( &( aDest->m_dv ), str1, true ) )
        {
            if( parseDouble( &( aDest->m_dt ), str2, true ) )
            {
            }
            else
            {
                status = false;
            }
        }
        else
        {
            status = false;
        }
    }
    else
    {
        status = false;
    }

    return status;
}


bool IbisParser::parseDouble( double* aDest, wxString aStr, bool aAllowModifiers )
{
    // "  an entry of the C matrix could be given as 1.23e-12 or as 1.23p or 1.23pF."
    skipWhitespaces();

    bool status = true;

    wxString str = aStr;

    double result;

    if( str == "NA" )
    {
        result = std::nan( "NA" );
    }
    else if( !str.ToDouble( &result ) )
    {
        if( aAllowModifiers )
        {
            int i;
            // Start at 1 because the first character cannot be the modifier
            for( i = 1; i < str.length(); i++ )
            {
                if( ( str.at( i ) == 'T' || str.at( i ) == 'G' || str.at( i ) == 'M'
                      || str.at( i ) == 'k' || str.at( i ) == 'm' || str.at( i ) == 'u'
                      || str.at( i ) == 'n' || str.at( i ) == 'p' || str.at( i ) == 'f' )
                    || std::isalpha(
                            str.at( i ) ) ) // Apparently some manufacturers could write "9.5V"
                {
                    break;
                }
            }
            wxString str2 = str.SubString( 0, i - 1 );

            if( !str2.ToDouble( &result ) )
            {
                status = false;
            }
            else
            {
                switch( static_cast<char>( str.at( i ) ) )
                {
                case 'T': result *= 1e12; break;
                case 'G': result *= 1e9; break;
                case 'M': result *= 1e6; break;
                case 'k': result *= 1e3; break;
                case 'm': result *= 1e-3; break;
                case 'u': result *= 1e-6; break;
                case 'n': result *= 1e-9; break;
                case 'p': result *= 1e-12; break;
                case 'f': result *= 1e-15; break;
                default:
                    break;
                    //std::cerr << "Internal error: could not apply modifier to double" << std::endl;
                    //status = false;
                }
            }
        }
        else
        {
            status = false;
        }
    }
    if( status == false )
    {
        result = std::nan( " " );
    }

    *aDest = result;

    return status;
}


bool IbisParser::GetNextLine()
{
    m_lineCounter++;

    long tmpIndex = m_bufferIndex;

    m_line = m_buffer + m_bufferIndex;

    char c = m_buffer[m_bufferIndex++];

    int i = 1;
    while( c != m_commentChar && c != 0 && c != '\n' && i < IBIS_MAX_LINE_LENGTH )
    {
        c = m_buffer[m_bufferIndex++];
        i++;
    }

    if( i == IBIS_MAX_LINE_LENGTH )
    {
        m_reporter->Report( "Line exceeds maximum length.", RPT_SEVERITY_ERROR );
        return false;
    }

    m_lineLength = m_bufferIndex - tmpIndex - 1; // Don't add the end of line condition
    m_lineIndex = 0;

    if( c == m_commentChar )
    {
        while( c != 0 && c != '\n' )
        {
            c = m_buffer[m_bufferIndex++];
        }
    }
    return true;
}


void IbisParser::PrintLine()
{
    for( int i = 0; i < m_lineLength; i++ )
    {
        std::cout << m_line[i];
    }
    std::cout << std::endl;
}

bool IbisParser::readDouble( double* aDest )
{
    bool status = true;
    wxString str;

    if ( readWord( &str ) )
    {
        if ( parseDouble( aDest, str,  true ) )
        {
            m_reporter->Report( "double: ok", RPT_SEVERITY_WARNING);   
        }
        else
        {
            m_reporter->Report( "Can't read double", RPT_SEVERITY_WARNING);
            status = false;
        }

    }
    else
    {
        m_reporter->Report( "Can't read word", RPT_SEVERITY_WARNING);
        status = false;
    }

    return status;
}

bool IbisParser::readWord( wxString* aDest )
{
    skipWhitespaces();

    wxString str;
    while( ( !isspace( m_line[m_lineIndex] ) ) && ( m_lineIndex < m_lineLength ) )
    {
        str += m_line[m_lineIndex++];
    }

    *( aDest ) = str;

    return ( str.size() > 0 );
}

bool IbisParser::readString( wxString* aDest )
{
    while( m_lineIndex < m_lineLength )
    {
        *aDest += m_line[m_lineIndex++];
    }

    // Remove extra whitespace characters
    aDest->Trim();

    return true;
}

bool IbisParser::StoreString( wxString* aDest, bool aMultiline )
{
    skipWhitespaces();

    readString( aDest );

    m_continue = aMultiline ? IBIS_PARSER_CONTINUE::STRING : IBIS_PARSER_CONTINUE::NONE;
    m_continuingString = aDest;

    return checkEndofLine();
}


bool IbisParser::ChangeCommentChar()
{
    skipWhitespaces();

    wxString strChar;

    // We cannot stop at m_lineLength here, because lineLength could stop before |_char
    // if the char is remains the same

    char c = m_line[m_lineIndex++];
    char d = c;

    if( !( c == '!' || c == '"' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\''
           || c == '(' || c == ')' || c == '*' || c == ',' || c == ':' || c == ';' || c == '<'
           || c == '>' || c == '?' || c == '@' || c == '\\' || c == '^' || c == '`' || c == '{'
           || c == '|' || c == '}' || c == '~' || c == ')' ) )
    {
        m_reporter->Report( "New comment character is invalid.", RPT_SEVERITY_ERROR );
    }

    c = m_line[m_lineIndex++];

    while( ( !isspace( c ) ) && c != 0 && c != '\n' )
    {
        strChar += c;
        c = m_line[m_lineIndex++];
    }

    if( !strChar.IsSameAs( wxString( "_char" ) ) )
    {
        m_reporter->Report( "Invalid syntax. Should be |_char or &_char, etc...",
                            RPT_SEVERITY_ERROR );
        return false;
    }

    int i = 0;
    while( isspace( c ) && c != 0 && c != '\n' && c != d )
    {
        c = m_line[m_lineIndex++];
    }

    if( ( !isspace( c ) ) && c != d )
    {
        m_reporter->Report( "No extra argument was expected", RPT_SEVERITY_ERROR );
        return false;
    }

    m_commentChar = d;


    m_continue = IBIS_PARSER_CONTINUE::NONE;

    return true;
}

wxString* IbisParser::getKeyword()
{
    //"Keywords must be enclosed in square brackets, “[]”, and must start in column 1 of the line."
    //"No space or tab is allowed immediately after the opening bracket “[” or immediately"
    // "before the closing bracket “]"

    if( m_line[m_lineIndex] != '[' )
    {
        return nullptr;
    }

    m_lineIndex++;

    wxString* keyword = new wxString( "" );

    char c;
    c = m_line[m_lineIndex++];

    while( ( c != ']' )
           && ( m_lineIndex
                < m_lineLength ) ) // We know the maximum keyword length, we could add a condition.
    {
        // "Underscores and spaces are equivalent in keywords"
        if( c == ' ' )
        {
            c = '_';
        }
        *keyword += c;
        c = m_line[m_lineIndex++];
    }

    return keyword;
}

bool IbisParser::changeContext( wxString aKeyword )
{
    bool status = true;

    // Old context;
    switch( m_context )
    {
    case IBIS_PARSER_CONTEXT::HEADER: m_ibisFile->m_header.Check(); break;
    case IBIS_PARSER_CONTEXT::COMPONENT: m_currentComponent->Check(); break;
    case IBIS_PARSER_CONTEXT::MODEL: m_currentModel->Check(); break;
    case IBIS_PARSER_CONTEXT::MODELSELECTOR: m_currentModel->Check(); break;
    case IBIS_PARSER_CONTEXT::END:
        m_reporter->Report( "Internal Error: Cannot change context after [END]" );
        break;

    default: m_reporter->Report( "Internal Error: Changing context from undefined context" );
    }

    if( aKeyword != "END" )
    {
        //New context
        if( aKeyword == "Component" )
        {
            IbisComponent comp;
            StoreString( &( comp.m_name ), false );
            m_ibisFile->m_components.push_back( comp );
            m_currentComponent = &( m_ibisFile->m_components.back() );
            m_context = IBIS_PARSER_CONTEXT::COMPONENT;
        }
        else if( aKeyword == "Model_Selector" )
        {
            IbisModelSelector MS;
            StoreString( &( MS.m_name ), false );
            m_ibisFile->m_modelSelectors.push_back( MS );
            m_currentModelSelector = &( m_ibisFile->m_modelSelectors.back() );
            m_context = IBIS_PARSER_CONTEXT::MODELSELECTOR;
            m_continue = IBIS_PARSER_CONTINUE::MODELSELECTOR;
        }
        else if( aKeyword == "Model" )
        {
            IbisModel model;
            StoreString( &( model.m_name ), false );
            m_ibisFile->m_models.push_back( model );
            m_currentModel = &( m_ibisFile->m_models.back() );
            m_context = IBIS_PARSER_CONTEXT::MODEL;
            m_continue = IBIS_PARSER_CONTINUE::MODEL;
        }
        else
        {
            status = false;
            wxString err_msg = wxString( "Unknown keyword in " );

            switch( m_context )
            {
            case IBIS_PARSER_CONTEXT::HEADER: err_msg += "HEADER";
            case IBIS_PARSER_CONTEXT::COMPONENT: err_msg += "COMPONENT";
            case IBIS_PARSER_CONTEXT::MODELSELECTOR: err_msg += "MODEL_SELECTOR";
            case IBIS_PARSER_CONTEXT::MODEL: err_msg += "MODEL";
            default: err_msg += "???";
            }

            err_msg += "context.";

            m_reporter->Report( err_msg, RPT_SEVERITY_ERROR );
        }
    }
    else
    {
        m_context = IBIS_PARSER_CONTEXT::END;
    }
    return status;
}


bool IbisParser::parseModelSelector( wxString aKeyword )
{
    bool status = true;

    if( !changeContext( aKeyword ) )
    {
        status = false;
    }
    return status;
}

bool IbisParser::readRamp()
{
    bool status = true;

    if( readNumericSubparam( wxString( "R_load" ), &( m_currentModel->m_ramp.m_Rload ) ) )
        ;
    else
    {
        wxString str;

        readWord( &str ); // skip a word

        m_continue = IBIS_PARSER_CONTINUE::RAMP;
        if( readWord( &str ) )
        {
            dvdtTypMinMax ramp;

            if( readDvdt( str, &( ramp.m_typ ) ) )
            {
                if( readDvdt( str, &( ramp.m_min ) ) )
                {
                    if( readDvdt( str, &( ramp.m_max ) ) )
                    {
                    }
                    else
                    {
                        status = false;
                    }
                }
                else
                {
                    status = false;
                }
            }
            else
            {
                status = false;
            }
        }
    }
    if( status == false )
    {
    }
    return status;
}


bool IbisParser::parseModel( wxString aKeyword )
{
    bool status = true;

    if( !aKeyword.CmpNoCase( "Voltage_Range" ) )
        readTypMinMaxValue( &( m_currentModel->m_voltageRange ) );
    else if( !aKeyword.CmpNoCase( "Temperature_Range" ) )
        readTypMinMaxValue( &( m_currentModel->m_temperatureRange ) );
    else if( !aKeyword.CmpNoCase( "GND_Clamp" ) )
        readIVtableEntry( &( m_currentModel->m_GNDClamp ) );
    else if( !aKeyword.CmpNoCase( "POWER_Clamp" ) )
        readIVtableEntry( &( m_currentModel->m_POWERClamp ) );
    else if( !aKeyword.CmpNoCase( "Pulldown" ) )
        readIVtableEntry( &( m_currentModel->m_pulldown ) );
    else if( !aKeyword.CmpNoCase( "Pullup" ) )
        readIVtableEntry( &( m_currentModel->m_pullup ) );
    else if( !aKeyword.CmpNoCase( "Rising_Waveform" ) )
        readWaveform( &( m_currentModel->m_risingWaveform ), IBIS_WAVEFORM_TYPE::RISING );
    else if( !aKeyword.CmpNoCase( "Falling_Waveform" ) )
        readWaveform( &( m_currentModel->m_fallingWaveform ), IBIS_WAVEFORM_TYPE::FALLING );
    else if( !aKeyword.CmpNoCase( "Ramp" ) )
        readRamp();

    else
    {
        if( !changeContext( aKeyword ) )
        {
            status = false;
        }
    }
    return status;
}


bool IbisParser::readModelSelector()
{
    bool status = true;

    IbisModelSelectorEntry model;

    if( readWord( &( model.m_modelName ) ) )
    {
        if( !readString( &( model.m_modelDescription ) ) )
        {
            status &= false;
        }
        m_currentModelSelector->m_models.push_back( model );
    }
    else
    {
        status = false;
    }
    return status;
}

bool IbisParser::readNumericSubparam( wxString aSubparam, double* aDest )
{
    wxString paramName;
    bool     status = true;

    m_lineIndex = 0;
    if( aSubparam.size() < m_lineLength )
    {
        for( int i = 0; i < aSubparam.size(); i++ )
        {
            paramName += m_line[m_lineIndex++];
        }

        if( paramName == aSubparam )
        {
            skipWhitespaces();
            if( m_line[m_lineIndex++] == '=' )
            {
                skipWhitespaces();

                wxString strValue;

                if( !StoreString( &strValue, false ) )
                {
                    status = false;
                }
                else
                {
                    if( !parseDouble( aDest, strValue, true ) )
                    {
                        status = false;
                    }
                }
            }
            else
            {
                status = false;
            }
        }
        else
        {
            status = false;
        }
    }
    else
    {
        status = false;
    }

    return status;
}


bool IbisParser::readTypMinMaxValue( TypMinMaxValue* aDest )
{
    bool status = true;

    skipWhitespaces();

    wxString strValue;


    if( !readWord( &strValue ) )
    {
        status = false;
    }
    else
    {
        if( parseDouble( &( ( *aDest ).typ ), strValue, true ) )
        {
            if( readWord( &strValue ) )
            {
                parseDouble( &( ( *aDest ).min ), strValue, true );
                if( readWord( &strValue ) )
                {
                    parseDouble( &( ( *aDest ).max ), strValue, true );
                }
            }
        }
        else
        {
            status = false;
            m_reporter->Report( "Typ-Min-Max Values requires at least Typ.", RPT_SEVERITY_ERROR );
        }
    }
    return status;
}

bool IbisParser::readTypMinMaxValueSubparam( wxString aSubparam, TypMinMaxValue* aDest )
{
    wxString paramName;
    bool     status = true;

    m_lineIndex = 0; // rewind

    if( aSubparam.size() < m_lineLength )
    {
        for( int i = 0; i < aSubparam.size(); i++ )
        {
            paramName += m_line[m_lineIndex++];
        }

        if( paramName == aSubparam )
        {
            readTypMinMaxValue( aDest );
        }
        else
        {
            status = false;
        }
    }
    else
    {
        status = false;
    }

    return status;
}

bool IbisParser::readModel()
{
    bool status = true;

    IbisModel model;

    int startOfLine = m_lineIndex;

    wxString subparam;
    if( readWord( &subparam ) )
    {
        switch( m_continue )
        {
        case IBIS_PARSER_CONTINUE::MODEL:
            if( subparam.SubString( 0, 9 ) == "Model_type" )
            {
                if( readWord( &subparam ) )
                {
                    if( subparam == "Input" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::INPUT;
                    else if( subparam == "Output" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OUTPUT;
                    else if( subparam == "I/O" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO;
                    else if( subparam == "3-state" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::THREE_STATE;
                    else if( subparam == "Open_drain" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OPEN_DRAIN;
                    else if( subparam == "I/O_Open_drain" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO_OPEN_DRAIN;
                    else if( subparam == "Open_sink" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OPEN_SINK;
                    else if( subparam == "I/O_open_sink" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO_OPEN_SINK;
                    else if( subparam == "Open_source" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OPEN_SOURCE;
                    else if( subparam == "I/O_open_source" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO_OPEN_SOURCE;
                    else if( subparam == "Input_ECL" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::INPUT_ECL;
                    else if( subparam == "Output_ECL" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OUTPUT_ECL;
                    else if( subparam == "I/O_ECL" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO_ECL;
                    else if( subparam == "3-state_ECL" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::THREE_STATE_ECL;
                    else if( subparam == "Terminator" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::TERMINATOR;
                    else if( subparam == "Series" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::SERIES;
                    else if( subparam == "Series_switch" )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::SERIES_SWITCH;
                    else
                    {
                        wxString err_msg = wxString( "Unknown Model_type: " );
                        err_msg += subparam;
                        m_reporter->Report( err_msg, RPT_SEVERITY_ERROR );
                        status = false;
                    }
                }
                else
                {
                    m_reporter->Report( "Internal Error while reading model_type",
                                        RPT_SEVERITY_ERROR );
                    status = false;
                }
            }
            else if( subparam.SubString( 0, 6 ) == "Enable" )
            {
                if( readWord( &subparam ) )
                {
                    if( subparam == "Active-High" )
                        m_currentModel->m_enable = IBIS_MODEL_ENABLE::ACTIVE_HIGH;
                    else if( subparam == "Active-Low" )
                        m_currentModel->m_enable = IBIS_MODEL_ENABLE::ACTIVE_LOW;
                    else
                    {
                        wxString err_msg = wxString( "Unknown Enable: " );
                        err_msg += subparam;
                        m_reporter->Report( err_msg, RPT_SEVERITY_ERROR );
                        status = false;
                    }
                }
                else
                {
                    m_reporter->Report( "Internal Error while reading Enable", RPT_SEVERITY_ERROR );
                    status = false;
                }
            }
            else if( subparam.SubString( 0, 8 ) == "Polarity" )
            {
                if( readWord( &subparam ) )
                {
                    if( subparam == "Inverting" )
                        m_currentModel->m_enable = IBIS_MODEL_ENABLE::ACTIVE_HIGH;
                    else if( subparam == "Non-Inverting" )
                        m_currentModel->m_enable = IBIS_MODEL_ENABLE::ACTIVE_LOW;
                    else
                    {
                        wxString err_msg = wxString( "Unknown Polarity: " );
                        err_msg += subparam;
                        m_reporter->Report( err_msg, RPT_SEVERITY_ERROR );
                        status = false;
                    }
                }
                else
                {
                    m_reporter->Report( "Internal Error while reading Enable", RPT_SEVERITY_ERROR );
                    status = false;
                }
            }
            else if( readNumericSubparam( wxString( "Vinl" ), &( m_currentModel->m_vinl ) ) )
                ;
            else if( readNumericSubparam( wxString( "Vinh" ), &( m_currentModel->m_vinh ) ) )
                ;
            else if( readNumericSubparam( wxString( "Vref" ), &( m_currentModel->m_vref ) ) )
                ;
            else if( readNumericSubparam( wxString( "Rref" ), &( m_currentModel->m_rref ) ) )
                ;
            else if( readNumericSubparam( wxString( "Cref" ), &( m_currentModel->m_cref ) ) )
                ;
            else if( readNumericSubparam( wxString( "Vmeas" ), &( m_currentModel->m_vmeas ) ) )
                ;
            else if( readTypMinMaxValueSubparam( wxString( "C_comp" ),
                                                 &( m_currentModel->m_C_comp ) ) )
                ;
            else
            {
                status = false;
            }

            m_continue = IBIS_PARSER_CONTINUE::MODEL;

            break;
        default:
            status = false;
            m_reporter->Report( "Internal error: continued reading a model that is not started.",
                                RPT_SEVERITY_ERROR );
        }
    }

    return status;
}


bool IbisParser::parseHeader( wxString aKeyword )
{
    bool status = true;

    if( !aKeyword.CmpNoCase( "IBIS_Ver" ) )
    {
        status &= readDouble( &( m_ibisFile->m_header.m_ibisVersion ) );
    }
    else if( !aKeyword.CmpNoCase( "Comment_char" ) )
    {
        ChangeCommentChar();
    }
    else if( !aKeyword.CmpNoCase( "File_Name" ) )
    {
        StoreString( &( m_ibisFile->m_header.m_fileName ), false );
    }
    else if( !aKeyword.CmpNoCase( "File_Rev" ) )
    {
        status &= readDouble( &( m_ibisFile->m_header.m_fileRevision) );
    }
    else if( !aKeyword.CmpNoCase( "Source" ) )
    {
        StoreString( &( m_ibisFile->m_header.m_source ), true );
    }
    else if( !aKeyword.CmpNoCase( "Notes" ) )
    {
        StoreString( &( m_ibisFile->m_header.m_notes ), true );
    }
    else if( !aKeyword.CmpNoCase( "Disclaimer" ) )
    {
        StoreString( &( m_ibisFile->m_header.m_disclaimer ), true );
    }
    else if( !aKeyword.CmpNoCase( "Copyright" ) )
    {
        StoreString( &( m_ibisFile->m_header.m_copyright ), true );
    }
    else if( !aKeyword.CmpNoCase( "Date" ) )
    {
        StoreString( &( m_ibisFile->m_header.m_date ), false );
    }
    else
    {
        if( !changeContext( aKeyword ) )
        {
            status = false;
        }
    }
    return status;
}


bool IbisParser::parseComponent( wxString aKeyword )
{
    bool status = true;

    if( aKeyword == "Manufacturer" )
    {
        status &= StoreString( &( m_currentComponent->m_manufacturer ), true );
    }
    else if( aKeyword == "Package" )
    {
        status &= readPackage();
    }
    else if( aKeyword == "Pin" )
    {
        status &= readPin();
    }
    else if( aKeyword == "Pin_Mapping" )
    {
        status &= readPinMapping();
    }
    /*
    else if( aKeyword == "Die_Supply_Pads" )
    {
        status &= ReadDieSupplyPads();
    }*/
    else if( aKeyword == "Package_Model" )
    {
        status &= StoreString( &( m_currentComponent->m_packageModel ), true );
    }
    else
    {
        if( !changeContext( aKeyword ) )
        {
            status = false;
        }
    }
    return status;
}

std::vector<wxString> IbisParser::ReadTableLine()
{
    std::vector<wxString> elements;
    while( m_lineIndex < m_lineLength )
    {
        wxString str;

        while( ( !isspace( m_line[m_lineIndex] ) ) && ( m_lineIndex < m_lineLength ) )
        {
            str += m_line[m_lineIndex++];
        }
        elements.push_back( str );
        while( isspace( m_line[m_lineIndex] ) && ( m_lineIndex < m_lineLength ) )
        {
            m_lineIndex++;
        }
    }
    return elements;
}

bool IbisParser::readPackage()
{
    bool status = true;

    std::vector<wxString> fields;

    fields = ReadTableLine();

    int extraArg = ( m_continue == IBIS_PARSER_CONTINUE::NONE ) ? 1 : 0;

    if( fields.size() == 4 )
    {
        if( fields.at( 0 ) == "R_pkg" )
        {
            if( parseDouble( &( m_currentComponent->m_package.m_Rpkg.typ ), fields.at( 1 ), true ) )
            {
                status = false;
            }

            parseDouble( &( m_currentComponent->m_package.m_Rpkg.min ), fields.at( 2 ), true );
            parseDouble( &( m_currentComponent->m_package.m_Rpkg.max ), fields.at( 3 ), true );
        }
        else if( fields.at( 0 ) == "L_pkg" )
        {
            if( parseDouble( &( m_currentComponent->m_package.m_Lpkg.typ ), fields.at( 1 ), true ) )
            {
                status = false;
            }

            parseDouble( &( m_currentComponent->m_package.m_Lpkg.min ), fields.at( 2 ), true );
            parseDouble( &( m_currentComponent->m_package.m_Lpkg.max ), fields.at( 3 ), true );
        }
        else if( fields.at( 0 ) == "C_pkg" )
        {
            if( parseDouble( &( m_currentComponent->m_package.m_Cpkg.typ ), fields.at( 1 ), true ) )
            {
                status = false;
            }

            parseDouble( &( m_currentComponent->m_package.m_Cpkg.min ), fields.at( 2 ), true );
            parseDouble( &( m_currentComponent->m_package.m_Cpkg.max ), fields.at( 3 ), true );
        }
    }
    else
    {
        if( fields.size() != 0 )
        {
            m_reporter->Report( "A [Package] line requires exactly 4 elements.",
                                RPT_SEVERITY_ERROR );
            std::cout << fields.size() << std::endl;
            status = false;
        }
    }
    m_continue = IBIS_PARSER_CONTINUE::COMPONENT_PACKAGE;

    return true;
}


bool IbisParser::readPin()
{
    bool status = true;

    std::vector<wxString> fields;

    fields = ReadTableLine();

    IbisComponentPin pin;

    if( ( fields.size() == 3 ) )
    {
        if( m_continue == IBIS_PARSER_CONTINUE::COMPONENT_PIN ) // No info on first line
        {
            pin.m_pinName = fields.at( 0 );
            pin.m_signalName = fields.at( 1 );
            pin.m_modelName = fields.at( 2 );
        }
        else
        {
            pin.m_virtual = true;
        }
    }
    else
    {
        if( m_continue == IBIS_PARSER_CONTINUE::COMPONENT_PIN ) // Not on the first line
        {
            pin.m_pinName = fields.at( 0 );
            pin.m_signalName = fields.at( 1 );
            pin.m_modelName = fields.at( 2 );

            pin.m_Rcol = m_currentComponent->m_pins.back().m_Rcol;
            pin.m_Lcol = m_currentComponent->m_pins.back().m_Lcol;
            pin.m_Ccol = m_currentComponent->m_pins.back().m_Ccol;

            if( pin.m_Rcol == 0 || pin.m_Lcol == 0 || pin.m_Ccol == 0 )
            {
                status = false; // Did we just try to go from a 3 column table to a 6 ?
            }
            else
            {
                parseDouble( &( pin.m_Rpin.typ ), fields.at( pin.m_Rcol ), true );
                parseDouble( &( pin.m_Lpin.typ ), fields.at( pin.m_Lcol ), true );
                parseDouble( &( pin.m_Cpin.typ ), fields.at( pin.m_Ccol ), true );
            }
        }
        else
        {
            for( int i = 3; i < 6; i++ )
            {
                if( fields.at( i ) == "R_pin" )
                {
                    pin.m_Rcol = i;
                }
                else if( fields.at( i ) == "L_pin" )
                {
                    pin.m_Lcol = i;
                }
                else if( fields.at( i ) == "C_pin" )
                {
                    pin.m_Ccol = i;
                }
                else
                {
                    m_reporter->Report( "[Pin]: Invalid field name", RPT_SEVERITY_ERROR );
                    status = false;
                }
            }

            if( pin.m_Rcol == 0 || pin.m_Lcol == 0 || pin.m_Ccol == 0 )
            {
                m_reporter->Report( "[Pin]: Missing argument", RPT_SEVERITY_ERROR );
                status = false;
            }
            pin.m_virtual = true;
        }
    }
    m_continue = IBIS_PARSER_CONTINUE::COMPONENT_PIN;
    m_currentComponent->m_pins.push_back( pin );

    return true;
}


bool IbisParser::readPinMapping()
{
    bool status = true;

    std::vector<wxString> fields;

    fields = ReadTableLine();

    IbisComponentPinMapping pinMapping;

    if( m_continue == IBIS_PARSER_CONTINUE::NONE ) // No info on first line
    {
        m_continue = IBIS_PARSER_CONTINUE::COMPONENT_PINMAPPING;
    }
    else
    {
        if( fields.size() != 0 )
        {
            if( fields.size() > 6 && fields.size() < 3 )
            {
                m_reporter->Report( "[Pin Mapping]: wrong number of columns", RPT_SEVERITY_ERROR );
                status = false;
            }
            else
            {
                pinMapping.m_pinName = fields.at( 0 );
                pinMapping.m_PDref = fields.at( 1 );
                pinMapping.m_PUref = fields.at( 2 );
                if( fields.size() > 3 )
                    pinMapping.m_GNDClampRef = fields.at( 3 );
                if( fields.size() > 4 )
                    pinMapping.m_POWERClampRef = fields.at( 4 );
                if( fields.size() > 5 )
                    pinMapping.m_extRef = fields.at( 5 );
            }
            m_currentComponent->m_pinMappings.push_back( pinMapping );
        }
    }
    return status;
}


bool IbisParser::readIVtableEntry( IVtable* aDest )
{
    bool status = true;

    skipWhitespaces();

    IVtableEntry entry;

    if( m_lineIndex < m_lineLength )
    {
        wxString str;
        if( readWord( &str ) )
        {
            if( parseDouble( &( entry.V ), str, true ) )
            {
                if( readTypMinMaxValue( &( entry.I ) ) )
                {
                }
                else
                {
                    status = false;
                }
            }
            else
            {
                status = false;
            }
        }
        else
        {
            status = false;
        }
    }

    m_continue = IBIS_PARSER_CONTINUE::IV_TABLE;
    m_currentIVtable = aDest;

    if( status )
    {
        aDest->m_entries.push_back( entry );
    }

    return status;
}

bool IbisParser::readVTtableEntry( VTtable* aDest )
{
    bool status = true;
    skipWhitespaces();

    VTtableEntry entry;

    if( m_lineIndex < m_lineLength )
    {
        wxString str;
        if( readWord( &str ) )
        {
            if( parseDouble( &( entry.t ), str, true ) )
            {
                if( readTypMinMaxValue( &( entry.V ) ) )
                {
                }
                else
                {
                    status = false;
                }
            }
            else
            {
                status = false;
            }
        }
        else
        {
            status = false;
        }
    }

    m_continue = IBIS_PARSER_CONTINUE::IV_TABLE;
    m_currentVTtable = aDest;

    if( status )
    {
        aDest->m_entries.push_back( entry );
    }
    else
    {
    }


    return status;
}

bool IbisParser::readWaveform( IbisWaveform* aDest, IBIS_WAVEFORM_TYPE aType )
{
    bool status = true;

    aDest->m_type = aType;

    IbisWaveform* wf;

    switch( aType )
    {
    case IBIS_WAVEFORM_TYPE::FALLING: wf = &( m_currentModel->m_fallingWaveform ); break;
    case IBIS_WAVEFORM_TYPE::RISING: wf = &( m_currentModel->m_risingWaveform ); break;
    default: m_reporter->Report( "Unknown waveform type", RPT_SEVERITY_ERROR );
    }
    m_currentWaveform = wf;
    if( status )
    {
        if( readNumericSubparam( wxString( "R_fixture" ), &( wf->m_R_fixture ) ) )
            ;
        else if( readNumericSubparam( wxString( "L_fixture" ), &( wf->m_L_fixture ) ) )
            ;
        else if( readNumericSubparam( wxString( "C_fixture" ), &( wf->m_C_fixture ) ) )
            ;
        else if( readNumericSubparam( wxString( "V_fixture" ), &( wf->m_V_fixture ) ) )
            ;
        else if( readNumericSubparam( wxString( "V_fixture_min" ), &( wf->m_V_fixture_min ) ) )
            ;
        else if( readNumericSubparam( wxString( "V_fixture_max" ), &( wf->m_V_fixture_max ) ) )
            ;
        else
        {
            VTtableEntry entry;

            if( readVTtableEntry( &m_currentWaveform->m_table ) )
            {
            }
            else
            {
                status = false;
            }
        }
    }

    m_continue = IBIS_PARSER_CONTINUE::WAVEFORM;
    return status;
}

bool IbisParser::onNewLine()
{
    bool      status = true;
    char      c;
    wxString* pKeyword = getKeyword();
    // New line

    if( pKeyword ) // New keyword
    {
        wxString keyword = *pKeyword;

        if( m_continue != IBIS_PARSER_CONTINUE::NONE )
        {
            m_continue = IBIS_PARSER_CONTINUE::NONE;
        }
        switch( m_context )
        {
        case IBIS_PARSER_CONTEXT::HEADER: status &= parseHeader( keyword ); break;
        case IBIS_PARSER_CONTEXT::COMPONENT: status &= parseComponent( keyword ); break;
        case IBIS_PARSER_CONTEXT::MODELSELECTOR: status &= parseModelSelector( keyword ); break;
        case IBIS_PARSER_CONTEXT::MODEL: status &= parseModel( keyword ); break;
        default: m_reporter->Report( "Internal error: Bad parser context.", RPT_SEVERITY_ERROR );
        }
    }
    else
    {
        skipWhitespaces();
        if( m_lineIndex == m_lineLength )
        {
            // That was an empty line
            return true;
        }

        // No new keyword ? Then it is the continuatino of the previous one !
        switch( m_continue )
        {
        case IBIS_PARSER_CONTINUE::STRING:
            skipWhitespaces();
            *m_continuingString += '\n';
            readString( m_continuingString );
            break;
        case IBIS_PARSER_CONTINUE::COMPONENT_PACKAGE: status &= readPackage(); break;
        case IBIS_PARSER_CONTINUE::COMPONENT_PIN: status &= readPin(); break;
        case IBIS_PARSER_CONTINUE::COMPONENT_PINMAPPING: status &= readPinMapping(); break;
        case IBIS_PARSER_CONTINUE::MODELSELECTOR: status &= readModelSelector(); break;
        case IBIS_PARSER_CONTINUE::MODEL: status &= readModel(); break;
        case IBIS_PARSER_CONTINUE::IV_TABLE: status &= readIVtableEntry( m_currentIVtable ); break;
        case IBIS_PARSER_CONTINUE::VT_TABLE: status &= readVTtableEntry( m_currentVTtable ); break;
        case IBIS_PARSER_CONTINUE::WAVEFORM:
            status &= readWaveform( m_currentWaveform, m_currentWaveform->m_type );
            break;
        case IBIS_PARSER_CONTINUE::RAMP: status &= readRamp(); break;
        case IBIS_PARSER_CONTINUE::NONE:
        default:
            m_reporter->Report( "Missing keyword.", RPT_SEVERITY_ERROR );
            return false;
            break;
        }
    }
    c = m_line[m_lineIndex];

    while( ( c != '\n' ) && ( c != 0 ) ) // Go to the end of line
    {
        c = m_line[m_lineIndex++];
    }
    return status;
}