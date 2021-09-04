#ifndef IBIS2KIBIS_H
#define IBIS2KIBIS_H

#include "kibis.h"
#include "ibisParser.h"

bool convertKibisFile( IbisParser* aParser, KIBIS_FILE* aFile );
bool convertKibisModel( IbisParser* aParser, IbisModel* aSource, KIBIS_MODEL* aDest );
bool convertKibisComponent( IbisParser* aParser, std::vector<KIBIS_MODEL*> aModels,
                            IbisComponent* aSource, KIBIS_COMPONENT* aDest );
bool convertKibisAll( IbisParser* aParser, std::vector<KIBIS_COMPONENT*>* aDest );
#endif