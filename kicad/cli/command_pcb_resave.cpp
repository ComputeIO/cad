#include "command_pcb_resave.h"
#include "jobs/job_pcb_resave.h"
#include "cli/exit_codes.h"


CLI::PCB_RESAVE_COMMAND::PCB_RESAVE_COMMAND() : COMMAND( "resave" )
{
    addCommonArgs( true, false, false, false);
    m_argParser.add_description( UTF8STDSTR( _( "Resave a PCB file's format to the latest one" ) ) );
}

int CLI::PCB_RESAVE_COMMAND::doPerform(KIWAY& aKiway)
{
    std::unique_ptr<JOB_PCB_RESAVE> resaveJob = std::make_unique<JOB_PCB_RESAVE>( true );

    resaveJob->m_filename = m_argInput;

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_PCB, resaveJob.get() );

    return exitCode;
}