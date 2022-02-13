#include "ibisParser.h"
#include <sstream>
/** To be removed
 *  We want to remove the wxString dependency if this files is to get into ngspice.
 *  KiCad reporter class uses wxString.
 *  To remove it easily in the future, we use this function....
 */
void IbisParser::ibisReport( std::string aMsg, SEVERITY aSeverity )
{
    //ibisReport( wxString( aMsg ), aSeverity );
    std::cout << aMsg << std::endl;
}

bool IbisParser::ibisToDouble( std::string aString, double* aDest )
{
    //Apparently stod does not check if there is an alpha character at the end.
    //This would prevent us for getting m,u,n,k,... modifiers
    for ( char c: aString )
    {
        if ( ! ( ( ( c >= '0' ) && ( c <= '9' ) ) || c == '.' || c== 'e' || c== '+' || c=='-' ) )
        {
            return false;
        }
    }
    try
    {
        *aDest = std::stod( aString );
    }
    catch (...)
    {
        ibisReport( "Cannot convert double value" );
        return false;
    }
    return true;
}

bool IBIS_MATRIX_BANDED::Check()
{
    bool status = true;

    if( m_dim < 1 )
    {
        std::cout << "Matrix Banded: dimension should be > 1 " << std::endl;
        status = false;
    }

    if( m_bandwidth < 1 )
    {
        std::cout << "Matrix Banded: bandwidth should be > 1 " << std::endl;
        status = false;
    }

    if( m_data == nullptr )
    {
        std::cout << "Matrix Banded: data array is uninitizalized " << std::endl;
        status = false;
    }
    else
    {
        for( int i = 0; i < m_bandwidth * m_dim; i++ )
        {
            if( std::isnan( m_data[i] ) )
            {
                std::cout << "Matrix Banded: an element is NaN " << std::endl;
                status = false;
            }
        }
    }


    return status;
}

bool IBIS_MATRIX_FULL::Check()
{
    bool status = true;

    if( m_dim < 1 )
    {
        std::cout << "Matrix Full: dimension should be > 1 " << std::endl;
        status = false;
    }

    if( m_data == nullptr )
    {
        std::cout << "Matrix Full: data array is uninitizalized " << std::endl;
        status = false;
    }
    else
    {
        for( int i = 0; i < m_dim * m_dim; i++ )
        {
            if( std::isnan( m_data[i] ) )
            {
                std::cout << "Matrix Full: an element is NaN " << std::endl;
                status = false;
            }
        }
    }


    return status;
}

bool IBIS_MATRIX_SPARSE::Check()
{
    bool status = true;

    if( m_dim < 1 )
    {
        std::cout << "Matrix Full: dimension should be > 1 " << std::endl;
        status = false;
    }

    if( m_data == nullptr )
    {
        std::cout << "Matrix Full: data array is uninitizalized " << std::endl;
        status = false;
    }
    else
    {
        for( int i = 0; i < m_dim * m_dim; i++ )
        {
            if( std::isnan( m_data[i] ) )
            {
                std::cout << "Matrix Full: an element is NaN " << std::endl;
                status = false;
            }
        }
    }


    return status;
}

bool isNumberNA( double aNumber )
{
    bool result = false;

    double NA = std::nan( NAN_NA );

    std::uint64_t resultBinNA;
    std::memcpy( &resultBinNA, &( NA ), sizeof NA );
    std::uint64_t resultBin;
    std::memcpy( &resultBin, &( aNumber ), sizeof aNumber );

    return resultBin == resultBinNA;
}

bool TypMinMaxValue::Check()
{
    bool status = true;

    if( std::isnan( value[IBIS_CORNER::TYP] ) )
        status = false;
    if( std::isnan( value[IBIS_CORNER::MIN] ) && !isNumberNA( value[IBIS_CORNER::MIN] ) )
        status = false;
    if( std::isnan( value[IBIS_CORNER::MAX] ) && !isNumberNA( value[IBIS_CORNER::MAX] ) )
        status = false;

    return status;
}

bool IbisComponentPackage::Check()
{
    bool status =  true;

    if( !m_Rpkg.Check() )
    {
        std::cerr << "Package: Invalid R_pkg" << std::endl;
        status = false;
    }
    if( !m_Lpkg.Check() )
    {
        std::cerr << "Package: Invalid L_pkg" << std::endl;
        status = false;
    }
    if( !m_Cpkg.Check() )
    {
        std::cerr << "Package: Invalid C_pkg" << std::endl;
        status = false;
    }

    return status;
}

bool IbisComponentPin::Check()
{
    bool status = true;
    if ( !m_dummy )
    {

        if( m_pinName.empty() )
        {
            std::cerr << "Pin: pin name cannot be empty." << std::endl;
            status = false;
        }
        if( m_signalName.empty() )
        {
            std::cerr << "Pin: signal name cannot be empty." << std::endl;
            status = false;
        }
        if( m_modelName.empty() )
        {
            std::cerr << "Pin: model name cannot be empty." << std::endl;
            status = false;
        }
        if( std::isnan( m_Rpin ) && !isNumberNA( m_Rpin ) )
        {
            std::cerr << "Pin: Rpin is not valid." << std::endl;
            status = false;
        }
        if( std::isnan( m_Lpin )&& !isNumberNA( m_Lpin )  )
        {
            std::cerr << "Pin: Lpin is not valid." << std::endl;
            status = false;
        }
        if( std::isnan( m_Cpin )&& !isNumberNA( m_Cpin ) )
        {
            std::cerr << "Pin: Cpin is not valid." << std::endl;
            status = false;
        }
    }
    return status;
}


bool IbisComponent::Check()
{
    bool status = true;

    if( m_name.empty() )
    {
        std::cerr << "Component: name cannot be empty." << std::endl;
        status = false;
    }

    std::cout << "Checking component " << m_name << "..." << std::endl;

    if( m_manufacturer.empty() )
    {
        std::cerr << "Component: manufacturer cannot be empty." << std::endl;
        status = false;
    }

    if ( !m_package.Check() )
    {
        std::cerr << "Component: Invalid Package." << std::endl;
        status = false;        
    }
    
    if ( m_pins.size() < 1 )
    {
        std::cerr << "Component: no pin" << std::endl;    
        status = false;
    }

    for ( IbisComponentPin pin: m_pins )
    {
        status &= pin.Check();
    }

    return status;
}


bool IbisModelSelector::Check()
{
    return true;
}

std::string doubleToStringParser( double aNumber )
{
    std::ostringstream ss;
    ss.setf( std::ios_base::scientific, std::ios_base::floatfield );
    ss << aNumber;
    return ss.str();
}

std::string IVtable::Spice( int aN, std::string aPort1, std::string aPort2, std::string aModelName,
                         IBIS_CORNER aCorner )
{
    std::string result = "";

    if( m_entries.size() > 0 )
    {
        result += "a";
        result += std::to_string( aN );
        result += " %vd(";
        result += aPort1;
        result += " ";
        result += aPort2;
        result += ") %id(";
        result += aPort1;
        result += " ";
        result += aPort2;
        result += ") ";
        result += aModelName;
        result += "\n";
        result += "\n";
        result += ".model ";
        result += aModelName;
        result += " pwl(\n+ x_array=[";
        for( auto entry : m_entries )
        {
            result += doubleToStringParser( entry.V );
            result += " ";
        }
        result += "]\n+ y_array=[";
        for( auto entry : m_entries )
        {
            result += doubleToStringParser( entry.I.value[aCorner] );
            result += " ";
        }
        result += "]\n+ input_domain=0.05 fraction=TRUE)\n\n";
    }
    return result;
}

double IVtable::InterpolatedI( double aV, IBIS_CORNER aCorner )
{
    double result;
    // @TODO change this algorithm

    if( m_entries.at( m_entries.size() - 1 ).V > m_entries.at( 0 ).V )
    {
        if( aV >= m_entries.at( m_entries.size() - 1 ).V )
        {
            return m_entries.at( m_entries.size() - 1 ).I.value[aCorner];
        }

        if( aV <= m_entries.at( 0 ).V )
        {
            return m_entries.at( 0 ).I.value[aCorner];
        }

        for( int i = 1; i < m_entries.size(); i++ )
        {
            if( m_entries.at( i ).V > aV )
            {
                return m_entries.at( i - 1 ).I.value[aCorner]
                       + ( m_entries.at( i ).I.value[aCorner]
                           - m_entries.at( i - 1 ).I.value[aCorner] )
                                 / ( m_entries.at( i ).V - m_entries.at( i - 1 ).V )
                                 * ( aV - m_entries.at( i - 1 ).V );
            }
        }

        std::cout << "ERROR: non monotonic data ? " << std::endl;
        return 0;
    }
    else
    {
        std::cout << "ERROR: I guess I need to implement that... " << std::endl;
        return 0;
    }
    // @TODO prefer another method such as a dichotomy
}

bool IVtable::Check()
{
    bool status = true;
    for ( IVtableEntry entry : m_entries )
    {
        if ( std::isnan( entry.V ) )
        {
            std::cerr << "IV table: voltage is nan." << std::endl;
            status = false;
            break;
        }

        if ( !entry.I.Check() )
        {
            std::cerr << "IV table: current is incorrect" << std::endl;
            status = false;
            break;
        }
    }
    // TODO: Check if the IV table is monotonic :
    // IBIS standard defines 8 criteria for an IV table to be monotonic
    // Issue a warning, not an error

    return status;
}


bool dvdtTypMinMax::Check()
{
    bool status = true;

    if( std::isnan( value[IBIS_CORNER::TYP].m_dv ) )
        status = false;
    if( std::isnan( value[IBIS_CORNER::TYP].m_dt ) )
        status = false;


    if( std::isnan( value[IBIS_CORNER::MIN].m_dv ) && !isNumberNA( value[IBIS_CORNER::MIN].m_dv ) )
        status = false;
    if( std::isnan( value[IBIS_CORNER::MIN].m_dt ) && !isNumberNA( value[IBIS_CORNER::MIN].m_dt ) )
        status = false;

    if( std::isnan( value[IBIS_CORNER::MIN].m_dv ) && !isNumberNA( value[IBIS_CORNER::MIN].m_dv ) )
        status = false;
    if( std::isnan( value[IBIS_CORNER::MIN].m_dt ) && !isNumberNA( value[IBIS_CORNER::MIN].m_dt ) )
        status = false;

    if ( !status )
    {
        std::cout << "dv/dt: Incorrect dv/dt" << std::endl;
    }

    return status;
}

bool IbisRamp::Check()
{
    bool status = true;

    if ( std::isnan( m_Rload ) )
    {
        status = false;
        std::cerr << "Ramp: Invalid R_load" << std::endl;
    }
    if ( !m_falling.Check() )
    {
        std::cerr << "Ramp: falling invalid" << std::endl;
        status = false;
    }
    if ( !m_rising.Check() )
    {
        std::cerr << "Ramp: rising invalid" << std::endl;
        status = false;
    }

    return status;
}



bool IbisModel::Check()
{
    bool status = true;

    if( m_name.empty() )
    {
        std::cerr << "Model: model name cannot be empty" << std::endl;
        status = false;
    }

    std::cout << "Checking model " << m_name << "..." << std::endl;
    if (m_type == IBIS_MODEL_TYPE::UNDEFINED)
    {
        std::cerr << "Model: Undefined model type" << std::endl;
        status = false;
    }

    if ( isnan( m_vinh ) && !isNumberNA( m_vinh ) )
    {
        std::cerr << "Model: Vinh is invalid." << std::endl;
        status = false;
    }
    if ( isnan( m_vinl ) && !isNumberNA( m_vinl ) )
    {
        std::cerr << "Model: Vinh is invalid." << std::endl;
        status = false;
    }
    if ( isnan( m_rref ) && !isNumberNA( m_rref ) )
    {
        std::cerr << "Model: R_ref is invalid." << std::endl;
        status = false;
    }
    if ( isnan( m_cref ) && !isNumberNA( m_cref ) )
    {
        std::cerr << "Model: C_ref is invalid." << std::endl;
        status = false;
    }
    if ( isnan( m_vref ) && !isNumberNA( m_vref ) )
    {
        std::cerr << "Model: V_ref is invalid." << std::endl;
        status = false;
    }
    if ( isnan( m_vmeas ) && !isNumberNA( m_vmeas ) )
    {
        std::cerr << "Model: V_meas is invalid." << std::endl;
        status = false;
    }
    if ( !m_C_comp.Check() )
    {
        std::cerr << "Model: C_comp is invalid." << std::endl;
        status = false;
    }
    if ( !m_temperatureRange.Check() )
    {
        std::cerr << "Model: Temperature Range is invalid." << std::endl;
        status = false;
    }

    if ( !m_voltageRange.Check() )
    {
        // If the voltage range is not valid, it's ok, only if we have pulls and clamps
        if ( !m_pulldownReference.Check() )
        {
            status = false;
        }
        if ( !m_pullupReference.Check() )
        {
            status = false;
        }
        if ( !m_GNDClampReference.Check() )
        {
            status = false;
        }
        if ( !m_POWERClampReference.Check() )
        {
            status = false;
        }
        if ( !status )
        {
        std::cerr << "Model: Voltage Range is invalid." << std::endl;
        }
        status = false;
    }
    if ( !m_pulldown.Check() )
    {
        std::cerr << "Model: Invalid pulldown." << std::endl;
        status = false;
    } 
    if ( !m_pullup.Check() )
    {
        std::cerr << "Model: Invalid pullup." << std::endl;
        status = false;
    } 
    if ( !m_POWERClamp.Check() )
    {
        std::cerr << "Model: Invalid POWER clamp." << std::endl;
        status = false;
    } 
    if ( !m_GNDClamp.Check() )
    {
        std::cerr << "Model: Invalid GND clamp." << std::endl;
        status = false;
    }
    if( m_type != IBIS_MODEL_TYPE::INPUT_ECL && m_type != IBIS_MODEL_TYPE::INPUT
        && m_type != IBIS_MODEL_TYPE::TERMINATOR && m_type != IBIS_MODEL_TYPE::SERIES
        && m_type != IBIS_MODEL_TYPE::SERIES_SWITCH )
    {
        if( !m_ramp.Check() )
        {
            std::cerr << "Invalid Ramp" << std::endl;
        }
    }
    return status;
}

bool IbisHeader::Check()
{
    bool status = true;


    std::cout << "Checking Header..." << std::endl;

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

    if( m_fileRevision == -1 )
    {
        std::cerr << "Missing [File Rev]" << std::endl;
        status = false;
    }

    if( m_fileName.empty() )
    {
        std::cerr << "Missing [File Name]" << std::endl;
        status = false;
    }

    std::string ext = m_fileName.substr( m_fileName.length() - 4 );

    if( !( !strcasecmp( ext.c_str(), ".ibs" )  || !strcasecmp( ext.c_str(), ".pkg" )
           || !strcasecmp( ext.c_str(), ".ebd" ) || !strcasecmp( ext.c_str(), ".ims" ) ) )
    {
        std::cerr << "[File Name]: Extension is not allowed: " <<  m_fileName << " " << ext << std::endl;
        status = false;
    }


    return status;
}

bool IbisPackageModel::Check()
{
    bool status = true;

    if( m_name.empty() )
    {
        std::cerr << "Package Model: name cannot be empty." << std::endl;
        status = false;
    }

    std::cout << "Checking package model " << m_name << "..." << std::endl;

    if( m_manufacturer.empty() )
    {
        std::cerr << "Package Model: manufacturer cannot be empty." << std::endl;
        status = false;
    }
    if( m_OEM.empty() )
    {
        std::cerr << "Package Model: OEM cannot be empty." << std::endl;
        status = false;
    }
    if( std::isnan( m_numberOfPins ) )
    {
        std::cerr << "Package Model: number of pins is NaN." << std::endl;
        status = false;
    }
    if( m_numberOfPins <= 0 )
    {
        std::cerr << "Package Model: number of pins is negative." << std::endl;
        status = false;
    }

    if( m_pins.size() != m_numberOfPins )
    {
        std::cerr << "Package Model: number of pins does not match [Pin Numbers] size" << std::endl;
        status = false;
    }

    for( int i = 0; i < m_pins.size(); i++ )
    {
        if( m_pins.at( i ).empty() )
        {
            std::cerr << "Package Model: Empty pin number" << std::endl;
            status = false;
        }
    }
    // resistance matrix is not required
    switch( m_resistanceMatrix->m_type )
    {
    case IBIS_MATRIX_TYPE::BANDED:

        if( !( static_cast<IBIS_MATRIX_BANDED*>( m_resistanceMatrix ) )->Check() )
        {
            std::cerr << "Package Model: Resistance matrix is incorrect" << std::endl;
            status = false;
        }
        break;
    case IBIS_MATRIX_TYPE::FULL:

        if( !( static_cast<IBIS_MATRIX_FULL*>( m_resistanceMatrix ) )->Check() )
        {
            std::cerr << "Package Model: Resistance matrix is incorrect" << std::endl;
            status = false;
        }
        break;
    case IBIS_MATRIX_TYPE::SPARSE:

        if( !( static_cast<IBIS_MATRIX_SPARSE*>( m_resistanceMatrix ) )->Check() )
        {
            std::cerr << "Package Model: Resistance matrix is incorrect" << std::endl;
            status = false;
        }
        break;
    }

    if( m_capacitanceMatrix != nullptr )
    {
        if( m_capacitanceMatrix->m_type == IBIS_MATRIX_TYPE::UNDEFINED )
        {
            std::cerr << "Package Model: Capacitance matrix is undefined" << std::endl;
            status = false;
        }

        switch( m_capacitanceMatrix->m_type )
        {
        case IBIS_MATRIX_TYPE::BANDED:

            if( !( static_cast<IBIS_MATRIX_BANDED*>( m_capacitanceMatrix ) )->Check() )
            {
                std::cerr << "Package Model: Capacitance matrix is incorrect" << std::endl;
                status = false;
            }
            break;
        case IBIS_MATRIX_TYPE::FULL:

            if( !( static_cast<IBIS_MATRIX_FULL*>( m_capacitanceMatrix ) )->Check() )
            {
                std::cerr << "Package Model: Capacitance matrix is incorrect" << std::endl;
                status = false;
            }
            break;
        case IBIS_MATRIX_TYPE::SPARSE:

            if( !( static_cast<IBIS_MATRIX_SPARSE*>( m_capacitanceMatrix ) )->Check() )
            {
                std::cerr << "Package Model: Capacitance matrix is incorrect" << std::endl;
                status = false;
            }
            break;
        default:
        {
            std::cerr << "Package Model: Capacitance matrix is missing " << std::endl;
            status = false;
        }
        }
    }
    else
    {
        std::cerr << "Package Model: Capacitance matrix is nullptr" << std::endl;
        status = false;
    }

    if( m_inductanceMatrix != nullptr )
    {
        if( m_inductanceMatrix->m_type == IBIS_MATRIX_TYPE::UNDEFINED )
        {
            std::cerr << "Package Model: Inductance matrix is undefined" << std::endl;
            status = false;
        }

        switch( m_inductanceMatrix->m_type )
        {
        case IBIS_MATRIX_TYPE::BANDED:

            if( !( static_cast<IBIS_MATRIX_BANDED*>( m_inductanceMatrix ) )->Check() )
            {
                std::cerr << "Package Model: Inductance matrix is incorrect" << std::endl;
                status = false;
            }
            break;
        case IBIS_MATRIX_TYPE::FULL:

            if( !( static_cast<IBIS_MATRIX_FULL*>( m_inductanceMatrix ) )->Check() )
            {
                std::cerr << "Package Model: Inductance matrix is incorrect" << std::endl;
                status = false;
            }
            break;
        case IBIS_MATRIX_TYPE::SPARSE:

            if( !( static_cast<IBIS_MATRIX_SPARSE*>( m_inductanceMatrix ) )->Check() )
            {
                std::cerr << "Package Model: Inductance matrix is incorrect" << std::endl;
                status = false;
            }
            break;
        default:
        {
            std::cerr << "Package Model: Inductance matrix is missing " << std::endl;
            status = false;
        }
        }
    }
    else
    {
        std::cerr << "Package Model: Inductance matrix is nullptr" << std::endl;
        status = false;
    }
    return status;
}


bool IbisParser::parseFile( std::string aFileName, IbisFile* aFile )
{
    m_ibisFile = aFile;
    std::string err_msg;

    std::ifstream ibisFile;
    ibisFile.open( aFileName );
    if( ibisFile )
    {
        err_msg = std::string( "Reading file " ) + aFileName + std::string( "..." );
        ibisReport( err_msg, RPT_SEVERITY_ACTION );
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
                ibisReport( "Unexpected end of file. Missing [END] ?", RPT_SEVERITY_ERROR );
                return false;
            }

            if( m_parrot )
            {
                PrintLine();
            }

            if( !onNewLine() )
            {
                err_msg = std::string( "Error at line " );
                err_msg += std::to_string( m_lineCounter );
                ibisReport( err_msg, RPT_SEVERITY_ERROR );
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
        err_msg = std::string( "Could not open file " );
        err_msg += aFileName;
        ibisReport( err_msg, RPT_SEVERITY_ERROR );
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


bool IbisParser::isLineEmptyFromCursor()
{
    int cursor = m_lineIndex;

    while( ( isspace( m_line[cursor] ) ) && ( cursor < m_lineLength ) )
    {
        cursor++;
    }

    return ( cursor >= m_lineLength );
}

bool IbisParser::readDvdt( std::string aString, dvdt* aDest )
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
        std::string str1 = aString.substr( 0, i );
        std::string str2 = aString.substr( i + 2, aString.size() );

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
        if ( aString == "NA" )
        {
            aDest->m_dv = std::nan( NAN_NA );
            aDest->m_dt = std::nan( NAN_NA );
        }
        else
        {
            status = false;
        }
    }

    return status;
}


bool IbisParser::parseDouble( double* aDest, std::string aStr, bool aAllowModifiers )
{
    // "  an entry of the C matrix could be given as 1.23e-12 or as 1.23p or 1.23pF."
    skipWhitespaces();
    bool status = true;

    std::string str = aStr;

    double result;

    if( str == "NA" )
    {
        result = std::nan( NAN_NA );
    }
    else if( !ibisToDouble( str, &result ) )
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
            std::string str2 = str.substr( 0, i );

            if( !ibisToDouble( str2, &result ) )
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
        result = std::nan( NAN_INVALID );
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
        ibisReport( "Line exceeds maximum length.", RPT_SEVERITY_ERROR );
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
    std::string str;

    if ( readWord( &str ) )
    {
        if ( parseDouble( aDest, str,  true ) )
        {
        }
        else
        {
            ibisReport( "Can't read double", RPT_SEVERITY_WARNING);
            status = false;
        }

    }
    else
    {
        ibisReport( "Can't read word", RPT_SEVERITY_WARNING);
        status = false;
    }

    return status;
}

bool IbisParser::readInt( int* aDest )
{
    bool     status = true;
    std::string str;
    double   buffer;

    if( aDest != nullptr )
    {
        if( readWord( &str ) )
        {
            if( !ibisToDouble( str, &buffer ) )
            {
                int result = static_cast<int>( buffer );
                if( buffer == static_cast<double>( result ) )
                {
                    *aDest = result;
                }
                else
                {
                    status = false;
                    ibisReport( "Number is not an integer", RPT_SEVERITY_WARNING );
                }
            }
            else
            {
                status = false;
                ibisReport( "Can't read number", RPT_SEVERITY_WARNING );
            }
        }
        else
        {
            ibisReport( "Can't read word", RPT_SEVERITY_WARNING );
            status = false;
        }
    }
    else
    {
        ibisReport( "Internal error while reading int", RPT_SEVERITY_ERROR );
        status = false;
    }

    return status;
}

bool IbisParser::readWord( std::string* aDest )
{
    skipWhitespaces();

    std::string str;
    while( ( !isspace( m_line[m_lineIndex] ) ) && ( m_lineIndex < m_lineLength ) )
    {
        str += m_line[m_lineIndex++];
    }

    *( aDest ) = str;

    return ( str.size() > 0 );
}

bool IbisParser::readString( std::string* aDest )
{
    while( m_lineIndex < m_lineLength )
    {
        *aDest += m_line[m_lineIndex++];
    }

    // Remove extra whitespace characters
    int len = aDest->length();
    char c = (*aDest)[ len -1 ];
    int i = 0;

    while( isspace( c ) && ( i < len ) )
    {
        c = (*aDest)[ len - 1 - i ];
        i++;
    }
    *aDest = aDest->substr( 0, len - i +1 );

    return true;
}

bool IbisParser::StoreString( std::string* aDest, bool aMultiline )
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

    std::string strChar;

    // We cannot stop at m_lineLength here, because lineLength could stop before |_char
    // if the char is remains the same

    char c = m_line[m_lineIndex++];
    char d = c;

    if( !( c == '!' || c == '"' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\''
           || c == '(' || c == ')' || c == '*' || c == ',' || c == ':' || c == ';' || c == '<'
           || c == '>' || c == '?' || c == '@' || c == '\\' || c == '^' || c == '`' || c == '{'
           || c == '|' || c == '}' || c == '~' || c == ')' ) )
    {
        ibisReport( "New comment character is invalid.", RPT_SEVERITY_ERROR );
    }

    c = m_line[m_lineIndex++];

    while( ( !isspace( c ) ) && c != 0 && c != '\n' )
    {
        strChar += c;
        c = m_line[m_lineIndex++];
    }

    if( !strcmp( strChar.c_str(), "_char" ) )
    {
        ibisReport( "Invalid syntax. Should be |_char or &_char, etc...",
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
        ibisReport( "No extra argument was expected", RPT_SEVERITY_ERROR );
        return false;
    }

    m_commentChar = d;


    m_continue = IBIS_PARSER_CONTINUE::NONE;

    return true;
}

std::string* IbisParser::getKeyword()
{
    //"Keywords must be enclosed in square brackets, “[]”, and must start in column 1 of the line."
    //"No space or tab is allowed immediately after the opening bracket “[” or immediately"
    // "before the closing bracket “]"

    if( m_line[m_lineIndex] != '[' )
    {
        return nullptr;
    }

    m_lineIndex++;

    std::string* keyword = new std::string( "" );

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

bool IbisParser::changeContext( std::string aKeyword )
{
    bool status = true;

    // Old context;
    IBIS_PARSER_CONTEXT old_context = m_context;
    IbisComponent*      old_component = m_currentComponent;
    IbisModel*          old_model = m_currentModel;
    IbisModelSelector*  old_modelSelector = m_currentModelSelector;
    IbisPackageModel*   old_packageModel = m_currentPackageModel;


    if( status )
    {
        switch( m_context )
        {
        case IBIS_PARSER_CONTEXT::HEADER: status &= m_ibisFile->m_header.Check(); break;
        case IBIS_PARSER_CONTEXT::COMPONENT: status &= m_currentComponent->Check(); break;
        case IBIS_PARSER_CONTEXT::MODEL: status &= m_currentModel->Check(); break;
        case IBIS_PARSER_CONTEXT::MODELSELECTOR: status &= m_currentModelSelector->Check(); break;
        case IBIS_PARSER_CONTEXT::PACKAGEMODEL: status &= m_currentPackageModel->Check(); break;
        case IBIS_PARSER_CONTEXT::END:
            ibisReport( "Internal Error: Cannot change context after [END]" );
            status = false;
            break;

        default: ibisReport( "Internal Error: Changing context from undefined context" );
        }
    }

    if( aKeyword != "END" && status )
    {
        //New context
        if( !strcasecmp( aKeyword.c_str(), "Component" ) )
        {
            IbisComponent comp;
            StoreString( &( comp.m_name ), false );
            m_ibisFile->m_components.push_back( comp );
            m_currentComponent = &( m_ibisFile->m_components.back() );
            m_context = IBIS_PARSER_CONTEXT::COMPONENT;
        }
        else if( !strcasecmp( aKeyword.c_str(), "Model_Selector" ) )
        {
            IbisModelSelector MS;
            StoreString( &( MS.m_name ), false );
            m_ibisFile->m_modelSelectors.push_back( MS );
            m_currentModelSelector = &( m_ibisFile->m_modelSelectors.back() );
            m_context = IBIS_PARSER_CONTEXT::MODELSELECTOR;
            m_continue = IBIS_PARSER_CONTINUE::MODELSELECTOR;
        }
        else if( !strcasecmp( aKeyword.c_str(), "Model" ) )
        {
            IbisModel model;
            model.m_temperatureRange.value[IBIS_CORNER::MIN] = 0;
            model.m_temperatureRange.value[IBIS_CORNER::TYP] = 50;
            model.m_temperatureRange.value[IBIS_CORNER::MAX] = 100;
            StoreString( &( model.m_name ), false );
            m_ibisFile->m_models.push_back( model );
            m_currentModel = &( m_ibisFile->m_models.back() );
            m_context = IBIS_PARSER_CONTEXT::MODEL;
            m_continue = IBIS_PARSER_CONTINUE::MODEL;
        }
        else if( !strcasecmp( aKeyword.c_str(), "Define_Package_Model" ) )
        {
            IbisPackageModel PM;
            PM.m_resistanceMatrix = new IBIS_MATRIX();
            PM.m_capacitanceMatrix = new IBIS_MATRIX();
            PM.m_inductanceMatrix = new IBIS_MATRIX();

            PM.m_resistanceMatrix->m_type = IBIS_MATRIX_TYPE::UNDEFINED;
            PM.m_capacitanceMatrix->m_type = IBIS_MATRIX_TYPE::UNDEFINED;
            PM.m_inductanceMatrix->m_type = IBIS_MATRIX_TYPE::UNDEFINED;

            PM.m_resistanceMatrix->m_dim = -1;
            PM.m_capacitanceMatrix->m_dim = -1;
            PM.m_inductanceMatrix->m_dim = -1;

            StoreString( &( PM.m_name ), false );

            m_ibisFile->m_packageModels.push_back( PM );
            m_currentPackageModel = &( m_ibisFile->m_packageModels.back() );
            m_context = IBIS_PARSER_CONTEXT::PACKAGEMODEL;
        }
        else if( !strcasecmp( aKeyword.c_str(), "End_Package_Model" ) )
        {
            if( m_currentComponent != nullptr )
            {
                m_context = IBIS_PARSER_CONTEXT::COMPONENT;
                m_continue = IBIS_PARSER_CONTINUE::NONE;
            }
            else // .pkg file, we just go back to header, to get the [END] keyword
            {    // This will cause the header to be checked twice.
                m_context = IBIS_PARSER_CONTEXT::HEADER;
                m_continue = IBIS_PARSER_CONTINUE::NONE;
            }
        }
        else
        {
            status = false;
            std::string err_msg = std::string( "Unknown keyword in " );

            switch( m_context )
            {
            case IBIS_PARSER_CONTEXT::HEADER: err_msg += "HEADER"; break;
            case IBIS_PARSER_CONTEXT::COMPONENT: err_msg += "COMPONENT"; break;
            case IBIS_PARSER_CONTEXT::MODELSELECTOR: err_msg += "MODEL_SELECTOR"; break;
            case IBIS_PARSER_CONTEXT::MODEL: err_msg += "MODEL"; break;
            case IBIS_PARSER_CONTEXT::PACKAGEMODEL: err_msg += "PACKAGE_MODEL"; break;
            case IBIS_PARSER_CONTEXT::PACKAGEMODEL_MODELDATA:
                err_msg += "PACKAGE_MODEL_MODEL_DATA";
                break;
            default: err_msg += "???"; break;
            }

            err_msg += " context : " + aKeyword;

            ibisReport( err_msg, RPT_SEVERITY_ERROR );
        }
    }
    else
    {
        m_context = IBIS_PARSER_CONTEXT::END;
    }

    return status;
}


bool IbisParser::parseModelSelector( std::string aKeyword )
{
    bool status = true;

    if( !changeContext( aKeyword ) )
    {
        status = false;
    }
    return status;
}


bool IbisParser::readRampdvdt( dvdtTypMinMax* aDest )
{
    bool status = true;
    std::string str;

    if( readWord( &str ) )
    {
        dvdtTypMinMax ramp;

        if( readDvdt( str, &( ramp.value[IBIS_CORNER::TYP] ) ) )
        {
            if( readDvdt( str, &( ramp.value[IBIS_CORNER::MIN] ) ) )
            {
                if( readDvdt( str, &( ramp.value[IBIS_CORNER::MAX] ) ) )
                {
                    *aDest = ramp;
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
    else
    {
        status = false;
    }

    return status;
}

bool IbisParser::readRamp()
{
    bool status = true;

    m_continue = IBIS_PARSER_CONTINUE::RAMP;

    if( readNumericSubparam( std::string( "R_load" ), &( m_currentModel->m_ramp.m_Rload ) ) )
        ;
    else
    {
        std::string str;

        if ( readWord( &str ) )
        {
            if ( !strcmp( str.c_str(), "dV/dt_r" ) )
            {
                readRampdvdt( &(m_currentModel->m_ramp.m_rising) );
            }
            else if ( !strcmp( str.c_str(), "dV/dt_f" ) )
            {
                readRampdvdt( &(m_currentModel->m_ramp.m_falling) );
            }
            else
            {
                std::cerr << "Ramp: Invalid line: "<< str << std::endl;
                status = false;
            }


        }

    }
    if( status == false )
    {
    }
    return status;
}


bool IbisParser::parseModel( std::string aKeyword )
{
    bool status = true;
    
    if( !strcasecmp( aKeyword.c_str(), "Voltage_Range" ) )
        status &= readTypMinMaxValue( &( m_currentModel->m_voltageRange ) );
    else if( !strcasecmp( aKeyword.c_str(), "Temperature_Range" ) )
        status &= readTypMinMaxValue( &( m_currentModel->m_temperatureRange ) );
    else if( !strcasecmp( aKeyword.c_str(), "GND_Clamp" ) )
        status &= readIVtableEntry( &( m_currentModel->m_GNDClamp ) );
    else if( !strcasecmp( aKeyword.c_str(), "POWER_Clamp" ) )
        status &= readIVtableEntry( &( m_currentModel->m_POWERClamp ) );
    else if( !strcasecmp( aKeyword.c_str(), "Pulldown" ) )
        status &= readIVtableEntry( &( m_currentModel->m_pulldown ) );
    else if( !strcasecmp( aKeyword.c_str(), "Pullup" ) )
        status &= readIVtableEntry( &( m_currentModel->m_pullup ) );
    else if( !strcasecmp( aKeyword.c_str(), "Rising_Waveform" ) )
        status &= readWaveform( nullptr, IBIS_WAVEFORM_TYPE::RISING );
    else if( !strcasecmp( aKeyword.c_str(), "Falling_Waveform" ) )
        status &= readWaveform( nullptr, IBIS_WAVEFORM_TYPE::FALLING );
    else if( !strcasecmp( aKeyword.c_str(), "Ramp" ) )
        status &= readRamp();
    else if( !strcasecmp( aKeyword.c_str(), "Pullup_Reference" ) )
        status &= readTypMinMaxValue( &( m_currentModel->m_pullupReference ) );
    else if( !strcasecmp( aKeyword.c_str(), "Pulldown_Reference" ) ) 
        status &= readTypMinMaxValue( &( m_currentModel->m_pulldownReference ) );
    else if( !strcasecmp( aKeyword.c_str(), "POWER_Clamp_Reference" ) )
        status &= readTypMinMaxValue( &( m_currentModel->m_POWERClampReference ) );
    else if( !strcasecmp( aKeyword.c_str(), "GND_Clamp_Reference" ) )
        status &= readTypMinMaxValue( &( m_currentModel->m_GNDClampReference ) );
    else if( !strcasecmp( aKeyword.c_str(), "Rac" ) )
        status &= readTypMinMaxValue( &( m_currentModel->m_Rac ) );
    else if( !strcasecmp( aKeyword.c_str(), "Cac" ) )
        status &= readTypMinMaxValue( &( m_currentModel->m_Cac ) );
    else if( !strcasecmp( aKeyword.c_str(), "Rpower" ) )
        status &= readTypMinMaxValue( &( m_currentModel->m_Rpower ) );
    else if( !strcasecmp( aKeyword.c_str(), "Rgnd" ) )
        status &= readTypMinMaxValue( &( m_currentModel->m_Rgnd ) );
    else
    {
        if( !changeContext( aKeyword ) )
        {
            status = false;
        }
    }
    return status;
}

bool IbisParser::readPackageModelPins()
{
    m_continue = IBIS_PARSER_CONTINUE::PACKAGEMODEL_PINS;
    std::string str;

    if( readWord( &str ) )
        m_currentPackageModel->m_pins.push_back( str );

    return true;
}


bool IbisParser::readMatrixBanded( std::string aKeyword, IBIS_MATRIX_BANDED* aDest )
{
    bool status = true;
    m_continue = IBIS_PARSER_CONTINUE::MATRIX;
    m_currentMatrix = static_cast<IBIS_MATRIX_BANDED*>( aDest );

    if( aDest != nullptr )
    {
        if( !strcasecmp( aKeyword.c_str(), "Bandwidth" ) )
        {
            if( m_currentMatrix->m_type == IBIS_MATRIX_TYPE::BANDED )
            {
                status &= readInt( &( aDest->m_bandwidth ) );
                if( status )
                {
                    if( aDest->m_data != nullptr )
                    {
                        free( aDest->m_data );
                    }
                    aDest->m_data = static_cast<double*>(
                            malloc( ( aDest->m_bandwidth * aDest->m_dim ) * sizeof( double ) ) );

                    if( !aDest->m_data )
                    {
                        ibisReport( "Banded matrix: malloc error.", RPT_SEVERITY_ERROR );
                        status = false;
                    }
                }
            }
            else
            {
                status = false;
                ibisReport( "[Bandwidth] is reserved for banded matrices.",
                                    RPT_SEVERITY_ERROR );
            }
        }
        if( strcasecmp( aKeyword.c_str(), "Dummy" ) )
        {
            int i;
            for( i = 0; i < aDest->m_bandwidth; i++ )
            {
                if( i + m_currentMatrixRowIndex >= aDest->m_bandwidth )
                {
                    ibisReport( "Banded matrix: too much data to fit into that row.",
                                        RPT_SEVERITY_ERROR );
                    status = false;
                    break;
                }

                int index = i + m_currentMatrixRow * aDest->m_bandwidth;

                if( !readDouble( &( aDest->m_data[index] ) ) )
                {
                    ibisReport( "Banded matrix: can't read row.", RPT_SEVERITY_ERROR );
                    status = false;
                    break;
                }
            }
            m_currentMatrixRowIndex = i;
        }
    }
    else
    {
        ibisReport( "Banded matrix: not initialized.", RPT_SEVERITY_ERROR );
        status = false;
    }

    return status;
}


bool IbisParser::readMatrixFull( std::string aKeyword, IBIS_MATRIX_FULL* aDest )
{
    bool status = true;
    m_continue = IBIS_PARSER_CONTINUE::MATRIX;
    m_currentMatrix = static_cast<IBIS_MATRIX_FULL*>( aDest );

    if( strcasecmp( aKeyword.c_str(), "Dummy" ) )
    {
        std::vector<std::string> values = ReadTableLine();
        int                   i;
        for( i = 0; i < values.size(); i++ )
        {
            int index = i + m_currentMatrixRow * aDest->m_dim + m_currentMatrixRow;
            // + final m_currentMatrixRow because we don't fill the lower triangle.

            if( i >= ( aDest->m_dim - m_currentMatrixRow - m_currentMatrixRowIndex ) )
            {
                ibisReport( "Full matrix : too much data to fit into that row",
                                    RPT_SEVERITY_ERROR );
                status = false;
                break;
            }

            if( index >= aDest->m_dim * aDest->m_dim )
            {
                status = false;
                ibisReport( "Full matrix : too much data to fit into that matrix",
                                    RPT_SEVERITY_ERROR );
                break;
            }
            if( !parseDouble( aDest->m_data + index, values.at( i ), true ) )
            {
                ibisReport( "Banded matrix: can't read row.", RPT_SEVERITY_ERROR );
                status = false;
            }
            else
            {
            }
        }
        m_currentMatrixRowIndex = i;
    }
    return status;
}


bool IbisParser::readMatrixSparse( std::string aKeyword, IBIS_MATRIX_SPARSE* aDest )
{
    bool status = true;

    if( strcasecmp( aKeyword.c_str(), "Dummy"  ) )
    {
        int    subindex;
        double value;

        if( readInt( &subindex ) )
        {
            if( readDouble( &value ) )
            {
                int index = subindex + m_currentMatrixRow * aDest->m_dim + m_currentMatrixRow;
            }
            else
            {
                ibisReport( "Sparse matrix : can't read value", RPT_SEVERITY_ERROR );
            }
        }
        else
        {
            ibisReport( "Sparse matrix : can't read index", RPT_SEVERITY_ERROR );
        }
    }
    return status;
}

bool IbisParser::readMatrix( IBIS_MATRIX** aSource )
{
    bool     status = true;
    std::string str;

    bool init = false;

    if( aSource != nullptr )
    {
        if( *aSource != nullptr )
        {
            if( ( *aSource )->m_type != IBIS_MATRIX_TYPE::BANDED
                && ( *aSource )->m_type != IBIS_MATRIX_TYPE::FULL
                && ( *aSource )->m_type != IBIS_MATRIX_TYPE::SPARSE )
            {
                init = false;
            }
            else
            {
                init = true;
            }
        }
        else
        {
            ibisReport( "Internal Error: matrix pointer is null." );
            status = false;
        }
    }
    else
    {
        ibisReport( "Internal Error: matrix pointer pointer is null." );
        status = false;
    }

    if( m_continue != IBIS_PARSER_CONTINUE::MATRIX && status )
    {
        if( !init )
        {
            if( readWord( &str ) )
            {
                if( !strcasecmp( str.c_str(), "Banded_Matrix" ) )
                {
                    IBIS_MATRIX_BANDED* matrix = new IBIS_MATRIX_BANDED();
                    matrix->m_dim = m_currentPackageModel->m_numberOfPins;
                    *aSource = static_cast<IBIS_MATRIX*>( matrix );
                    m_currentMatrix = *aSource;
                    m_currentMatrix->m_type = IBIS_MATRIX_TYPE::BANDED;
                    m_continue = IBIS_PARSER_CONTINUE::MATRIX;
                }
                if( !strcasecmp( str.c_str(), "Full_Matrix" ) )
                {
                    IBIS_MATRIX_FULL* matrix = new IBIS_MATRIX_FULL();
                    *aSource = static_cast<IBIS_MATRIX*>( matrix );
                    m_currentMatrix = *aSource;
                    m_currentMatrix->m_dim = m_currentPackageModel->m_numberOfPins;

                    matrix->m_dim = m_currentPackageModel->m_numberOfPins;
                    matrix->m_data = static_cast<double*>(
                            malloc( ( matrix->m_dim * matrix->m_dim ) * sizeof( double ) ) );
                    m_currentMatrix->m_type = IBIS_MATRIX_TYPE::FULL;
                    m_continue = IBIS_PARSER_CONTINUE::MATRIX;
                }
                if( !strcasecmp( str.c_str(), "Sparse_Matrix" ) )
                {
                    IBIS_MATRIX_SPARSE* matrix = new IBIS_MATRIX_SPARSE();
                    *aSource = static_cast<IBIS_MATRIX*>( matrix );
                    m_currentMatrix = *aSource;
                    matrix->m_dim = m_currentPackageModel->m_numberOfPins;
                    matrix->m_data = static_cast<double*>(
                            malloc( ( matrix->m_dim * matrix->m_dim ) * sizeof( double ) ) );
                    m_currentMatrix->m_type = IBIS_MATRIX_TYPE::SPARSE;
                    m_continue = IBIS_PARSER_CONTINUE::MATRIX;
                }
                else
                {
                    status = false;
                    ibisReport( "Matrix: unknown type.", RPT_SEVERITY_ERROR );
                    ibisReport( str, RPT_SEVERITY_INFO );
                }
            }
            else
            {
                status = false;
                ibisReport( "Matrix: type missing.", RPT_SEVERITY_ERROR );
            }
        }
        else
        {
            status = false;
            ibisReport( " Matrix :already init. But m_continue was not set" );
        }
    }
    else
    {
        if( aSource != nullptr )
        {
            if( *aSource != nullptr )
            {
                // If m_continue is set, ( and no keyword ) then it is a row
                switch( ( *aSource )->m_type )
                {
                case IBIS_MATRIX_TYPE::BANDED:
                    readMatrixBanded( std::string( "Dummy" ),
                                      static_cast<IBIS_MATRIX_BANDED*>( *aSource ) );
                    break;
                case IBIS_MATRIX_TYPE::FULL:
                    readMatrixFull( std::string( "Dummy" ),
                                    static_cast<IBIS_MATRIX_FULL*>( *aSource ) );
                    break;
                case IBIS_MATRIX_TYPE::SPARSE:
                    readMatrixSparse( std::string( "Dummy" ),
                                      static_cast<IBIS_MATRIX_SPARSE*>( *aSource ) );
                    break;
                case IBIS_MATRIX_TYPE::UNDEFINED:
                default:
                {
                    status = false;
                    ibisReport( " Matrix :Tried to read a row from an undefined matrix" );
                }
                }
            }
            else
            {
                ibisReport( "Internal Error : matrix pointer is null" );
            }
        }
        else
        {
            ibisReport( "Internal Error : matrix pointer pointer is null" );
        }
    }
    return status;
}

bool IbisParser::parsePackageModelModelData( std::string aKeyword )
{
    bool status = true;

    if( !strcasecmp( aKeyword.c_str(), "Resistance_Matrix" ) )
    {
        IBIS_MATRIX dest, source;
        status &= readMatrix( &( m_currentPackageModel->m_resistanceMatrix ) );
    }
    else if( !strcasecmp( aKeyword.c_str(), "Capacitance_Matrix" ) )
    {
        status &= readMatrix( &( m_currentPackageModel->m_capacitanceMatrix ) );
    }
    else if( !strcasecmp( aKeyword.c_str(), "Inductance_Matrix" ) )
    {
        status &= readMatrix( &( m_currentPackageModel->m_inductanceMatrix ) );
    }
    else if( !strcasecmp( aKeyword.c_str(), "Bandwidth" ) )
    {
        status &= readMatrixBanded( aKeyword, static_cast<IBIS_MATRIX_BANDED*>( m_currentMatrix ) );
    }
    else if( !strcasecmp( aKeyword.c_str(), "Row" ) )
    {
        status &= readInt( &( m_currentMatrixRow ) );
        m_currentMatrixRow--;        // The matrix starts at 0
        m_currentMatrixRowIndex = 0; // The matrix starts at 0*/
        m_continue = IBIS_PARSER_CONTINUE::MATRIX;
    }
    else if( !strcasecmp( aKeyword.c_str(), "End_Model_Data" ) )
    {
        m_context = IBIS_PARSER_CONTEXT::PACKAGEMODEL;
        m_continue = IBIS_PARSER_CONTINUE::NONE;
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

bool IbisParser::parsePackageModel( std::string aKeyword )
{
    bool status = true;

    if( !strcasecmp( aKeyword.c_str(), "Manufacturer" ) )
        status &= StoreString( &( m_currentPackageModel->m_manufacturer ), false );
    else if( !strcasecmp( aKeyword.c_str(), "OEM" ) )
        status &= StoreString( &( m_currentPackageModel->m_OEM ), false );
    else if( !strcasecmp( aKeyword.c_str(), "Description" ) )
        status &= StoreString( &( m_currentPackageModel->m_description ), false );
    else if( !strcasecmp( aKeyword.c_str(), "Number_of_Pins" ) )
        status &= readInt( &( m_currentPackageModel->m_numberOfPins ) );
    else if( !strcasecmp( aKeyword.c_str(), "Pin_Numbers" ) )
        status &= readPackageModelPins();
    else if( !strcasecmp( aKeyword.c_str(), "Model_Data" ) )
    {
        m_context = IBIS_PARSER_CONTEXT::PACKAGEMODEL_MODELDATA;
        m_continue = IBIS_PARSER_CONTINUE::NONE;
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

bool IbisParser::readNumericSubparam( std::string aSubparam, double* aDest )
{
    std::string paramName;
    bool     status = true;

    int old_index = m_lineIndex;
    m_lineIndex = 0;
    if( aSubparam.size() < m_lineLength )
    {
        for( int i = 0; i < aSubparam.size(); i++ )
        {
            paramName += m_line[m_lineIndex++];
        }

        if( !strcmp( paramName.c_str(), aSubparam.c_str() ) )
        {
            skipWhitespaces();
            if( m_line[m_lineIndex++] == '=' )
            {
                skipWhitespaces();

                std::string strValue;

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

    if ( status == false )
    {
        m_lineIndex = old_index;
    }

    return status;
}


bool IbisParser::readTypMinMaxValue( TypMinMaxValue* aDest )
{
    bool status = true;

    skipWhitespaces();

    std::string strValue;


    if( !readWord( &strValue ) )
    {
        status = false;
    }
    else
    {
        if( parseDouble( &( ( *aDest ).value[IBIS_CORNER::TYP] ), strValue, true ) )
        {
            if( readWord( &strValue ) )
            {
                parseDouble( &( ( *aDest ).value[IBIS_CORNER::MIN] ), strValue, true );
                if( readWord( &strValue ) )
                {
                    parseDouble( &( ( *aDest ).value[IBIS_CORNER::MAX] ), strValue, true );
                }
            }
        }
        else
        {
            status = false;
            ibisReport( "Typ-Min-Max Values requires at least Typ.", RPT_SEVERITY_ERROR );
        }
    }
    return status;
}

bool IbisParser::readTypMinMaxValueSubparam( std::string aSubparam, TypMinMaxValue* aDest )
{
    std::string paramName;
    bool     status = true;

    m_lineIndex = 0; // rewind

    if( aSubparam.size() < m_lineLength )
    {
        for( int i = 0; i < aSubparam.size(); i++ )
        {
            paramName += m_line[m_lineIndex++];
        }

        if( !strcmp( paramName.c_str(), aSubparam.c_str() ) )
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

    std::string subparam;
    if( readWord( &subparam ) )
    {
        std::cout << "Reading suparam..." << subparam.substr( 0, 10 ) << std::endl;
        switch( m_continue )
        {
        case IBIS_PARSER_CONTINUE::MODEL:
            if( !strcmp( subparam.substr( 0, 10 ).c_str(), "Model_type" ) )
            {
                std::cout << "Assigning model type..." << std::endl;
                if( readWord( &subparam ) )
                {
                    if( !strcmp( subparam.c_str(), "Input" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::INPUT;
                    else if( !strcmp( subparam.c_str(), "Output" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OUTPUT;
                    else if( !strcmp( subparam.c_str(), "I/O" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO;
                    else if( !strcmp( subparam.c_str(), "3-state" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::THREE_STATE;
                    else if( !strcmp( subparam.c_str(), "Open_drain" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OPEN_DRAIN;
                    else if( !strcmp( subparam.c_str(), "I/O_Open_drain" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO_OPEN_DRAIN;
                    else if( !strcmp( subparam.c_str(), "Open_sink" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OPEN_SINK;
                    else if( !strcmp( subparam.c_str(), "I/O_open_sink" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO_OPEN_SINK;
                    else if( !strcmp( subparam.c_str(), "Open_source" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OPEN_SOURCE;
                    else if( !strcmp( subparam.c_str(), "I/O_open_source" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO_OPEN_SOURCE;
                    else if( !strcmp( subparam.c_str(), "Input_ECL" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::INPUT_ECL;
                    else if( !strcmp( subparam.c_str(), "Output_ECL" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::OUTPUT_ECL;
                    else if( !strcmp( subparam.c_str(), "I/O_ECL" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::IO_ECL;
                    else if( !strcmp( subparam.c_str(), "3-state_ECL" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::THREE_STATE_ECL;
                    else if( !strcmp( subparam.c_str(), "Terminator" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::TERMINATOR;
                    else if( !strcmp( subparam.c_str(), "Series" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::SERIES;
                    else if( !strcmp( subparam.c_str(), "Series_switch" ) )
                        m_currentModel->m_type = IBIS_MODEL_TYPE::SERIES_SWITCH;
                    else
                    {
                        std::string err_msg = std::string( "Unknown Model_type: " );
                        err_msg += subparam;
                        ibisReport( err_msg, RPT_SEVERITY_ERROR );
                        status = false;
                    }
                }
                else
                {
                    ibisReport( "Internal Error while reading model_type",
                                        RPT_SEVERITY_ERROR );
                    status = false;
                }
            }
            else if( !strcmp( subparam.substr( 0, 7 ).c_str(), "Enable" ) )
            {
                if( readWord( &subparam ) )
                {
                    if( !strcmp( subparam.c_str(), "Active-High" ) )
                        m_currentModel->m_enable = IBIS_MODEL_ENABLE::ACTIVE_HIGH;
                    else if( !strcmp( subparam.c_str(), "Active-Low" ) )
                        m_currentModel->m_enable = IBIS_MODEL_ENABLE::ACTIVE_LOW;
                    else
                    {
                        std::string err_msg = std::string( "Unknown Enable: " );
                        err_msg += subparam;
                        ibisReport( err_msg, RPT_SEVERITY_ERROR );
                        status = false;
                    }
                }
                else
                {
                    ibisReport( "Internal Error while reading Enable", RPT_SEVERITY_ERROR );
                    status = false;
                }
            }
            else if( subparam.substr( 0, 9 ) == "Polarity" )
            {
                if( readWord( &subparam ) )
                {
                    if( subparam == "Inverting" )
                        m_currentModel->m_enable = IBIS_MODEL_ENABLE::ACTIVE_HIGH;
                    else if( subparam == "Non-Inverting" )
                        m_currentModel->m_enable = IBIS_MODEL_ENABLE::ACTIVE_LOW;
                    else
                    {
                        std::string err_msg = std::string( "Unknown Polarity: " );
                        err_msg += subparam;
                        ibisReport( err_msg, RPT_SEVERITY_ERROR );
                        status = false;
                    }
                }
                else
                {
                    ibisReport( "Internal Error while reading Enable", RPT_SEVERITY_ERROR );
                    status = false;
                }
            }
            else if( readNumericSubparam( std::string( "Vinl" ), &( m_currentModel->m_vinl ) ) )
                ;
            else if( readNumericSubparam( std::string( "Vinh" ), &( m_currentModel->m_vinh ) ) )
                ;
            else if( readNumericSubparam( std::string( "Vref" ), &( m_currentModel->m_vref ) ) )
                ;
            else if( readNumericSubparam( std::string( "Rref" ), &( m_currentModel->m_rref ) ) )
                ;
            else if( readNumericSubparam( std::string( "Cref" ), &( m_currentModel->m_cref ) ) )
                ;
            else if( readNumericSubparam( std::string( "Vmeas" ), &( m_currentModel->m_vmeas ) ) )
                ;
            else if( readTypMinMaxValueSubparam( std::string( "C_comp" ), &( m_currentModel->m_C_comp ) ) )
                ; 
            else
            {
                status = false;
            }

            m_continue = IBIS_PARSER_CONTINUE::MODEL;

            break;
        default:
            status = false;
            ibisReport( "Internal error: continued reading a model that is not started.",
                                RPT_SEVERITY_ERROR );
        }
    }

    return status;
}


bool IbisParser::parseHeader( std::string aKeyword )
{
    bool status = true;

    if( !strcasecmp( aKeyword.c_str(), "IBIS_Ver" ) )
    {
        status &= readDouble( &( m_ibisFile->m_header.m_ibisVersion ) );
    }
    else if( !strcasecmp( aKeyword.c_str(), "Comment_char" ) )
    {
        ChangeCommentChar();
    }
    else if( !strcasecmp( aKeyword.c_str(), "File_Name" ) )
    {
        StoreString( &( m_ibisFile->m_header.m_fileName ), false );
    }
    else if( !strcasecmp( aKeyword.c_str(), "File_Rev" ) )
    {
        status &= readDouble( &( m_ibisFile->m_header.m_fileRevision) );
    }
    else if( !strcasecmp( aKeyword.c_str(), "Source" ) )
    {
        StoreString( &( m_ibisFile->m_header.m_source ), true );
    }
    else if( !strcasecmp( aKeyword.c_str(), "Notes" ) )
    {
        StoreString( &( m_ibisFile->m_header.m_notes ), true );
    }
    else if( !strcasecmp( aKeyword.c_str(), "Disclaimer" ) )
    {
        StoreString( &( m_ibisFile->m_header.m_disclaimer ), true );
    }
    else if( !strcasecmp( aKeyword.c_str(), "Copyright" ) )
    {
        StoreString( &( m_ibisFile->m_header.m_copyright ), true );
    }
    else if( !strcasecmp( aKeyword.c_str(), "Date" ) )
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


bool IbisParser::parseComponent( std::string aKeyword )
{
    bool status = true;
    if( !strcasecmp( aKeyword.c_str(), "Manufacturer" ) )
    {
        status &= StoreString( &( m_currentComponent->m_manufacturer ), true );
    }
    else if( !strcasecmp( aKeyword.c_str(), "Package" ) )
    {
        status &= readPackage();
    }
    else if( !strcasecmp( aKeyword.c_str(), "Pin" ) )
    {
        status &= readPin();
    }
    else if( !strcasecmp( aKeyword.c_str(), "Pin_Mapping" ) )
    {
        status &= readPinMapping();
    }
    else if( !strcasecmp( aKeyword.c_str(), "Diff_Pin" ) )
    {
        status &= readDiffPin();
    }
    /*
    else if( aKeyword == "Die_Supply_Pads" )
    {
        status &= ReadDieSupplyPads();
    }*/
    else if( !strcasecmp( aKeyword.c_str(), "Package_Model" ) )
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

std::vector<std::string> IbisParser::ReadTableLine()
{
    std::vector<std::string> elements;
    while( m_lineIndex < m_lineLength )
    {
        std::string str;

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

    std::vector<std::string> fields;

    fields = ReadTableLine();

    int extraArg = ( m_continue == IBIS_PARSER_CONTINUE::NONE ) ? 1 : 0;

    if( fields.size() == ( 4 + extraArg ) )
    {
        if( fields.at( 0 ) == "R_pkg" )
        {
            if( parseDouble( &( m_currentComponent->m_package.m_Rpkg.value[IBIS_CORNER::TYP] ),
                             fields.at( 1 ), true ) )
            {
                status = false;
            }

            parseDouble( &( m_currentComponent->m_package.m_Rpkg.value[IBIS_CORNER::MIN] ),
                         fields.at( 2 ), true );
            parseDouble( &( m_currentComponent->m_package.m_Rpkg.value[IBIS_CORNER::MAX] ),
                         fields.at( 3 ), true );
        }
        else if( fields.at( 0 ) == "L_pkg" )
        {
            if( parseDouble( &( m_currentComponent->m_package.m_Lpkg.value[IBIS_CORNER::TYP] ),
                             fields.at( 1 ), true ) )
            {
                status = false;
            }

            parseDouble( &( m_currentComponent->m_package.m_Lpkg.value[IBIS_CORNER::MIN] ),
                         fields.at( 2 ), true );
            parseDouble( &( m_currentComponent->m_package.m_Lpkg.value[IBIS_CORNER::MAX] ),
                         fields.at( 3 ), true );
        }
        else if( fields.at( 0 ) == "C_pkg" )
        {
            if( parseDouble( &( m_currentComponent->m_package.m_Cpkg.value[IBIS_CORNER::TYP] ),
                             fields.at( 1 ), true ) )
            {
                status = false;
            }

            parseDouble( &( m_currentComponent->m_package.m_Cpkg.value[IBIS_CORNER::MIN] ),
                         fields.at( 2 ), true );
            parseDouble( &( m_currentComponent->m_package.m_Cpkg.value[IBIS_CORNER::MAX] ),
                         fields.at( 3 ), true );
        }
    }
    else
    {
        if( fields.size() != 0 )
        {
            ibisReport( "A [Package] line requires exactly 4 elements.",
                                RPT_SEVERITY_ERROR );
            status = false;
        }
    }
    m_continue = IBIS_PARSER_CONTINUE::COMPONENT_PACKAGE;

    return true;
}


bool IbisParser::readPin()
{
    bool status = true;

    std::vector<std::string> fields;

    fields = ReadTableLine();

    IbisComponentPin pin;

    if( ( fields.size() == 3 ) )
    {
        if( m_continue == IBIS_PARSER_CONTINUE::COMPONENT_PIN ) // No info on first line
        {
            pin.m_pinName = fields.at( 0 );
            pin.m_signalName = fields.at( 1 );
            pin.m_modelName = fields.at( 2 );

            m_currentComponent->m_pins.push_back( pin );
        }
        else
        {
            pin.m_dummy = true;
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
                if ( parseDouble( &( pin.m_Rpin ), fields.at( pin.m_Rcol ), true ) )
                {
                    if ( parseDouble( &( pin.m_Lpin ), fields.at( pin.m_Lcol ), true ) )
                    {
                        if ( parseDouble( &( pin.m_Cpin ), fields.at( pin.m_Ccol ), true ) )
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
                    ibisReport( "[Pin]: Invalid field name", RPT_SEVERITY_ERROR );
                    status = false;
                }
            }

            if( pin.m_Rcol == 0 || pin.m_Lcol == 0 || pin.m_Ccol == 0 )
            {
                ibisReport( "[Pin]: Missing argument", RPT_SEVERITY_ERROR );
                status = false;
            }
            pin.m_dummy = true;
        }
    }

    m_currentComponent->m_pins.push_back( pin );
    m_continue = IBIS_PARSER_CONTINUE::COMPONENT_PIN;


    return true;
}


bool IbisParser::readPinMapping()
{
    bool status = true;

    std::vector<std::string> fields;

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
                ibisReport( "[Pin Mapping]: wrong number of columns", RPT_SEVERITY_ERROR );
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


bool IbisParser::readDiffPin()
{
    bool status = true;

    IbisDiffPinEntry entry;

    if( m_continue == IBIS_PARSER_CONTINUE::NONE ) // No info on first line
    {
        m_continue = IBIS_PARSER_CONTINUE::COMPONENT_DIFFPIN;
    }
    else
    {
        if( readWord( &( entry.pinA ) ) )
        {
            if( readWord( &( entry.pinB ) ) )
            {
                if( readDouble( &( entry.Vdiff ) ) )
                {
                    if( readTypMinMaxValue( &( entry.tdelay ) ) )
                    {
                    }
                    m_currentComponent->m_diffPin.m_entries.push_back( entry );
                }
                else
                {
                    ibisReport( "[Pin Diff]: Incorrect vdiff", RPT_SEVERITY_ERROR );
                }
            }
            else
            {
                std::cout << entry.pinB << std::endl;

                ibisReport( "[Pin Diff]: Incorrect inv_pin", RPT_SEVERITY_ERROR );
            }
        }
        else
        {
            ibisReport( "[Pin Diff]: Incorrect pin name", RPT_SEVERITY_ERROR );
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
        std::string str;
        if( readWord( &str ) )
        {
            if( parseDouble( &( entry.V ), str, true ) )
            {
                if( readTypMinMaxValue( &( entry.I ) ) )
                {
                    aDest->m_entries.push_back( entry );
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


    return status;
}

bool IbisParser::readVTtableEntry( VTtable* aDest )
{
    bool status = true;
    skipWhitespaces();

    VTtableEntry entry;

    if( m_lineIndex < m_lineLength )
    {
        std::string str;
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
        if( aDest != nullptr )
        {
            aDest->m_entries.push_back( entry );
        }
        else
        {
            ibisReport( "Internal Error: destination IV table is empty" );
            status = false;
        }
    }
    else
    {
    }


    return status;
}

bool IbisParser::readWaveform( IbisWaveform* aDest, IBIS_WAVEFORM_TYPE aType )
{
    bool status = true;


    IbisWaveform* wf;

    if( m_continue != IBIS_PARSER_CONTINUE::WAVEFORM )
    {
        wf = new IbisWaveform();
        wf->m_type = aType;

        switch( aType )
        {
        case IBIS_WAVEFORM_TYPE::FALLING: m_currentModel->m_fallingWaveforms.push_back( wf ); break;
        case IBIS_WAVEFORM_TYPE::RISING: m_currentModel->m_risingWaveforms.push_back( wf ); break;
        default: ibisReport( "Unknown waveform type", RPT_SEVERITY_ERROR ); status = false;
        }
    }
    else
    {
        if( aDest != nullptr )
        {
            wf = aDest;
        }
    }


    if( status )
    {
        if( isLineEmptyFromCursor() )
        {
            std::cout << "That line was empty" << std::endl;
        }
        else if( readNumericSubparam( std::string( "R_fixture" ), &( wf->m_R_fixture ) ) )
            ;
        else if( readNumericSubparam( std::string( "L_fixture" ), &( wf->m_L_fixture ) ) )
            ;
        else if( readNumericSubparam( std::string( "C_fixture" ), &( wf->m_C_fixture ) ) )
            ;
        else if( readNumericSubparam( std::string( "V_fixture" ), &( wf->m_V_fixture ) ) )
            ;
        else if( readNumericSubparam( std::string( "V_fixture_min" ), &( wf->m_V_fixture_min ) ) )
            ;
        else if( readNumericSubparam( std::string( "V_fixture_max" ), &( wf->m_V_fixture_max ) ) )
            ;
        else if( readNumericSubparam( std::string( "R_dut" ), &( wf->m_R_fixture ) ) )
            ;
        else if( readNumericSubparam( std::string( "L_dut" ), &( wf->m_L_fixture ) ) )
            ;
        else if( readNumericSubparam( std::string( "C_dut" ), &( wf->m_C_fixture ) ) )
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
    m_currentWaveform = wf;
    m_continue = IBIS_PARSER_CONTINUE::WAVEFORM;
    return status;
}

bool IbisParser::onNewLine()
{
    bool      status = true;
    char      c;
    std::string* pKeyword = getKeyword();
    // New line

    if( pKeyword ) // New keyword
    {
        std::string keyword = *pKeyword;

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
        case IBIS_PARSER_CONTEXT::PACKAGEMODEL: status &= parsePackageModel( keyword ); break;
        case IBIS_PARSER_CONTEXT::PACKAGEMODEL_MODELDATA:
            status &= parsePackageModelModelData( keyword );
            break;
        default:
        {
            status = false;
            ibisReport( "Internal error: Bad parser context.", RPT_SEVERITY_ERROR );
        }
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
        case IBIS_PARSER_CONTINUE::COMPONENT_DIFFPIN: status &= readDiffPin(); break;
        case IBIS_PARSER_CONTINUE::MODELSELECTOR: status &= readModelSelector(); break;
        case IBIS_PARSER_CONTINUE::MODEL: status &= readModel(); break;
        case IBIS_PARSER_CONTINUE::IV_TABLE: status &= readIVtableEntry( m_currentIVtable ); break;
        case IBIS_PARSER_CONTINUE::VT_TABLE: status &= readVTtableEntry( m_currentVTtable ); break;
        case IBIS_PARSER_CONTINUE::WAVEFORM:
            status &= readWaveform( m_currentWaveform, m_currentWaveform->m_type );
            break;
        case IBIS_PARSER_CONTINUE::RAMP: status &= readRamp(); break;
        case IBIS_PARSER_CONTINUE::PACKAGEMODEL_PINS: status &= readPackageModelPins(); break;
        case IBIS_PARSER_CONTINUE::MATRIX: status &= readMatrix( &m_currentMatrix ); break;
        case IBIS_PARSER_CONTINUE::NONE:
        default:
            ibisReport( "Missing keyword.", RPT_SEVERITY_ERROR );
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