#include <jobs/job_pcb_resave.h>


JOB_PCB_RESAVE::JOB_PCB_RESAVE( bool aIsCli ) :
    JOB( "resave", aIsCli ),
    m_filename(),
    //m_libraryPath(),
    //m_outputLibraryPath(),
    m_force( false )
{
}