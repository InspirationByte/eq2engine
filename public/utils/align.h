//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: value structure alignment
//////////////////////////////////////////////////////////////////////////////////

#ifndef ALIGN_H
#define ALIGN_H

#include "dktypes.h"

#define ALIGN4( a ) a	= (ubyte *)((int)((ubyte *)a + 3) & ~ 3)
#define ALIGN16( a ) a	= (ubyte *)((int)((ubyte *)a + 15) & ~ 15)
#define ALIGN32( a ) a	= (ubyte *)((int)((ubyte *)a + 31) & ~ 31)
#define ALIGN64( a ) a	= (ubyte *)((int)((ubyte *)a + 63) & ~ 63)

#endif // ALIGN_H