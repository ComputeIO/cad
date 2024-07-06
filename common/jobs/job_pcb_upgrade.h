#ifndef JOB_PCB_UPGRADE_H
#define JOB_PCB_UPGRADE_H

#include <kicommon.h>
#include <wx/string.h>
#include "job.h"

class KICOMMON_API JOB_PCB_UPGRADE : public JOB
{
public:
    JOB_PCB_UPGRADE( bool aIsCli );
    wxString m_filename;
};

#endif