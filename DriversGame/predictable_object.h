//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: Network predictable object
//////////////////////////////////////////////////////////////////////////////////

#ifndef PREDICTABLE_OBJECT_H
#define PREDICTABLE_OBJECT_H

#include "Platform.h"
#include "GameObject.h"

//
// Player input command
//
struct netInputCmd_s
{
	int controls;
	int	timeStamp;
};
ALIGNED_TYPE(netInputCmd_s,4) netInputCmd_t;

//
// network object snapshot
//
struct netSnapshot_s
{
	TVec4D<half>	car_rot;
	FVector3D		car_pos;
	Vector3D		car_vel;

	netInputCmd_t	in;
};

ALIGNED_TYPE(netSnapshot_s,4) netSnapshot_t;

//------------------------------------------------------------------------------------

//
// Player car snapshot
//
struct netObjSnapshot_t
{
	netObjSnapshot_t() {}

	netObjSnapshot_t(const netSnapshot_t& snap)
	{
		rotation = Quaternion(snap.car_rot.w,snap.car_rot.x,snap.car_rot.y,snap.car_rot.z);
		position = snap.car_pos;
		velocity = snap.car_vel;

		in = snap.in;
	}

	Quaternion		rotation;
	FVector3D		position;
	Vector3D		velocity;
	netInputCmd_t	in;

	float			latency;
};


#endif // PREDICTABLE_OBJECT_H