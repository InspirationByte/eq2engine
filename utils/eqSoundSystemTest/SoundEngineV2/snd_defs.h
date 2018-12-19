//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq sound engine types
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_DEFS_H
#define SND_DEFS_H

#include "dktypes.h"
#include "math/Vector.h"

//----------------------------------------------------------

// the sample pair used as output mix
typedef struct samplepair_s
{
	int     left;
	int     right;
} samplepair_t;

//----------------------------------------------------------

typedef struct stereo16_s
{
	short   left;
	short   right;
} stereo16_t;

typedef struct stereo8_s
{
	ubyte    left;
	ubyte    right;
} stereo8_t;

//----------------------------------------------------------

#define PAINTBUFFER_BYTES   4

typedef struct paintbuffer_s
{
	int		frequency;
	int		channels;
	int		volume;
	int		size;
	ubyte*	data;
} paintbuffer_t;

#endif // SND_DEFS_H