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


#ifndef KIBIS_H
#define KIBIS_H

#include "ibisParser.h"

class KIBIS_PIN;
class KIBIS_FILE;
class KIBIS_MODEL;
class KIBIS_COMPONENT;
class KIBIS;

class KIBIS_ANY
{
public:
    KIBIS_ANY( KIBIS* aTopLevel ) { m_topLevel = aTopLevel, m_valid = false; };
    KIBIS* m_topLevel;
    bool   m_valid;
};

enum KIBIS_WAVEFORM_TYPE
{
    NONE = 0, // Used for three state
    RECTANGULAR,
    STUCK_HIGH,
    STUCK_LOW,
    HIGH_Z
};


class KIBIS_WAVEFORM
{
public:
    KIBIS_WAVEFORM(){};
    KIBIS_WAVEFORM_TYPE GetType() { return m_type; };

protected:
    KIBIS_WAVEFORM_TYPE m_type = KIBIS_WAVEFORM_TYPE::NONE;
};

class KIBIS_WAVEFORM_RECTANGULAR : public KIBIS_WAVEFORM
{
public:
    KIBIS_WAVEFORM_RECTANGULAR() : KIBIS_WAVEFORM() { m_type = KIBIS_WAVEFORM_TYPE::RECTANGULAR; };
    double m_ton;
    double m_toff;
    int    m_cycles;
};

class KIBIS_WAVEFORM_STUCK_HIGH : public KIBIS_WAVEFORM
{
public:
    KIBIS_WAVEFORM_STUCK_HIGH() : KIBIS_WAVEFORM() { m_type = KIBIS_WAVEFORM_TYPE::STUCK_HIGH; };
};

class KIBIS_WAVEFORM_STUCK_LOW : public KIBIS_WAVEFORM
{
public:
    KIBIS_WAVEFORM_STUCK_LOW() : KIBIS_WAVEFORM() { m_type = KIBIS_WAVEFORM_TYPE::STUCK_LOW; };
};

class KIBIS_WAVEFORM_HIGH_Z : public KIBIS_WAVEFORM
{
public:
    KIBIS_WAVEFORM_HIGH_Z() : KIBIS_WAVEFORM() { m_type = KIBIS_WAVEFORM_TYPE::HIGH_Z; };
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

class KIBIS : public KIBIS_ANY
{
public:
    KIBIS( std::string aFileName );
    std::vector<KIBIS_COMPONENT*> m_components;
    std::vector<KIBIS_MODEL*>     m_models;
    KIBIS_FILE*                   m_file;
};

class KIBIS_FILE : KIBIS_ANY
{
public:
    KIBIS_FILE( KIBIS* aTopLevel, IbisParser* aParser );

    std::string m_fileName;
    double   m_fileRev;
    double   m_ibisVersion;

    std::string m_date;
    std::string m_source;
    std::string m_notes;
    std::string m_disclaimer;
    std::string m_copyright;
};

class KIBIS_MODEL : public KIBIS_ANY
{
public:
    KIBIS_MODEL( KIBIS* aTopLevel, IbisModel* aSource, IbisParser* aParser );

    std::string        m_name;
    std::string        m_description;
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

    std::string SpiceDie( IBIS_CORNER aSupply, IBIS_CORNER aSpeed );

    //private:
    std::vector<std::pair<IbisWaveform*, IbisWaveform*>> waveformPairs();
    std::string generateSquareWave( std::string aNode1, std::string aNode2,
                                 KIBIS_WAVEFORM_RECTANGULAR*             aWave,
                                 std::pair<IbisWaveform*, IbisWaveform*> aPair,
                                 IBIS_CORNER                             aCorner );
};

class KIBIS_PIN : public KIBIS_ANY
{
public:
    KIBIS_PIN( KIBIS* aTopLevel, IbisComponentPin* aPin, IbisComponentPackage* aPackage,
               IbisParser* aParser, std::vector<KIBIS_MODEL*> aModels );
    /** @brief Name of the pin
     *  Examples : "VCC", "GPIOA", "CLK", etc...
     */
    std::string m_signalName;
    /** @brief Pin Number
     * Examples : 1, 2, 3 ( or for BGA ), A1, A2, A3, etc...
     */
    std::string m_pinNumber;

    /** @brief Resistance from die to pin */
    TypMinMaxValue* R_pin;
    /** @brief Inductance from die to pin */
    TypMinMaxValue* L_pin;
    /** @brief Capacitance from pin to GND */
    TypMinMaxValue* C_pin;

    std::vector<double> m_t, m_Ku, m_Kd;

    std::vector<KIBIS_MODEL*> m_models;

    bool     writeSpiceDriver( std::string* aDest, std::string aName, KIBIS_MODEL* aModel,
                               IBIS_CORNER aSupply, IBIS_CORNER aSpeed, KIBIS_ACCURACY aAccuracy,
                               KIBIS_WAVEFORM* aWave );
    bool     writeSpiceDevice( std::string* aDest, std::string aName, KIBIS_MODEL* aModel,
                               IBIS_CORNER aSupply, IBIS_CORNER aSpeed );
    void     getKuKdNoWaveform( KIBIS_MODEL* aModel, KIBIS_WAVEFORM* aWave, IBIS_CORNER aSupply );
    std::string getKuKdOneWaveform( KIBIS_MODEL*                            aModel,
                                    std::pair<IbisWaveform*, IbisWaveform*> aPair,
                                    KIBIS_WAVEFORM* aWave, IBIS_CORNER aSupply,
                                    IBIS_CORNER aSpeed );
    std::string getKuKdTwoWaveforms( KIBIS_MODEL*                            aModel,
                                     std::pair<IbisWaveform*, IbisWaveform*> aPair1,
                                     std::pair<IbisWaveform*, IbisWaveform*> aPair2,
                                     KIBIS_WAVEFORM* aWave, IBIS_CORNER aSupply,
                                     IBIS_CORNER aSpeed );

    std::string KuKdDriver( KIBIS_MODEL* aModel, std::pair<IbisWaveform*, IbisWaveform*> aPair,
                            KIBIS_WAVEFORM* aWave, IBIS_CORNER aSupply, IBIS_CORNER aSpeed,
                            int index );
    std::string addDie( KIBIS_MODEL* aModel, IBIS_CORNER aSupply, int aIndex );
    void     getKuKdFromFile( std::string* aSimul );
};

class KIBIS_COMPONENT : public KIBIS_ANY
{
public:
    KIBIS_COMPONENT( KIBIS* aToplevel, IbisComponent* aSource, IbisParser* aParser );
    /** @brief Name of the component */
    std::string m_name;
    /** @brief Name of the manufacturer */
    std::string m_manufacturer;

    std::vector<KIBIS_PIN*> m_pins;

    KIBIS_PIN* getPin( std::string aPinNumber );
};

#endif