#ifndef COMMAND_PCB_RESAVE_H
#define COMMAND_PCB_RESAVE_H

#include "command.h"

namespace CLI
{
struct PCB_RESAVE_COMMAND : public COMMAND
{
    PCB_RESAVE_COMMAND();

protected:
    int doPerform( KIWAY& aKiway ) override;
};
}

#endif