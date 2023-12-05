#include <jobs/job_sch_resave.h>


JOB_SCH_RESAVE::JOB_SCH_RESAVE( bool aIsCli ) :
    JOB( "resave", aIsCli ),
    m_filename()
{
}