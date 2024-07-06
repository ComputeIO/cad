#include "command_pcb_upgrade.h"
#include "jobs/job_pcb_upgrade.h"
#include "cli/exit_codes.h"
#include <wx/crt.h>


CLI::PCB_UPGRADE_COMMAND::PCB_UPGRADE_COMMAND() : COMMAND( "upgrade" )
{
    addCommonArgs( true, false, false, false);
    m_argParser.add_description( UTF8STDSTR( _( "Upgrade a PCB file's format to the latest one" ) ) );
}

int CLI::PCB_UPGRADE_COMMAND::doPerform(KIWAY& aKiway)
{
    std::unique_ptr<JOB_PCB_UPGRADE> upgradeJob = std::make_unique<JOB_PCB_UPGRADE>( true );

    upgradeJob->m_filename = m_argInput;

    if( !wxFile::Exists( upgradeJob->m_filename ) )
    {
        wxFprintf( stderr, _( "Board file does not exist or is not accessible\n" ) );
        return EXIT_CODES::ERR_INVALID_INPUT_FILE;
    }

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, upgradeJob.get() );

    return exitCode;
}