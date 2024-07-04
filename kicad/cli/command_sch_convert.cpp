/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "command_sch_convert.h"
#include "command_utils.h"

#include "jobs/job_sch_convert.h"


#include <cli/exit_codes.h>
#include "command_utils.h"
#include <kiface_base.h>
#include <layer_ids.h>
#include <string_utils.h>
#include <wx/crt.h>
#include <magic_enum.hpp>

#include <macros.h>
#include <wx/tokenzr.h>

#define ARG_FROM "--from"
#define ARG_TO "--to"


CLI::SCH_CONVERT_COMMAND::SCH_CONVERT_COMMAND() : COMMAND( "convert" )
{
    addCommonArgs( true, true, false, false );
    addDefineArg();

    m_argParser.add_description( UTF8STDSTR(
            _( "Convert a vendor's schematic document to another vendor's schematic document" ) ) );


    m_argParser.add_argument( ARG_FROM )
            .default_value( std::string( "sch_altium" ) )
            .add_choices( enumChoices<JOB_SCH_CONVERT::From>() )
            .metavar( "FROM" )
            .help( UTF8STDSTR( wxString::Format( _( "Accepted PCB format. Options: %s" ),
                                                 enumString<JOB_SCH_CONVERT::From>() ) ) );


    m_argParser.add_argument( ARG_TO )
            .default_value( std::string( "sch_kicad" ) )
            .add_choices( enumChoices<JOB_SCH_CONVERT::To>() )
            .metavar( "TO" )
            .help( UTF8STDSTR( wxString::Format( _( "Converted format. Options: %s" ),
                                                 enumString<JOB_SCH_CONVERT::To>() ) ) );
}


int CLI::SCH_CONVERT_COMMAND::doPerform( KIWAY& aKiway )
{
    std::unique_ptr<JOB_SCH_CONVERT> convertJob( new JOB_SCH_CONVERT( true ) );

    convertJob->m_outputFile = m_argOutput;
    convertJob->m_filename = m_argInput;
    convertJob->SetVarOverrides( m_argDefineVars );
    getToEnum( m_argParser.get<std::string>( ARG_FROM ), convertJob->m_from );
    getToEnum( m_argParser.get<std::string>( ARG_TO ), convertJob->m_to );
    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, convertJob.get() );
    return exitCode;
}
