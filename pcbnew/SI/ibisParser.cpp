#include <wx/string.h>
#include <wx/filename.h>
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

    bool CheckHeader();
};

class IbisTypMinMaxValue
{
public:
    double min = std::nan( " " );
    double typ = std::nan( " " );
    double max = std::nan( " " );
};

class IbisComponentPackage
{
    IbisTypMinMaxValue m_Rpkg;
    IbisTypMinMaxValue m_Lpkg;
    IbisTypMinMaxValue m_Cpkg;
};

class IbisComponent
{
public:
    wxString             m_name;
    wxString             m_manufacturer;
    IbisComponentPackage m_package;
    wxString             m_pin;
    wxString             m_mapping;
    wxString             m_busLabel;
    wxString             m_dieSupplyPads;
    wxString             m_diffPins;
};

bool IbisHeader::CheckHeader()
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
};


enum class IBIS_PARSER_CONTINUE
{
    NONE,
    STRING,
    COMPONENT_PACKAGE,
    IV_TABLE,
    VT_TABLE
};

enum class IBIS_PARSER_CONTEXT
{
    HEADER,
    COMPONENT
};

class IbisParser
{
public:
    char  m_commentChar = '|';
    char* m_buffer;
    int   m_bufferIndex;
    char* m_line;
    int   m_lineIndex;
    int   m_lineLength;

    IbisFile*      m_ibisFile;
    IbisComponent* m_currentComponent;

    bool parseFile( wxFileName aFileName, IbisFile* );
    bool parseHeader( wxString );
    bool parseComponent( wxString );
    bool parseDouble( double* aDest, bool aAllowModifiers = false );

private:
    wxString* m_continuingString;


    bool onNewLine(); // Gets rid of comments ( except on a comment character change command...)
    bool GetNextLine();
    void PrintLine();

    void      skipWhitespaces();
    bool      checkEndofLine(); // To be used when there cannot be any character left on the line
    wxString* getKeyword();
    bool      readDouble( double* );
    bool      readString( wxString* );
    bool      StoreString( wxString* aDest, bool aMultiline );
    std::vector<wxString> ReadTableLine();

    bool ReadIbisVersion();
    bool ReadFileName();
    bool ReadFileRev();
    bool ReadSource();
    bool ReadNotes();
    bool ReadDisclaimer();
    bool ReadCopyright();

    bool readPackage();


    bool ReadManufacturer();


    bool ChangeCommentChar();
    bool changeContext( wxString aKeyword );

    IBIS_PARSER_CONTINUE m_continue = IBIS_PARSER_CONTINUE::NONE;
    IBIS_PARSER_CONTEXT  m_context = IBIS_PARSER_CONTEXT::HEADER;
};

bool IbisParser::parseFile( wxFileName aFileName, IbisFile* aFile )
{
    m_ibisFile = aFile;

    std::ifstream ibisFile;
    ibisFile.open( aFileName.GetFullName() );
    if( ibisFile )
    {
        std::filebuf* pbuf = ibisFile.rdbuf();
        long          size = pbuf->pubseekoff( 0, ibisFile.end );
        pbuf->pubseekoff( 0, ibisFile.beg ); // rewind

        std::cout << size << std::endl;
        m_buffer = new char[size];
        pbuf->sgetn( m_buffer, size );

        for( int i = 0; i < 100; i++ )
        {
            if( !GetNextLine() )
            {
                return false;
            }
            PrintLine();
            if( !onNewLine() )
            {
                return false;
            }
        }
    }
    else
    {
        std::cout << "Could not open file " << aFileName.GetFullName() << std::endl;
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


bool parseDouble( double* aDest, bool aAllowModifiers )
{
    // "  an entry of the C matrix could be given as 1.23e-12 or as 1.23p or 1.23pF."
    skipWhitespaces();

    wxString str;
    while( !( isspace( m_line[m_lineIndex] ) ) && m_lineIndex < m_lineLength )
    {
        str += m_line[m_lineIndex++];
    }

    double = result;
    if( !str.ToString( &result ) )
    {
    }
}


bool IbisParser::GetNextLine()
{
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
        std::cerr << "Line too long" << std::endl;
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
    wxString str;
    while( ( !isspace( m_line[m_lineIndex] ) ) && ( m_lineIndex < m_lineLength ) )
    {
        str += m_line[m_lineIndex++];
    }

    return str.ToDouble( aDest );
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

bool IbisParser::ReadIbisVersion()
{
    skipWhitespaces();

    if( !readDouble( &( m_ibisFile->m_header.m_ibisVersion ) ) )
    {
        std::cerr << "Invalid Ibis version format." << std::endl;
        return false;
    }

    m_continue = IBIS_PARSER_CONTINUE::NONE;

    return checkEndofLine();
}

bool IbisParser::ReadFileRev()
{
    skipWhitespaces();

    if( !readDouble( &( m_ibisFile->m_header.m_fileRevision ) ) )
    {
        std::cerr << "Invalid file revision Format." << std::endl;
        return false;
    }

    m_continue = IBIS_PARSER_CONTINUE::NONE;

    return checkEndofLine();
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
        std::cerr << "New comment char is not valid" << std::endl;
    }

    c = m_line[m_lineIndex++];

    while( ( !isspace( c ) ) && c != 0 && c != '\n' )
    {
        strChar += c;
        c = m_line[m_lineIndex++];
    }

    if( !strChar.IsSameAs( wxString( "_char" ) ) )
    {
        std::cerr << "Invalid syntax. Should be |_char or &_char, etc..." << std::endl;
        return false;
    }

    int i = 0;
    while( isspace( c ) && c != 0 && c != '\n' && c != d )
    {
        c = m_line[m_lineIndex++];
    }

    if( ( !isspace( c ) ) && c != d )
    {
        std::cerr << "No extra argument was expected" << std::endl;
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

    switch( m_context )
    {
    case ::IBIS_PARSER_CONTEXT::HEADER:

        if( aKeyword == "Component" )
        {
            IbisComponent comp;
            StoreString( &( comp.m_name ), false );
            m_ibisFile->m_components.push_back( comp );
            m_currentComponent = &( m_ibisFile->m_components.back() );
            m_context = IBIS_PARSER_CONTEXT::COMPONENT;
        }
        else
            status = false;
        break;
    default: status = false;
    }
    return status;
}

bool IbisParser::parseHeader( wxString aKeyword )
{
    bool status = true;

    if( aKeyword == "IBIS_Ver" )
    {
        ReadIbisVersion();
    }
    else if( aKeyword == "Comment_char" )
    {
        ChangeCommentChar();
    }
    else if( aKeyword == "File_Name" )
    {
        StoreString( &( m_ibisFile->m_header.m_fileName ), false );
    }
    else if( aKeyword == "File_Rev" )
    {
        ReadFileRev();
    }
    else if( aKeyword == "Source" )
    {
        StoreString( &( m_ibisFile->m_header.m_source ), false );
    }
    else if( aKeyword == "Notes" )
    {
        StoreString( &( m_ibisFile->m_header.m_notes ), true );
    }
    else if( aKeyword == "Disclaimer" )
    {
        StoreString( &( m_ibisFile->m_header.m_disclaimer ), true );
    }
    else if( aKeyword == "Copyright" )
    {
        StoreString( &( m_ibisFile->m_header.m_copyright ), true );
    }
    else if( aKeyword == "Date" )
    {
        StoreString( &( m_ibisFile->m_header.m_date ), false );
    }
    else
    {
        if( !changeContext( aKeyword ) )
        {
            std::cerr << "unknwon keyword in HEADER context." << std::endl;
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
        StoreString( &( m_currentComponent->m_manufacturer ), true );
    }
    else if( aKeyword == "Package" )
    {
        readPackage();
    }
    else if( aKeyword == "File_Name" )
    {
        StoreString( &( m_ibisFile->m_header.m_fileName ), false );
    }
    else if( aKeyword == "File_Rev" )
    {
        ReadFileRev();
    }
    else if( aKeyword == "Source" )
    {
        StoreString( &( m_ibisFile->m_header.m_notes ), false );
    }
    else if( aKeyword == "Notes" )
    {
        StoreString( &( m_ibisFile->m_header.m_notes ), true );
    }
    else if( aKeyword == "Disclaimer" )
    {
        StoreString( &( m_ibisFile->m_header.m_disclaimer ), true );
    }
    else if( aKeyword == "Copyright" )
    {
        StoreString( &( m_ibisFile->m_header.m_copyright ), true );
    }
    else if( aKeyword == "Date" )
    {
        StoreString( &( m_ibisFile->m_header.m_date ), false );
    }
    else
    {
        if( !changeContext( aKeyword ) )
        {
            std::cerr << "unknwon keyword in COMPONENT context." << std::endl;
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
    std::vector<wxString> fields;

    fields = ReadTableLine();

    if( fields.at( 0 ) == "R_pkg" )
    {
        std::cout << "Resistance" << std::endl;
    }
    else if( fields.at( 0 ) == "L_pkg" )
    {
        std::cout << "Inductance" << std::endl;
    }
    else if( fields.at( 0 ) == "C_pkg" )
    {
        std::cout << "Capacitance" << std::endl;
    }
    m_continue = IBIS_PARSER_CONTINUE::COMPONENT_PACKAGE;

    return true;
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
            std::cout << *m_continuingString << std::endl;
        }

        switch( m_context )
        {
        case IBIS_PARSER_CONTEXT::HEADER: status &= parseHeader( keyword ); break;
        case IBIS_PARSER_CONTEXT::COMPONENT: status &= parseComponent( keyword ); break;
        default: std::cerr << "Internal error: Bad parser context." << std::endl; status = false;
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
        case IBIS_PARSER_CONTINUE::COMPONENT_PACKAGE: readPackage(); break;
        case IBIS_PARSER_CONTINUE::NONE:
        default:
            std::cerr << "Missing keyword" << std::endl;
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