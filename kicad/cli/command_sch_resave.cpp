#include "command_sch_resave.h"
#include "jobs/job_sch_resave.h"
#include "cli/exit_codes.h"


CLI::SCH_RESAVE_COMMAND::SCH_RESAVE_COMMAND() : COMMAND( "resave" )
{
    addCommonArgs( true, false, false, false);
    m_argParser.add_description( UTF8STDSTR( _( "Resave a SCH file's format into the current one" ) ) );
}

int CLI::SCH_RESAVE_COMMAND::doPerform(KIWAY& aKiway)
{
    std::unique_ptr<JOB_SCH_RESAVE> resaveJob = std::make_unique<JOB_SCH_RESAVE>( true );

    resaveJob->m_filename = m_argInput;

    int exitCode = aKiway.ProcessJob( KIWAY::FACE_SCH, resaveJob.get() );

    return exitCode;
}