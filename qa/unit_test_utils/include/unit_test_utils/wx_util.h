/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef UNIT_TEST_UTILS_WX_ASSERT__H
#define UNIT_TEST_UTILS_WX_ASSERT__H

#include <wx/app.h>
#include <wx/image.h>
#include <wx/string.h>
#include <wx/window.h>

#include <exception>
#include <string>

namespace KI_TEST
{

/**
 * An exception class to represent a WX assertion.
 *
 * In normal programs, this is popped as a dialog, but in unit tests, it
 * prints a fairly unhelpful stack trace and otherwise doesn't inform the
 * test runner.
 *
 * We want to raise a formal exception to allow us to catch it with
 * things like BOOST_CHECK_THROW if expected, or for the test case to fail if
 * not expected.
 */
class WX_ASSERT_ERROR : public std::exception
{
public:
    WX_ASSERT_ERROR( const wxString& aFile, int aLine, const wxString& aFunc, const wxString& aCond,
            const wxString& aMsg );

    const char* what() const noexcept override;

    // Public, so catchers can have a look (though be careful as the function
    // names can change over time!)
    std::string m_file;
    int         m_line;
    std::string m_func;
    std::string m_cond;
    std::string m_msg;

    std::string m_format_msg;
};


/**
 * This is base wxApp for use in unit tests.
 *
 * If necessary, a new class should be derived from this and create frame or dialog under test.
 * Constructor contains common initialisation code for wxWidgets.
 */
class TEST_APP_BASE : public wxApp
{
public:
    TEST_APP_BASE()
    {
        wxInitAllImageHandlers();
    }
    virtual ~TEST_APP_BASE() {};

    /**
     * Force paint event on desired window
     */
    void RefreshWindow( wxWindow* aWin );

    /**
     * Remove pending events and destroy window
     */
    void DisposeOfWindow( wxWindow** aWin );
};


/**
 * This is base for fixture to make wxWidgets running in unit tests.
 *
 * If necessary for test, a new class should be derived from this. Constructor should be called
 * with new app allocated. This wxApp object will be freed in destructor of WX_FIXTURE_BASE.
 *
 * Template parameter should be class derived from TEST_APP_BASE.
 */
template <class __TA> class WX_FIXTURE_BASE
{
public:
    WX_FIXTURE_BASE();
    WX_FIXTURE_BASE(__TA *app);
    virtual ~WX_FIXTURE_BASE();

    __TA* App()
    {
        return m_app;
    }

private:
    __TA* m_app;
};


template <class __TA>
WX_FIXTURE_BASE<__TA>::WX_FIXTURE_BASE() :
    WX_FIXTURE_BASE( new __TA() )
{
}


template <class __TA>
WX_FIXTURE_BASE<__TA>::WX_FIXTURE_BASE(__TA *app) :
    m_app( app )
{
    char appname[] = "wxUnitTest.exe";
    int argc = 1;
    char *argv[1] = {appname};

    wxApp::SetInstance( m_app );
    wxEntryStart( argc, argv );
    m_app->OnInit();
    m_app->ProcessPendingEvents();
    m_app->SafeYield( nullptr, true );
}


template <class __TA>
WX_FIXTURE_BASE<__TA>::~WX_FIXTURE_BASE()
{
    m_app->OnExit();
    m_app->DeletePendingEvents();
    wxEntryCleanup();
    wxApp::SetInstance(nullptr);
}


} // namespace KI_TEST

#endif // UNIT_TEST_UTILS_WX_ASSERT__H
