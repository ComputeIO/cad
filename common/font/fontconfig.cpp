/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <font/fontconfig.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>

using namespace fontconfig;

static FONTCONFIG* fc = nullptr;


inline static FcChar8* wxStringToFcChar8( const wxString& str )
{
    wxScopedCharBuffer const fcBuffer = str.ToUTF8();
    return (FcChar8*) fcBuffer.data();
}


FONTCONFIG::FONTCONFIG()
{
    //fontconfig = FcInitLoadConfig();
    fontconfig = FcInitLoadConfigAndFonts();

    wxString configDirPath( Pgm().GetSettingsManager().GetUserSettingsPath() + wxT( "/fonts" ) );
    FcConfigAppFontAddDir( nullptr, wxStringToFcChar8( configDirPath ) );
};


FONTCONFIG& Fontconfig()
{
    if( !fc )
    {
        FcInit();
        fc = new FONTCONFIG();
    }
    return *fc;
}


bool FONTCONFIG::FindFont( const wxString& aFontName, wxString& aFontFile )
{
    FcPattern* pat = FcNameParse( wxStringToFcChar8( aFontName ) );
    FcConfigSubstitute( nullptr, pat, FcMatchPattern );
    FcDefaultSubstitute( pat );

    FcResult   r;
    FcPattern* font = FcFontMatch( nullptr, pat, &r );

    bool ok = false;
    if( font )
    {
        FcChar8* file = nullptr;
        if( FcPatternGetString( font, FC_FILE, 0, &file ) == FcResultMatch )
        {
            aFontFile = wxString::FromUTF8( (char*) file );
            ok = true;
        }
    }

    FcPatternDestroy( font );
    FcPatternDestroy( pat );
    return ok;
}
