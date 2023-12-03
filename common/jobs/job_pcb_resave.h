#ifndef JOB_PCB_RESAVE_H
#define JOB_PCB_RESAVE_H

#include <kicommon.h>
#include <wx/string.h>
#include "job.h"

class KICOMMON_API JOB_PCB_RESAVE : public JOB
{
public:
    JOB_PCB_RESAVE( bool aIsCli );

    wxString m_filename;
    //wxString m_libraryPath;
    //wxString m_outputLibraryPath;

    bool m_force;
};

#endif