#ifndef KIBIS_H
#define KIBIS_H

#include "ibisParser.h"

class KIBIS_FILE
{
public:
    wxString m_fileName;
    double   m_fileRev;
    double   m_ibisVersion;

    wxString m_date;
    wxString m_source;
    wxString m_notes;
    wxString m_disclaimer;
    wxString m_copyright;
};

class KIBIS_ANY
{
public:
    KIBIS_FILE* m_file;
};


class KIBIS_MODEL : public KIBIS_ANY
{
public:
    wxString        m_name;
    wxString        m_description;
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

    bool HasPulldown();
    bool HasPullup();

    bool writeSpiceDriver( wxString* aDest );

    //private:
    std::vector<std::pair<IbisWaveform*, IbisWaveform*>> waveformPairs();
    wxString generateSquareWave( wxString aNode1, wxString aNode2, double aTon, double aToff,
                                 int aCycles, std::pair<IbisWaveform*, IbisWaveform*> aPair );
};

class KIBIS_PIN : public KIBIS_ANY
{
public:
    /** @brief Name of the pin
     *  Examples : "VCC", "GPIOA", "CLK", etc...
     */
    wxString m_signalName;
    /** @brief Pin Number
     * Examples : 1, 2, 3 ( or for BGA ), A1, A2, A3, etc...
     */
    wxString m_pinNumber;

    /** @brief Resistance from die to pin */
    TypMinMaxValue R_pin;
    /** @brief Inductance from die to pin */
    TypMinMaxValue L_pin;
    /** @brief Capacitance from pin to GND */
    TypMinMaxValue C_pin;

    std::vector<double> m_t, m_Ku, m_Kd;

    std::vector<KIBIS_MODEL*> m_models;

    bool writeSpiceDriver( wxString* aDest, KIBIS_MODEL* aModel );
    wxString getKuKdOneWaveform( KIBIS_MODEL*                            aModel,
                                 std::pair<IbisWaveform*, IbisWaveform*> aPair );
};

class KIBIS_COMPONENT : public KIBIS_ANY
{
public:
    /** @brief Name of the component */
    wxString m_name;
    /** @brief Name of the manufacturer */
    wxString m_manufacturer;

    std::vector<KIBIS_PIN*> m_pins;

    KIBIS_PIN* getPin( wxString aPinNumber );
};

bool KibisFromFile( wxString aFileName, std::vector<KIBIS_COMPONENT*>* aDest );

#endif