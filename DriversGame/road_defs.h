//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate road definitions
//////////////////////////////////////////////////////////////////////////////////

#ifndef ROAD_DEFS_H
#define ROAD_DEFS_H

#include "math/Vector.h"

#define ROADNEIGHBOUR_OFFS_X(x)		{x, x-1, x, x+1}		// non-diagonal
#define ROADNEIGHBOUR_OFFS_Y(y)		{y-1, y, y+1, y}

//
// Helper struct for road straight
//
struct straight_t
{
	straight_t()
	{
		breakIter = 0;
		dirChangeIter = 0;
		direction = -1;
		lane = -1;
		hasTrafficLight = false;
	}

	IVector2D	start;
	IVector2D	end;

	bool		hasTrafficLight;

	int			breakIter;
	int			dirChangeIter;
	int			direction;
	int			lane;
};

struct roadJunction_t
{
	roadJunction_t()
	{
		startIter = 0;
		breakIter = 0;
	}

	IVector2D	start;
	IVector2D	end;

	int			startIter;
	int			breakIter;
};

// helper functions
bool		IsOppositeDirectionTo(int dirA, int dirB);
bool		CompareDirection(int dirA, int dirB);

IVector2D	GetPerpendicularDirVec(const IVector2D& vec);
int			GetPerpendicularDir(int dir);

IVector2D	GetDirectionVecBy(const IVector2D& posA, const IVector2D& posB);
IVector2D	GetDirectionVec(int dirIdx);
int			GetDirectionIndex(const IVector2D& vec);
int			GetDirectionIndexByAngles(const Vector3D& angles);

bool		IsPointOnStraight(const IVector2D& pos, const straight_t& straight);
int			GetCellsBeforeStraight(const IVector2D& pos, const straight_t& straight);
bool		IsStraightsOnSameLane( const straight_t& a, const straight_t& b );

#endif // ROAD_DEFS_H