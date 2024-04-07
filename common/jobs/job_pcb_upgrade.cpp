#include <jobs/job_pcb_upgrade.h>


JOB_PCB_UPGRADE::JOB_PCB_UPGRADE( bool aIsCli ) :
    JOB( "upgrade", aIsCli ),
    m_filename()
{
}