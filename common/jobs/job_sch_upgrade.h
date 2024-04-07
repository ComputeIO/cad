#ifndef JOB_SCH_UPGRADE_H
#define JOB_SCH_UPGRADE_H

#include <kicommon.h>
#include <wx/string.h>
#include "job.h"

class KICOMMON_API JOB_SCH_UPGRADE : public JOB
{
public:
    JOB_SCH_UPGRADE( bool aIsCli );
    wxString m_filename;
};

#endif