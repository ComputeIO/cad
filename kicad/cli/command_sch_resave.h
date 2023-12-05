#ifndef COMMAND_SCH_RESAVE_H
#define COMMAND_SCH_RESAVE_H

#include "command.h"

namespace CLI
{
struct SCH_RESAVE_COMMAND : public COMMAND
{
    SCH_RESAVE_COMMAND();

protected:
    int doPerform( KIWAY& aKiway ) override;
};
}

#endif