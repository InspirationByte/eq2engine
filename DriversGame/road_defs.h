//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
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
		id = 0xFFFF;
	}

	IVector2D	start;
	IVector2D	end;

	short			breakIter;
	short			dirChangeIter;
	short		direction;
	short		lane;

	ushort		id;

	bool		hasTrafficLight;
};

struct roadJunction_t
{
	roadJunction_t()
	{
		startIter = 0;
		breakIter = 0;
		id = 0xFFFF;
	}

	IVector2D	start;
	IVector2D	end;

	short		startIter;
	short		breakIter;

	ushort		id;
};

struct pathFindResult_t
{
	IVector2D			start;
	IVector2D			end;

	DkList<IVector2D>	points;	// z is distance in cells

	short				gridSelector;
};

class CEqCollisionObject;

struct pathFindResult3D_t
{
	Vector3D			start;
	Vector3D			end;

	DkList<Vector4D>	points;	// w is narrowness factor

	void InitFrom(pathFindResult_t& path, CEqCollisionObject* ignore);
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
int			GetCellsBeforeStraightStart(const IVector2D& pos, const straight_t& straight);
int			GetCellsBeforeStraightEnd(const IVector2D& pos, const straight_t& straight);
bool		IsStraightsOnSameLane( const straight_t& a, const straight_t& b );

#endif // ROAD_DEFS_H
