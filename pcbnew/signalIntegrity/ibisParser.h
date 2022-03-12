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

class IBIS_MATRIX
{
public:
    IBIS_MATRIX_TYPE m_type = IBIS_MATRIX_TYPE::UNDEFINED;
    int              m_dim = -5;
    double*          m_data;

    bool Check() { return false; };
};

class IBIS_MATRIX_BANDED : public IBIS_MATRIX
{
public:
    IBIS_MATRIX_TYPE m_type = IBIS_MATRIX_TYPE::BANDED;
    int              m_dim = -2;
    int              m_bandwidth = 0;
    double*          m_data;

    bool Check();
};

class IBIS_MATRIX_SPARSE : public IBIS_MATRIX
{
public:
    IBIS_MATRIX_TYPE m_type = IBIS_MATRIX_TYPE::BANDED;
    int              m_dim = -3;
    double*          m_data;

    bool Check();
};
class IBIS_MATRIX_FULL : public IBIS_MATRIX
{
public:
    IBIS_MATRIX_TYPE m_type = IBIS_MATRIX_TYPE::FULL;
    int              m_dim = -4;
    double*          m_data;

    bool Check();
};


class IbisHeader
{
public:
    double   m_ibisVersion = -1;
    double   m_fileRevision = -1;
    std::string m_fileName;
    std::string m_source;
    std::string m_date;
    std::string m_notes;
    std::string m_disclaimer;
    std::string m_copyright;

    bool Check();
};

class TypMinMaxValue
{
public:
    double value[3];

    bool Check();
};

class IbisComponentPackage
{
public:
    TypMinMaxValue m_Rpkg;
    TypMinMaxValue m_Lpkg;
    TypMinMaxValue m_Cpkg;

    bool Check();
};


class IbisComponentPin
{
public:
    std::string m_pinName;
    std::string m_signalName;
    std::string m_modelName;
    double   m_Rpin = std::nan( NAN_NA );
    double   m_Lpin = std::nan( NAN_NA );
    double   m_Cpin = std::nan( NAN_NA );

    int m_Rcol = 0;
    int m_Lcol = 0;
    int m_Ccol = 0;

    bool Check();

    bool m_dummy = false;
};


class IbisComponentPinMapping
{
public:
    std::string m_pinName;
    std::string m_PDref;
    std::string m_PUref;
    std::string m_GNDClampRef;
    std::string m_POWERClampRef;
    std::string m_extRef;

    bool m_virtual;
};

class IbisDiffPinEntry
{
public:
    std::string       pinA;
    std::string       pinB;
    double         Vdiff = 0.2; // ignored for input
    TypMinMaxValue tdelay;      // ignored for outputs
};

class IbisDiffPin
{
public:
    std::vector<IbisDiffPinEntry> m_entries;
};

class IbisComponent
{
public:
    std::string                             m_name = "";
    std::string                             m_manufacturer = "";
    IbisComponentPackage                 m_package;
    std::vector<IbisComponentPin>        m_pins;
    std::vector<IbisComponentPinMapping> m_pinMappings;
    std::string                             m_packageModel;
    std::string                             m_busLabel;
    std::string                             m_dieSupplyPads;
    IbisDiffPin                          m_diffPin;

    bool Check();
};


class IbisModelSelectorEntry
{
public:
    std::string m_modelName;
    std::string m_modelDescription;
};

class IbisModelSelector
{
public:
    std::string                            m_name;
    std::vector<IbisModelSelectorEntry> m_models;

    bool Check();
};


class IVtableEntry
{
public:
    double         V;
    TypMinMaxValue I;
};

class IVtable
{
public:
    std::vector<IVtableEntry> m_entries;

    bool Check();
    double   InterpolatedI( double aV, IBIS_CORNER aCorner );
    std::string Spice( int aN, std::string aPort1, std::string aPort2, std::string aModelName,
                    IBIS_CORNER aCorner );

private:
};

class VTtableEntry
{
public:
    double         t;
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
    dvdt value[3];

    bool Check();
};


class IbisRamp
{
public:
    dvdtTypMinMax m_falling;
    dvdtTypMinMax m_rising;
    double m_Rload = 50; // The R_load subparameter is optional if the default 50 ohm load is used

    bool Check();
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

class IbisModel
{
public:
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

    TypMinMaxValue m_C_comp;
    TypMinMaxValue m_voltageRange;
    TypMinMaxValue m_temperatureRange;
    TypMinMaxValue m_pullupReference;
    TypMinMaxValue m_pulldownReference;
    TypMinMaxValue m_GNDClampReference;
    TypMinMaxValue m_POWERClampReference;
    TypMinMaxValue m_Rgnd;
    TypMinMaxValue m_Rpower;
    TypMinMaxValue m_Rac;
    TypMinMaxValue m_Cac;
    IVtable        m_GNDClamp;
    IVtable        m_POWERClamp;
    IVtable        m_pullup;
    IVtable        m_pulldown;
    std::vector<IbisWaveform*> m_risingWaveforms;
    std::vector<IbisWaveform*> m_fallingWaveforms;
    IbisRamp       m_ramp;

    bool Check();
};


class IbisPackageModel
{
public:
    std::string              m_name;
    std::string              m_manufacturer;
    std::string              m_OEM;
    std::string              m_description;
    int                   m_numberOfPins;
    std::vector<std::string> m_pins;

    IBIS_MATRIX* m_resistanceMatrix;
    IBIS_MATRIX* m_capacitanceMatrix;
    IBIS_MATRIX* m_inductanceMatrix;

    bool Check();
};

class IbisFile
{
public:
    IbisHeader                     m_header;
    std::vector<IbisComponent>     m_components;
    std::vector<IbisModelSelector> m_modelSelectors;
    std::vector<IbisModel>         m_models;
    std::vector<IbisPackageModel>  m_packageModels;
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

class IbisParser
{
public:
    bool m_parrot = false; // Write back all lines.

    long  m_lineCounter;
    char  m_commentChar = '|';
    char* m_buffer;
    int   m_bufferIndex;
    char* m_line;
    int   m_lineIndex;
    int   m_lineLength;

    IbisFile*          m_ibisFile;
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


    bool parseFile( std::string aFileName, IbisFile* );
    bool parseHeader( std::string );
    bool parseComponent( std::string );
    bool parseModelSelector( std::string );
    bool parseModel( std::string );
    bool parsePackageModel( std::string );
    bool parsePackageModelModelData( std::string );
    bool parseDouble( double* aDest, std::string aStr, bool aAllowModifiers = false );

private:
    std::string* m_continuingString;


    bool onNewLine(); // Gets rid of comments ( except on a comment character change command...)
    bool GetNextLine();
    void PrintLine();

    void      skipWhitespaces();
    bool      checkEndofLine(); // To be used when there cannot be any character left on the line
    bool      isLineEmptyFromCursor();
    std::string* getKeyword();
    bool      readInt( int* aDest );
    bool      readDouble( double* aDest );
    bool      readWord( std::string* );
    bool      readDvdt( std::string, dvdt* );
    bool      readMatrix( IBIS_MATRIX** );
    bool      readMatrixBanded( std::string, IBIS_MATRIX_BANDED* );
    bool      readMatrixFull( std::string, IBIS_MATRIX_FULL* );
    bool      readMatrixSparse( std::string, IBIS_MATRIX_SPARSE* );
    bool      readRampdvdt( dvdtTypMinMax* aDest );
    bool      readRamp();
    bool      readWaveform( IbisWaveform* aDest, IBIS_WAVEFORM_TYPE aType );
    bool      readString( std::string* );
    bool      StoreString( std::string* aDest, bool aMultiline );
    std::vector<std::string> ReadTableLine();

    bool readNumericSubparam( std::string aSubparam, double* aDest );
    bool readIVtableEntry( IVtable* aTable );
    bool readVTtableEntry( VTtable* aTable );
    bool readTypMinMaxValue( TypMinMaxValue* aDest );
    bool readTypMinMaxValueSubparam( std::string aSubparam, TypMinMaxValue* aDest );
    //bool ReadDieSupplyPads();

    bool readPackage();
    bool readPin();
    bool readPinMapping();
    bool readDiffPin();
    bool readModelSelector();
    bool readModel();
    bool readPackageModelPins();


    bool ChangeCommentChar();
    bool changeContext( std::string aKeyword );

    IBIS_PARSER_CONTINUE m_continue = IBIS_PARSER_CONTINUE::NONE;
    IBIS_PARSER_CONTEXT  m_context = IBIS_PARSER_CONTEXT::HEADER;

    REPORTER* m_reporter = new STDOUT_REPORTER();

    // To be removed
    void ibisReport( std::string aMsg, SEVERITY aSeverity=RPT_SEVERITY_INFO );
    bool ibisToDouble( std::string aString, double* aDest );
};

#endif