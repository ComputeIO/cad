/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <eeschema_test_utils.h>
#include <sim/sim_library_spice.h>


class TEST_SIM_LIBRARY_SPICE_FIXTURE
{
public:
    wxString GetLibraryPath( const wxString& aBaseName )
    {
        wxFileName fn = KI_TEST::GetEeschemaTestDataDir();
        fn.AppendDir( "spice_netlists" );
        fn.AppendDir( "libraries" );
        fn.SetName( aBaseName );
        fn.SetExt( "lib.spice" );

        return fn.GetFullPath();
    }

    void LoadLibrary( const wxString& aBaseName )
    {
        wxString path = GetLibraryPath( aBaseName );
        m_library = std::make_unique<SIM_LIBRARY_SPICE>();
        BOOST_CHECK( m_library->ReadFile( path ) );
    }

    void CompareToUsualDiodeModel( const SIM_MODEL& aModel, const wxString& aModelName, int aModelIndex )
    {

        BOOST_CHECK_EQUAL( aModelName, aModel.GetSpiceInfo().modelType.Upper()
                                       + wxString::FromCDouble( aModelIndex )
                                       + "_Usual" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "bv" )->value->ToString(), "1.1u" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "cjo" )->value->ToString(), "2.2m" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "ibv" )->value->ToString(), "3.3" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "is" )->value->ToString(), "4.4k" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "m" )->value->ToString(), "5.5M" );
        BOOST_CHECK_EQUAL( aModel.FindParam( "n" )->value->ToString(), "6.6G" );
    }

    void CompareToEmptyModel( const SIM_MODEL& aModel, const wxString& aModelName, int aModelIndex )
    {
        BOOST_CHECK_EQUAL( aModelName, aModel.GetSpiceInfo().modelType.Upper()
                                       + wxString::FromCDouble( aModelIndex )
                                       + "_Empty" );

        for( unsigned i = 0; i < aModel.GetParamCount(); ++i )
        {
            BOOST_CHECK_EQUAL( aModel.GetUnderlyingParam( i ).value->ToString(), "" );
        }
    }

    void TestBjt( const SIM_MODEL& aModel, const wxString& aModelName, int aModelIndex,
                  SIM_MODEL::TYPE aType, const std::vector<wxString>& aParamNames )
    {
        BOOST_CHECK_EQUAL( aModelName, aModel.GetSpiceInfo().modelType.Upper()
                                       + wxString::FromCDouble( aModelIndex )
                                       + "_" + aModel.GetTypeInfo().fieldValue );

        for( int i = 0; i < aParamNames.size(); ++i )
        {
            wxString paramName = aParamNames.at( i );

            if( i == 0 )
            {
                BOOST_CHECK_EQUAL( aModel.FindParam( paramName )->value->ToString(), "0" );
            }
            else
            {
                BOOST_CHECK_EQUAL( aModel.FindParam( paramName )->value->ToString(),
                                   wxString::FromCDouble( i ) + ".0000"
                                   + wxString::FromCDouble( i ) + "G" );
            }
        }
    }

    std::unique_ptr<SIM_LIBRARY_SPICE> m_library;
};


BOOST_FIXTURE_TEST_SUITE( SimLibrarySpice, TEST_SIM_LIBRARY_SPICE_FIXTURE )


BOOST_AUTO_TEST_CASE( Diodes )
{
    LoadLibrary( "diodes" );

    const std::vector<std::reference_wrapper<SIM_MODEL>> models = m_library->GetModels();
    const std::vector<wxString>& modelNames = m_library->GetModelNames();

    BOOST_CHECK_EQUAL( models.size(), 18 );

    for( int i = 0; i < models.size(); ++i )
    {
        const SIM_MODEL& model = models.at( i );
        const wxString& modelName = modelNames.at( i );

        switch( i )
        {
            case 0:
                BOOST_CHECK_EQUAL( modelName, "1N4148" );
                BOOST_CHECK_EQUAL( model.FindParam( "bv" )->value->ToString(), "100" );
                BOOST_CHECK_EQUAL( model.FindParam( "cjo" )->value->ToString(), "4p" );
                BOOST_CHECK_EQUAL( model.FindParam( "ibv" )->value->ToString(), "100u" );
                BOOST_CHECK_EQUAL( model.FindParam( "is" )->value->ToString(), "4n" );
                BOOST_CHECK_EQUAL( model.FindParam( "m" )->value->ToString(), "330m" );
                BOOST_CHECK_EQUAL( model.FindParam( "n" )->value->ToString(), "2" );
                BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value->ToString(), "500m" );
                BOOST_CHECK_EQUAL( model.FindParam( "tt" )->value->ToString(), "10n" );
                BOOST_CHECK_EQUAL( model.FindParam( "vj" )->value->ToString(), "800m" );
                break;

            case 1:
                BOOST_CHECK_EQUAL( modelName, "D1" );
                BOOST_CHECK_EQUAL( model.FindParam( "is" )->value->ToString(), "1.23n" );
                BOOST_CHECK_EQUAL( model.FindParam( "n" )->value->ToString(), "1.23" );
                BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value->ToString(), "789m" );
                BOOST_CHECK_EQUAL( model.FindParam( "ikf" )->value->ToString(), "12.34m" );
                BOOST_CHECK_EQUAL( model.FindParam( "xti" )->value->ToString(), "3" );
                BOOST_CHECK_EQUAL( model.FindParam( "eg" )->value->ToString(), "1.23" );
                BOOST_CHECK_EQUAL( model.FindParam( "cjo" )->value->ToString(), "900f" );
                BOOST_CHECK_EQUAL( model.FindParam( "m" )->value->ToString(), "560m" );
                BOOST_CHECK_EQUAL( model.FindParam( "vj" )->value->ToString(), "780m" );
                BOOST_CHECK_EQUAL( model.FindParam( "fc" )->value->ToString(), "900m" );
                BOOST_CHECK_EQUAL( model.FindParam( "isr" )->value->ToString(), "12.34n" );
                BOOST_CHECK_EQUAL( model.FindParam( "nr" )->value->ToString(), "2.345" );
                BOOST_CHECK_EQUAL( model.FindParam( "bv" )->value->ToString(), "100" );
                BOOST_CHECK_EQUAL( model.FindParam( "ibv" )->value->ToString(), "100u" );
                BOOST_CHECK_EQUAL( model.FindParam( "tt" )->value->ToString(), "12.34n" );
                break;

            case 2:
            case 3:
                CompareToUsualDiodeModel( model, modelName, i );
                break;

            case 4:
                BOOST_CHECK_EQUAL( modelName, "D4" );
                BOOST_CHECK_EQUAL( model.FindParam( "is" )->value->ToString(), "100f" );
                BOOST_CHECK_EQUAL( model.FindParam( "rs" )->value->ToString(), "2" );
                BOOST_CHECK_EQUAL( model.FindParam( "cjo" )->value->ToString(), "3p" );
                BOOST_CHECK_EQUAL( model.FindParam( "tt" )->value->ToString(), "45n" );
                BOOST_CHECK_EQUAL( model.FindParam( "bv" )->value->ToString(), "678" );
                BOOST_CHECK_EQUAL( model.FindParam( "ibv" )->value->ToString(), "100f" );
                break;

            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
                CompareToEmptyModel( model, modelName, i );
                break;

            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
                CompareToUsualDiodeModel( model, modelName, i );
                break;

            default:
                BOOST_FAIL( "Unknown parameter index" );
        }
    }
}


BOOST_AUTO_TEST_CASE( Bjts )
{
    LoadLibrary( "bjts" );

    const std::vector<std::reference_wrapper<SIM_MODEL>> models = m_library->GetModels();
    const std::vector<wxString>& modelNames = m_library->GetModelNames();

    BOOST_CHECK_EQUAL( models.size(), 6 );

    for( int i = 0; i < models.size(); ++i )
    {
        const SIM_MODEL& model = models.at( i );
        const wxString& modelName = modelNames.at( i );

        switch( i )
        {
        case 0:
            TestBjt( model, modelName, i, SIM_MODEL::TYPE::NPN_GUMMELPOON,
                     { "is", "nf", "ise", "ne", "bf", "ikf", "vaf", "nr", "isc", "nc" } );
            break;

        case 1:
            TestBjt( model, modelName, i, SIM_MODEL::TYPE::PNP_GUMMELPOON,
                     { "is", "nf", "ise", "ne", "bf", "ikf", "vaf", "nr", "isc", "nc" } );
            break;

        case 2:
            TestBjt( model, modelName, i, SIM_MODEL::TYPE::NPN_VBIC,
                     { "rcx", "rci", "vo", "gamm", "hrcf", "rbx", "rbi", "re", "rs", "rbp" } );
            break;

        case 3:
            TestBjt( model, modelName, i, SIM_MODEL::TYPE::PNP_VBIC,
                     { "rcx", "rci", "vo", "gamm", "hrcf", "rbx", "rbi", "re", "rs", "rbp" } );
            break;

        case 4:
            TestBjt( model, modelName, i, SIM_MODEL::TYPE::NPN_HICUML2,
                     { "c10", "qp0", "ich", "hf0", "hfe", "hfc", "hjei", "ahjei", "rhjei", "hjci" } );
            break;

        case 5:
            TestBjt( model, modelName, i, SIM_MODEL::TYPE::PNP_HICUML2,
                     { "c10", "qp0", "ich", "hf0", "hfe", "hfc", "hjei", "ahjei", "rhjei", "hjci" } );
            break;
        }
    }
}


BOOST_AUTO_TEST_SUITE_END()
