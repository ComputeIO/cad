#ifndef COMMAND_SCH_UPGRADE_H
#define COMMAND_SCH_UPGRADE_H

#include "command.h"

namespace CLI
{
struct SCH_UPGRADE_COMMAND : public COMMAND
{
    SCH_UPGRADE_COMMAND();

protected:
    int doPerform( KIWAY& aKiway ) override;
};
}

#endif