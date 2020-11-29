/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * @file
 * Test suite for SCH_SHEET
 */

#include <unit_test_utils/unit_test_utils.h>
#include <unit_test_utils/hippomocks.h>
#include <unit_test_utils/wx_util.h>
#include <wx/string.h>
#include <string>
#include <sim_types.h>

// Code under test
#include <dialog_sim_settings.h>

//Mocked code
#include <netlist_exporter_pspice_sim.h>


//Local and forward declarations
using namespace KI_TEST;
class TEST_DIALOG_SIM_SETTINGS_FIXTURE;

struct DCParams
{
    bool enable;
    std::string src, start, stop, inc;
};

class TEST_DIALOG_SIM_SETTINGS : private DIALOG_SIM_SETTINGS
{
public:
    TEST_DIALOG_SIM_SETTINGS( wxWindow* aParent ) : DIALOG_SIM_SETTINGS( aParent ) {};

    void SetDC( struct DCParams* sweep1, struct DCParams* sweep2 )
    {
        m_simPages->ChangeSelection( m_simPages->FindPage( m_pgDC ) );
        if( sweep1 )
        {
            m_dcEnable1->SetValue( sweep1->enable );
            m_dcSource1->SetValue( sweep1->src );
            m_dcStart1->SetValue( sweep1->start );
            m_dcStop1->SetValue( sweep1->stop );
            m_dcIncr1->SetValue( sweep1->inc );
        }
        if( sweep2 )
        {
            m_dcEnable2->SetValue( sweep2->enable );
            m_dcSource2->SetValue( sweep2->src );
            m_dcStart2->SetValue( sweep2->start );
            m_dcStop2->SetValue( sweep2->stop );
            m_dcIncr2->SetValue( sweep2->inc );
        }
    }

    void CheckDC( struct DCParams* sweep1, struct DCParams* sweep2 )
    {
        m_simPages->ChangeSelection( m_simPages->FindPage( m_pgDC ) );
        if( sweep1 )
        {
            BOOST_CHECK( m_dcEnable1->IsChecked() == sweep1->enable );
            BOOST_CHECK_EQUAL( m_dcSource1->GetValue().ToStdString().compare( sweep1->src ), 0 );
            BOOST_CHECK_EQUAL( m_dcStart1->GetValue().ToStdString().compare( sweep1->start ), 0 );
            BOOST_CHECK_EQUAL( m_dcStop1->GetValue().ToStdString().compare( sweep1->stop ), 0 );
            BOOST_CHECK_EQUAL( m_dcIncr1->GetValue().ToStdString().compare( sweep1->inc ), 0 );
        }
        else
            BOOST_CHECK( m_dcEnable1->IsChecked() == false );

        if( sweep2 )
        {
            BOOST_CHECK( m_dcEnable2->IsChecked() == sweep2->enable );
            BOOST_CHECK_EQUAL( m_dcSource2->GetValue().ToStdString().compare( sweep2->src ), 0 );
            BOOST_CHECK_EQUAL( m_dcStart2->GetValue().ToStdString().compare( sweep2->start ), 0 );
            BOOST_CHECK_EQUAL( m_dcStop2->GetValue().ToStdString().compare( sweep2->stop ), 0 );
            BOOST_CHECK_EQUAL( m_dcIncr2->GetValue().ToStdString().compare( sweep2->inc ), 0 );
        }
        else
            BOOST_CHECK( m_dcEnable2->IsChecked() == false );
    }

    bool FromWindow()
    {
        return TransferDataFromWindow();
    }

    bool ToWindow()
    {
        return TransferDataToWindow();
    }

    void SetNetlistExporter( NETLIST_EXPORTER_PSPICE_SIM* aExporter )
    {
        DIALOG_SIM_SETTINGS::SetNetlistExporter( aExporter );
    }

    void CleanUp()
    {
        size_t len;
        wxTextCtrl* textCtrls[] = { m_customTxt,
            m_dcStart1, m_dcStart2, m_dcStop1, m_dcStop2, m_dcIncr1, m_dcIncr2 };
        len = sizeof( textCtrls ) / sizeof( wxTextCtrl* );
        for( int i = 0; i < len; i++ )
            textCtrls[i]->SetValue( wxEmptyString );

        wxCheckBox* checkBoxes[] = { m_dcEnable1, m_dcEnable2 };
        len = sizeof( checkBoxes ) / sizeof( wxCheckBox* );
        for( int i = 0; i < len; i++ )
            checkBoxes[i]->SetValue( false );
    }


    enum SIM_TYPE GetOpenedTab()
    {
        wxPanel* currentPage = dynamic_cast<wxPanel*>( m_simPages->GetCurrentPage() );

        if( currentPage == m_pgDC )
            return ST_DC;
        else if( currentPage == m_pgAC )
            return ST_AC;
        else if( currentPage == m_pgTransient )
            return ST_TRANSIENT;
        else if( currentPage == m_pgOP )
            return ST_OP;
        else
            return ST_UNKNOWN;
    }
};


class TEST_DIALOG_SIM_SETTINGS_FIXTURE : public WX_FIXTURE_BASE<TEST_APP_BASE>
{
public:
    TEST_DIALOG_SIM_SETTINGS_FIXTURE()
    {
        dummy = new wxFrame();
        m_dlgSimSettings = new TEST_DIALOG_SIM_SETTINGS(dummy);
    }

    virtual ~TEST_DIALOG_SIM_SETTINGS_FIXTURE() {};

    wxFrame* dummy;

    ///> Object under test
    TEST_DIALOG_SIM_SETTINGS* m_dlgSimSettings;
};


/**
 * Declare the test suite
 */
BOOST_FIXTURE_TEST_SUITE( DialogSimSettings, TEST_DIALOG_SIM_SETTINGS_FIXTURE )


/**
 * Check default properties
 */
#if 0
BOOST_AUTO_TEST_CASE( Default )
{
    BOOST_CHECK_EQUAL( 0, 1 );
}
#endif

/**
 * Check default properties
 */
#if 0
BOOST_AUTO_TEST_CASE( FromWinDC )
{
    m_dlgSimSettings->SetDC();
    BOOST_CHECK( m_dlgSimSettings->FromWindow() );

}
#endif

#if 1
BOOST_AUTO_TEST_CASE( TransferDataToWindow )
{
    MockRepository mocks;
    NETLIST_EXPORTER_PSPICE_SIM* mockExporter =
            mocks.Mock<NETLIST_EXPORTER_PSPICE_SIM>();
    m_dlgSimSettings->SetNetlistExporter( mockExporter );

    struct _testData
    {
        wxString simDirective;
        enum SIM_TYPE detectedSimType;
        bool retVal;
        struct DCParams src1, src2;
    } testData[] =
    {
        { ".op",              ST_OP, true,
                { false, "", "", "", "" },        { false, "", "", "", "" } },
        // Positive tests - one source
        { ".dc V1 1 10 0.01", ST_DC,  true,
                { true, "V1", "1", "10", "10m" }, { false, "", "", "", "" } },
        { ".dc I2 0.1 1 0.05", ST_DC,  true,
                { true, "I2", "100m", "1", "50m" }, { false, "", "", "", "" } },
        { ".dc R3 1 1k 100", ST_DC,  true,
                { true, "R3", "1", "1k", "100" }, { false, "", "", "", "" } },
        { ".dc TEMP -30 70 2", ST_DC,  true,
                { true, "TEMP", "-30", "70", "2" }, { false, "", "", "", "" } },
        // Positive tests - two sources
        { ".dc V1 1 10 0.01 I2 0.1 1 0.05", ST_DC,  true,
                { true, "V1", "1", "10", "10m" }, { true, "I2", "100m", "1", "50m" } },
        { ".dc R3 1 1k 100 TEMP -30 70 2", ST_DC,  true,
                { true, "R3", "1", "1k", "100" }, { true, "TEMP", "-30", "70", "2" } },
        // Negative tests - one source
        { ".dc V1 1 10", ST_DC,  false,
                { false, "V1", "1", "10", "" }, { false, "", "", "", "" } },
        { ".dc V1 1", ST_DC,  false,
                { false, "V1", "1", "", "" }, { false, "", "", "", "" } },
        { ".dc V1", ST_DC,  false,
                { false, "V1", "", "", "" }, { false, "", "", "", "" } },
        { ".dc", ST_DC,  false,
                { false, "", "", "", "" }, { false, "", "", "", "" } },
        { ".dc I2 ab 1 0.05", ST_DC,  false,
                { false, "I2", "", "1", "50m" }, { false, "", "", "", "" } },
        { ".dc I2 0.1 cd 0.05", ST_DC,  false,
                { false, "I2", "100m", "", "50m" }, { false, "", "", "", "" } },
        { ".dc TEMP -30 70 gh", ST_DC,  false,
                { false, "TEMP", "-30", "70", "" }, { false, "", "", "", "" } },
        // Negative tests - two sources
        { ".dc R3 1 1k 100 TEMP -30 70 ", ST_DC,  false,
                { true, "R3", "1", "1k", "100" }, { false, "TEMP", "-30", "70", "" } },
        { ".dc R3 1 1k 100 TEMP -30  ", ST_DC,  false,
                { true, "R3", "1", "1k", "100" }, { false, "TEMP", "-30", "", "" } },
        { ".dc R3 1 1k 100 TEMP   ", ST_DC,  false,
                { true, "R3", "1", "1k", "100" }, { false, "TEMP", "", "", "" } },
        { ".dc V1 1 10 0.01 I2 XY 1 0.05", ST_DC,  false,
                { true, "V1", "1", "10", "10m" }, { false, "I2", "", "1", "50m" } },
        { ".dc V1 1 10 0.01 I2 0.1 ZW 0.05", ST_DC,  false,
                { true, "V1", "1", "10", "10m" }, { false, "I2", "100m", "", "50m" } },
        { ".dc V1 1 10 0.01 I2 0.1 1 TU", ST_DC,  false,
                { true, "V1", "1", "10", "10m" }, { false, "I2", "100m", "1", "" } },
    };

    for( int i = 0; i < ( sizeof( testData ) / sizeof( struct _testData ) ); i++ )
    {
        m_dlgSimSettings->CleanUp();
        mocks.reset();

        mocks.ExpectCall(mockExporter, NETLIST_EXPORTER_PSPICE_SIM::GetSheetSimCommand)
                .Return( testData[i].simDirective );

        BOOST_CHECK( m_dlgSimSettings->ToWindow() == testData[i].retVal );
        if( testData[i].retVal )
        {
            BOOST_CHECK_EQUAL( m_dlgSimSettings->GetOpenedTab(), testData[i].detectedSimType );
            m_dlgSimSettings->CheckDC( &testData[i].src1, &testData[i].src2 );
        }
    }
}
#endif

BOOST_AUTO_TEST_SUITE_END()
