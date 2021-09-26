#ifndef KIBIS_H
#define KIBIS_H

#include "ibisParser.h"


enum KIBIS_WAVEFORM_TYPE
{
    NONE = 0, // Used for three state
    RECTANGULAR
};


class KIBIS_WAVEFORM
{
public:
    KIBIS_WAVEFORM_TYPE GetType() { return m_type; };

private:
    KIBIS_WAVEFORM_TYPE m_type = KIBIS_WAVEFORM_TYPE::NONE;
};

class KIBIS_WAVEFORM_RECTANGULAR : public KIBIS_WAVEFORM
{
public:
    double m_ton;
    double m_toff;
    int    m_cycles;

public:
    KIBIS_WAVEFORM_TYPE GetType() { return m_type; };

private:
    KIBIS_WAVEFORM_TYPE m_type = KIBIS_WAVEFORM_TYPE::RECTANGULAR;
};

/** Accuracy level.
 * 
 * Level 0 is faster, but not as accurate
 * 
 * Level 0 :
 *      - Driver: Don't use waveform
 *      - Driver: Don't use _DUT info
 * Level 1 :
 *      _ Driver: Use up to one waveform
 *      _ Driver: Don't use _DUT info 
 * Level 2 :
 *      _ Driver: Use up to two waveforms
 *      _ Driver: Don't use _DUT info
 *      
 * Level 3 : ( not implemented, fallback to level 2 )
 *      _ Driver: Use up to two waveforms 
 *      _ Driver: Use _DUT info if at least one waveform
*/
enum class KIBIS_ACCURACY
{
    LEVEL_0,
    LEVEL_1,
    LEVEL_2,
    LEVEL_3,
};


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
    bool HasGNDClamp();
    bool HasPOWERClamp();

    wxString SpiceDie( IBIS_CORNER aSupply, IBIS_CORNER aSpeed );

    //private:
    std::vector<std::pair<IbisWaveform*, IbisWaveform*>> waveformPairs();
    wxString generateSquareWave( wxString aNode1, wxString aNode2,
                                 KIBIS_WAVEFORM_RECTANGULAR*             aWave,
                                 std::pair<IbisWaveform*, IbisWaveform*> aPair,
                                 IBIS_CORNER                             aCorner );
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

    bool     writeSpiceDriver( wxString* aDest, wxString aName, KIBIS_MODEL* aModel,
                               IBIS_CORNER aSupply, IBIS_CORNER aSpeed, KIBIS_ACCURACY aAccuracy,
                               KIBIS_WAVEFORM* aWave, KIBIS_WAVEFORM_TYPE aWaveType );
    bool     writeSpiceDevice( wxString* aDest, wxString aName, KIBIS_MODEL* aModel,
                               IBIS_CORNER aSupply, IBIS_CORNER aSpeed );
    void     getKuKdNoWaveform( KIBIS_MODEL* aModel, KIBIS_WAVEFORM* aWave,
                                KIBIS_WAVEFORM_TYPE aWaveType, IBIS_CORNER aSupply );
    wxString getKuKdOneWaveform( KIBIS_MODEL* aModel, std::pair<IbisWaveform*, IbisWaveform*> aPair,
                                 KIBIS_WAVEFORM* aWave, KIBIS_WAVEFORM_TYPE aWaveType,
                                 IBIS_CORNER aSupply, IBIS_CORNER aSpeed );
    wxString getKuKdTwoWaveforms( KIBIS_MODEL*                            aModel,
                                  std::pair<IbisWaveform*, IbisWaveform*> aPair1,
                                  std::pair<IbisWaveform*, IbisWaveform*> aPair2,
                                  KIBIS_WAVEFORM* aWave, KIBIS_WAVEFORM_TYPE aWaveType,
                                  IBIS_CORNER aSupply, IBIS_CORNER aSpeed );

    wxString KuKdDriver( KIBIS_MODEL* aModel, std::pair<IbisWaveform*, IbisWaveform*> aPair,
                         KIBIS_WAVEFORM* aWave, KIBIS_WAVEFORM_TYPE aWaveType, IBIS_CORNER aSupply,
                         IBIS_CORNER aSpeed, int index );
    wxString addDie( KIBIS_MODEL* aModel, IBIS_CORNER aSupply, int aIndex );
    void     getKuKdFromFile( wxString* aSimul );
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