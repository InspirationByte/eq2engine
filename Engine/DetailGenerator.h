//***********Copyright (C) Illusion Way Interactive Software 2008-2010************
//
// Description: Grass batch generator
//
//********************************************************************************

#ifndef DETAILGENERATOR_H
#define DETAILGENERATOR_H

#include "EngineBSPLoader.h"

void	GenerateDetails(const char* mapName, enginebspnode_t *nodes, bsphwleaf_t* leaves);
void	DrawDetails();
void	DestroyDetails();

#endif // DETAILGENERATOR_H