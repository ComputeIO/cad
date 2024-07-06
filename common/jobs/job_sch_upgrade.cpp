#include <jobs/job_sch_upgrade.h>


JOB_SCH_UPGRADE::JOB_SCH_UPGRADE( bool aIsCli ) :
    JOB( "upgrade", aIsCli ),
    m_filename()
{
}