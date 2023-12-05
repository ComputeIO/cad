#ifndef JOB_SCH_RESAVE_H
#define JOB_SCH_RESAVE_H

#include <kicommon.h>
#include <wx/string.h>
#include "job.h"

class KICOMMON_API JOB_SCH_RESAVE : public JOB
{
public:
    JOB_SCH_RESAVE( bool aIsCli );
    wxString m_filename;
};

#endif