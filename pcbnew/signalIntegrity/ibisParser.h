#ifndef IBIS_PARSER_H
#define IBIS_PARSER_H


#define NAN_NA "1"
#define NAN_INVALID "0"

#define IBIS_MAX_VERSION 7.0      // Up to v7.0, IBIS is fully backward compatible
#define IBIS_MAX_LINE_LENGTH 2048 // official limit is 1024

#include <wx/string.h>
#include <wx/filename.h>
#include <reporter.h>
//#include "common.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include <cstring>


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
    double min = std::nan( NAN_INVALID );
    double typ = std::nan( NAN_INVALID );
    double max = std::nan( NAN_INVALID );

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
    wxString m_pinName;
    wxString m_signalName;
    wxString m_modelName;
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
    wxString m_pinName;
    wxString m_PDref;
    wxString m_PUref;
    wxString m_GNDClampRef;
    wxString m_POWERClampRef;
    wxString m_extRef;

    bool m_virtual;
};

class IbisDiffPinEntry
{
public:
    wxString       pinA;
    wxString       pinB;
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
    wxString                             m_name = "";
    wxString                             m_manufacturer = "";
    IbisComponentPackage                 m_package;
    std::vector<IbisComponentPin>        m_pins;
    std::vector<IbisComponentPinMapping> m_pinMappings;
    wxString                             m_packageModel;
    wxString                             m_busLabel;
    wxString                             m_dieSupplyPads;
    IbisDiffPin                          m_diffPin;

    bool Check();
};


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
    double InterpolatedI( double aV );

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
    dvdt m_typ;
    dvdt m_min;
    dvdt m_max;

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
    wxString        m_name;
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
    wxString              m_name;
    wxString              m_manufacturer;
    wxString              m_OEM;
    wxString              m_description;
    int                   m_numberOfPins;
    std::vector<wxString> m_pins;

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


    bool parseFile( wxFileName aFileName, IbisFile* );
    bool parseHeader( wxString );
    bool parseComponent( wxString );
    bool parseModelSelector( wxString );
    bool parseModel( wxString );
    bool parsePackageModel( wxString );
    bool parsePackageModelModelData( wxString );
    bool parseDouble( double* aDest, wxString aStr, bool aAllowModifiers = false );

private:
    wxString* m_continuingString;


    bool onNewLine(); // Gets rid of comments ( except on a comment character change command...)
    bool GetNextLine();
    void PrintLine();

    void      skipWhitespaces();
    bool      checkEndofLine(); // To be used when there cannot be any character left on the line
    bool      isLineEmptyFromCursor();
    wxString* getKeyword();
    bool      readInt( int* aDest );
    bool      readDouble( double* aDest );
    bool      readWord( wxString* );
    bool      readDvdt( wxString, dvdt* );
    bool      readMatrix( IBIS_MATRIX** );
    bool      readMatrixBanded( wxString, IBIS_MATRIX_BANDED* );
    bool      readMatrixFull( wxString, IBIS_MATRIX_FULL* );
    bool      readMatrixSparse( wxString, IBIS_MATRIX_SPARSE* );
    bool      readRampdvdt( dvdtTypMinMax* aDest );
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
    bool readDiffPin();
    bool readModelSelector();
    bool readModel();
    bool readPackageModelPins();


    bool ChangeCommentChar();
    bool changeContext( wxString aKeyword );

    IBIS_PARSER_CONTINUE m_continue = IBIS_PARSER_CONTINUE::NONE;
    IBIS_PARSER_CONTEXT  m_context = IBIS_PARSER_CONTEXT::HEADER;

    REPORTER* m_reporter = new STDOUT_REPORTER();
};

#endif