#ifndef COMMAND_PCB_UPGRADE_H
#define COMMAND_PCB_UPGRADE_H

#include "command.h"

namespace CLI
{
struct PCB_UPGRADE_COMMAND : public COMMAND
{
    PCB_UPGRADE_COMMAND();

protected:
    int doPerform( KIWAY& aKiway ) override;
};
}

#endif