/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Fabien Corona f.corona<at>laposte.net
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


#ifndef IBIS_PARSER_H
#define IBIS_PARSER_H

#define _( x ) x

#define NAN_NA "1"
#define NAN_INVALID "0"

#define IBIS_MAX_VERSION 7.0      // Up to v7.0, IBIS is fully backward compatible
#define IBIS_MAX_LINE_LENGTH 2048 // official limit is 1024

#include <wx/string.h>
#include <reporter.h>
//#include "common.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <cstring>


class IBIS_REPORTER
{
public:
    /** @brief Print a message
     * 
     * In the future, this function could do more than just printing a message.
     * All KIBIS messages are concentrated at a single point in the code. 
     * 
     * @param aMsg Message
     * @param aSeverity Message sevirity
     */
    void Report( std::string aMsg, SEVERITY aSeverity ) { std::cout << aMsg << std::endl; };
};

class IBIS_ANY
{
public:
    IBIS_ANY( IBIS_REPORTER* aReporter ) { m_reporter = aReporter; };
    IBIS_REPORTER* m_reporter;

    /** @brief Print a message
     * 
     * Call m_reporter->Report if m_reporter exists.
     * 
     * @param aMsg Message
     * @param aSeverity Message sevirity
     */
    void           Report( std::string aMsg, SEVERITY aSeverity = RPT_SEVERITY_INFO )
    {
        if( m_reporter )
        {
            m_reporter->Report( aMsg, aSeverity );
        }
    };
protected:
    /** @brief Convert a double to string using scientific notation
     * 
     * @param aNumber Number
     * @return Output string
     */
    std::string doubleToString( double aNumber );
};


class IBIS_INPUT : public IBIS_ANY
{
public:
    IBIS_INPUT( IBIS_REPORTER* aReporter ) : IBIS_ANY( aReporter ){};
    /** @brief Check if the data held by the object is valid. 
     * 
     * @return true in case of success
     */
    bool virtual Check() { return false; };
};


enum IBIS_CORNER
{
    TYP = 0,
    MIN,
    MAX
};


enum class IBIS_MATRIX_TYPE
{
    // All matrices are supposed to be symmetrical, only upper right triangle is given
    UNDEFINED,
    BANDED, // Give the main diagonal + [bandwith] elements on the right
    SPARSE, // Only give non-zero values.
    FULL,   // Give the whole upper triangle.
};

class IBIS_MATRIX : public IBIS_INPUT
{
public:
    IBIS_MATRIX( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter ){};
    IBIS_MATRIX_TYPE m_type = IBIS_MATRIX_TYPE::UNDEFINED;
    int              m_dim = -5;
    double*          m_data;

};

class IBIS_MATRIX_BANDED : public IBIS_MATRIX
{
public:
    IBIS_MATRIX_BANDED( IBIS_REPORTER* aReporter ) : IBIS_MATRIX( aReporter ){};
    IBIS_MATRIX_TYPE m_type = IBIS_MATRIX_TYPE::BANDED;
    int              m_dim = -2;
    int              m_bandwidth = 0;
    double*          m_data;

    bool Check() override;
};

class IBIS_MATRIX_SPARSE : public IBIS_MATRIX
{
public:
    IBIS_MATRIX_SPARSE( IBIS_REPORTER* aReporter ) : IBIS_MATRIX( aReporter ){};
    IBIS_MATRIX_TYPE m_type = IBIS_MATRIX_TYPE::BANDED;
    int              m_dim = -3;
    double*          m_data;

    bool Check() override;
};


class IBIS_MATRIX_FULL : public IBIS_MATRIX
{
public:
    IBIS_MATRIX_FULL( IBIS_REPORTER* aReporter ) : IBIS_MATRIX( aReporter ){};
    IBIS_MATRIX_TYPE m_type = IBIS_MATRIX_TYPE::FULL;
    int              m_dim = -4;
    double*          m_data;

    bool Check() override;
};


class IBIS_SECTION : public IBIS_INPUT
{
public:
    IBIS_SECTION( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter ){};
};


class IbisHeader : IBIS_SECTION
{
public:
    IbisHeader( IBIS_REPORTER* aReporter ) : IBIS_SECTION( aReporter ){};
    double   m_ibisVersion = -1;
    double   m_fileRevision = -1;
    std::string m_fileName;
    std::string m_source;
    std::string m_date;
    std::string m_notes;
    std::string m_disclaimer;
    std::string m_copyright;

    bool Check() override;
};


class TypMinMaxValue : public IBIS_INPUT
{
public:
    TypMinMaxValue( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter ){};
    double value[3];

    bool Check() override;
};


class IbisComponentPackage : public IBIS_INPUT
{
public:
    IbisComponentPackage( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter )
    {
        m_Rpkg = new TypMinMaxValue( m_reporter );
        m_Lpkg = new TypMinMaxValue( m_reporter );
        m_Cpkg = new TypMinMaxValue( m_reporter );
    };
    TypMinMaxValue* m_Rpkg;
    TypMinMaxValue* m_Lpkg;
    TypMinMaxValue* m_Cpkg;

    bool Check() override;
};


class IbisComponentPin : public IBIS_INPUT
{
public:
    IbisComponentPin( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter ){};

    std::string m_pinName;
    std::string m_signalName;
    std::string m_modelName;
    double   m_Rpin = std::nan( NAN_NA );
    double   m_Lpin = std::nan( NAN_NA );
    double   m_Cpin = std::nan( NAN_NA );

    int m_Rcol = 0;
    int m_Lcol = 0;
    int m_Ccol = 0;

    bool Check() override;

    bool m_dummy = false;
};


class IbisComponentPinMapping : public IBIS_INPUT
{
public:
    IbisComponentPinMapping( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter ){};
    std::string m_pinName;
    std::string m_PDref;
    std::string m_PUref;
    std::string m_GNDClampRef;
    std::string m_POWERClampRef;
    std::string m_extRef;

    bool m_virtual;
};


class IbisDiffPinEntry : public IBIS_INPUT
{
public:
    IbisDiffPinEntry( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter )
    {
        tdelay = new TypMinMaxValue( aReporter );
    };

    std::string     pinA;
    std::string     pinB;
    double          Vdiff = 0.2; // ignored for input
    TypMinMaxValue* tdelay;      // ignored for outputs
};


class IbisDiffPin : IBIS_INPUT
{
public:
    IbisDiffPin( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter ){};
    std::vector<IbisDiffPinEntry*> m_entries;
};

class IbisComponent : public IBIS_INPUT
{
public:
    IbisComponent( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter )
    {
        m_package = new IbisComponentPackage( m_reporter );
        m_diffPin = new IbisDiffPin( m_reporter );
    };

    std::string                             m_name = "";
    std::string                             m_manufacturer = "";
    IbisComponentPackage*                   m_package;
    std::vector<IbisComponentPin*>          m_pins;
    std::vector<IbisComponentPinMapping*>   m_pinMappings;
    std::string                             m_packageModel;
    std::string                             m_busLabel;
    std::string                             m_dieSupplyPads;
    IbisDiffPin*                            m_diffPin;

    bool Check() override;
};


class IbisModelSelectorEntry
{
public:
    std::string m_modelName;
    std::string m_modelDescription;
};


class IbisModelSelector : public IBIS_INPUT
{
public:
    IbisModelSelector( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter ){};
    std::string                            m_name;
    std::vector<IbisModelSelectorEntry> m_models;

    bool Check() override;
};


class IVtableEntry : public IBIS_INPUT
{
public:
    IVtableEntry( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter )
    {
        I = new TypMinMaxValue( m_reporter );
    };
    double         V;
    TypMinMaxValue* I;
};


class IVtable : public IBIS_INPUT
{
public:
    IVtable( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter ){};
    std::vector<IVtableEntry*> m_entries;

    bool Check() override;

    /** @brief Interpolate the IV table
     * 
     * Linear interpolation to find the current for voltage aV
     * 
     * @param aV voltage
     * @param aCorner Power supply corner
     * @return current
     */
    double   InterpolatedI( double aV, IBIS_CORNER aCorner );

    /** @brief Interpolate the IV table
     * 
     * Generate the spice directive needed to define a model defined by its IV table.
     * The order of aPort1 and aPort2 is important. ( Inverting them will reverse the component )
     * 
     * @param aN Index of the 'a' device
     * @param aPort1 Spice node
     * @param aPort2 Spice node
     * @param aPort2 Name of the generated model
     * @param aCorner Power supply corner
     * @return Multline spice directives
     */
    std::string Spice( int aN, std::string aPort1, std::string aPort2, std::string aModelName,
                    IBIS_CORNER aCorner );

private:
};

class VTtableEntry : public IBIS_INPUT
{
public:
    VTtableEntry( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter )
    {
        V = new TypMinMaxValue( m_reporter );
    };
    double         t;
    TypMinMaxValue* V;
};

class VTtable : public IBIS_INPUT
{
public:
    VTtable( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter ){};
    std::vector<VTtableEntry*> m_entries;
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

class dvdtTypMinMax : public IBIS_INPUT
{
public:
    dvdtTypMinMax( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter ){};
    dvdt value[3];

    bool Check() override;
};


class IbisRamp : public IBIS_INPUT
{
public:
    IbisRamp( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter )
    {
        m_falling = new dvdtTypMinMax( m_reporter );
        m_rising = new dvdtTypMinMax( m_reporter );
    };

    dvdtTypMinMax* m_falling;
    dvdtTypMinMax* m_rising;
    double m_Rload = 50; // The R_load subparameter is optional if the default 50 ohm load is used

    bool Check() override;
};

enum class IBIS_WAVEFORM_TYPE
{
    RISING,
    FALLING
};

class IbisWaveform : public IBIS_INPUT
{
public:
    IbisWaveform( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter )
    {
        m_table = new VTtable( m_reporter );
    };
    VTtable*           m_table;
    IBIS_WAVEFORM_TYPE m_type;
    double             m_R_dut;
    double             m_C_dut;
    double             m_L_dut;
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

class IbisModel : IBIS_INPUT
{
public:
    IbisModel( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter )
    {
        m_C_comp = new TypMinMaxValue( m_reporter );
        m_voltageRange = new TypMinMaxValue( m_reporter );
        m_temperatureRange = new TypMinMaxValue( m_reporter );
        m_pullupReference = new TypMinMaxValue( m_reporter );
        m_pulldownReference = new TypMinMaxValue( m_reporter );
        m_GNDClampReference = new TypMinMaxValue( m_reporter );
        m_POWERClampReference = new TypMinMaxValue( m_reporter );
        m_Rgnd = new TypMinMaxValue( m_reporter );
        m_Rpower = new TypMinMaxValue( m_reporter );
        m_Rac = new TypMinMaxValue( m_reporter );
        m_Cac = new TypMinMaxValue( m_reporter );
        m_GNDClamp = new IVtable( m_reporter );
        m_POWERClamp = new IVtable( m_reporter );
        m_pullup = new IVtable( m_reporter );
        m_pulldown = new IVtable( m_reporter );
        m_ramp = new IbisRamp( m_reporter );
    };

    std::string        m_name;
    IBIS_MODEL_TYPE m_type = IBIS_MODEL_TYPE::UNDEFINED;
    /* The Polarity, Enable, Vinl, Vinh, Vmeas, Cref, Rref, and Vref subparameters are optional. */
    /* the default values of Vinl = 0.8 V and Vinh = 2.0 V are assumed. */
    double              m_vinl = 0.8;
    double              m_vinh = 2;
    double              m_vref = 0;
    double              m_rref = 0;
    double              m_cref = 0;
    double              m_vmeas = 0;
    IBIS_MODEL_ENABLE   m_enable = IBIS_MODEL_ENABLE::UNDEFINED;
    IBIS_MODEL_POLARITY m_polarity = IBIS_MODEL_POLARITY::UNDEFINED;
    // End of optional subparameters

    TypMinMaxValue*            m_C_comp;
    TypMinMaxValue*            m_voltageRange;
    TypMinMaxValue*            m_temperatureRange;
    TypMinMaxValue*            m_pullupReference;
    TypMinMaxValue*            m_pulldownReference;
    TypMinMaxValue*            m_GNDClampReference;
    TypMinMaxValue*            m_POWERClampReference;
    TypMinMaxValue*            m_Rgnd;
    TypMinMaxValue*            m_Rpower;
    TypMinMaxValue*            m_Rac;
    TypMinMaxValue*            m_Cac;
    IVtable*                   m_GNDClamp;
    IVtable*                   m_POWERClamp;
    IVtable*                   m_pullup;
    IVtable*                   m_pulldown;
    std::vector<IbisWaveform*> m_risingWaveforms;
    std::vector<IbisWaveform*> m_fallingWaveforms;
    IbisRamp*                  m_ramp;

    bool Check() override;
};


class IbisPackageModel : public IBIS_INPUT
{
public:
    IbisPackageModel( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter ){};

    std::string              m_name;
    std::string              m_manufacturer;
    std::string              m_OEM;
    std::string              m_description;
    int                   m_numberOfPins;
    std::vector<std::string> m_pins;

    IBIS_MATRIX* m_resistanceMatrix;
    IBIS_MATRIX* m_capacitanceMatrix;
    IBIS_MATRIX* m_inductanceMatrix;

    bool Check() override;
};

class IbisFile : public IBIS_INPUT
{
public:
    IbisFile( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter )
    {
        m_header = new IbisHeader( m_reporter );
    };

    IbisHeader*                     m_header;
    std::vector<IbisComponent*>     m_components;
    std::vector<IbisModelSelector*> m_modelSelectors;
    std::vector<IbisModel*>         m_models;
    std::vector<IbisPackageModel*>  m_packageModels;
};


enum class IBIS_PARSER_CONTINUE
{
    NONE,
    STRING,
    COMPONENT_PACKAGE,
    COMPONENT_PINMAPPING,
    COMPONENT_DIFFPIN,
    COMPONENT_DIESUPPLYPADS,
    COMPONENT_PIN,
    MATRIX,
    MODELSELECTOR,
    MODEL,
    IV_TABLE,
    VT_TABLE,
    RAMP,
    WAVEFORM,
    PACKAGEMODEL_PINS
};

enum class IBIS_PARSER_CONTEXT
{
    HEADER,
    COMPONENT,
    MODELSELECTOR,
    MODEL,
    PACKAGEMODEL,
    PACKAGEMODEL_MODELDATA,
    END
};

class IbisParser : public IBIS_INPUT
{
public:
    IbisParser( IBIS_REPORTER* aReporter ) : IBIS_INPUT( aReporter ), m_ibisFile( aReporter ){};

    bool m_parrot = false; // Write back all lines.

    long  m_lineCounter;
    char  m_commentChar = '|';
    char* m_buffer;
    int   m_bufferIndex;
    char* m_line;
    int   m_lineIndex;
    int   m_lineLength;

    IbisFile           m_ibisFile;
    IbisComponent*     m_currentComponent;
    IbisModelSelector* m_currentModelSelector;
    IbisModel*         m_currentModel;
    IbisPackageModel*  m_currentPackageModel;
    IBIS_MATRIX*       m_currentMatrix;
    int                m_currentMatrixRow;
    int                m_currentMatrixRowIndex;
    IVtable*           m_currentIVtable;
    VTtable*           m_currentVTtable;
    IbisWaveform*      m_currentWaveform;

    /** @brief Parse a file
     * 
     * This is the entry point to parse a file
     * 
     * @param aFileName input file name
     * @return True in case of success
     */
    bool parseFile( std::string& aFileName );
    /** @brief Parse a single keyword in the header context
     * 
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parseHeader( std::string& aKeyword );
    /** @brief Parse a single keyword in the component context
     * 
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parseComponent( std::string& aKeyword );
    /** @brief Parse a single keyword in the component context
     * 
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parseModelSelector( std::string& aKeyword );
    /** @brief Parse a single keyword in the model selector context
     * 
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parseModel( std::string& aKeyword );
    /** @brief Parse a single keyword in the model context
     * 
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parsePackageModel( std::string& aKeyword );
    /** @brief Parse a single keyword in the package model context
     * 
     * @param aKeyword Keyword
     * @return True in case of success
     */
    bool parsePackageModelModelData( std::string& );
    /** @brief Parse a double according to the ibis standard
     * 
     * @param aDest Where the double should be stored
     * @param aStr The string to parse
     * @param aAllowModifiers Allows modifiers ( p for pico, f for femto, k for kilo, ... )
     * @return True in case of success
     */
    bool parseDouble( double& aDest, std::string& aStr, bool aAllowModifiers = false );

private:
    std::string* m_continuingString;

    /** @brief Parse the current line
     *
     * @return True in case of success
     */
    bool onNewLine(); // Gets rid of comments ( except on a comment character change command...)
    /** @brief Load the next line
     *
     * @return True in case of success
     */
    bool GetNextLine();
    /** @brief Print the current line */
    void PrintLine();

    void      skipWhitespaces();
    bool      checkEndofLine(); // To be used when there cannot be any character left on the line
    bool      isLineEmptyFromCursor();
    std::string getKeyword();

    bool readInt( int& aDest );
    bool readDouble( double& aDest );
    bool readWord( std::string& aDest );
    bool readDvdt( std::string& aString, dvdt& aDest );
    bool readMatrix( IBIS_MATRIX* aDest );
    bool readMatrixBanded( std::string, IBIS_MATRIX_BANDED& aDest );
    bool readMatrixFull( std::string, IBIS_MATRIX_FULL& aDest );
    bool readMatrixSparse( std::string, IBIS_MATRIX_SPARSE& aDest );
    bool readRampdvdt( dvdtTypMinMax& aDest );
    bool readRamp();
    bool readWaveform( IbisWaveform* aDest, IBIS_WAVEFORM_TYPE aType );
    bool readString( std::string& aDest );
    bool StoreString( std::string& aDest, bool aMultiline );
    bool ReadTableLine( std::vector<std::string>& aDest );

    bool readNumericSubparam( std::string aSubparam, double& aDest );
    bool readIVtableEntry( IVtable& aTable );
    bool readVTtableEntry( VTtable& aTable );
    bool readTypMinMaxValue( TypMinMaxValue& aDest );
    bool readTypMinMaxValueSubparam( std::string aSubparam, TypMinMaxValue& aDest );
    //bool ReadDieSupplyPads();

    bool readPackage();
    bool readPin();
    bool readPinMapping();
    bool readDiffPin();
    bool readModelSelector();
    bool readModel();
    bool readPackageModelPins();

    /** @brief Ibis can change the character used for comments */
    bool ChangeCommentChar();
    bool changeContext( std::string& aKeyword );

    IBIS_PARSER_CONTINUE m_continue = IBIS_PARSER_CONTINUE::NONE;
    IBIS_PARSER_CONTEXT  m_context = IBIS_PARSER_CONTEXT::HEADER;

    // To be removed
    /** @brief Basic string->double conversion
     * 
     * @param aString input string
     * @param aDest destination
     * @return True in case of success
     */
    bool ibisToDouble( std::string& aString, double& aDest );
};

#endif