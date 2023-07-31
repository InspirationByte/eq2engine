//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium coordinate system
//////////////////////////////////////////////////////////////////////////////////

#pragma once

/*
UNITS:
	64 units = 1 meter
	104 units = deafult human height
*/

#define METERS_PER_UNIT					(0.015625f)
#define CUBIC_METERS_PER_CUBIC_UNIT		(METERS_PER_UNIT*METERS_PER_UNIT*METERS_PER_UNIT)
#define METERS_PER_UNIT_INV				(1.0f/0.015625f)

// Maximum world size
#define MAX_COORD_UNITS					(196608.0f) // 6.3 Kilometers
#define MIN_COORD_UNITS					(-MAX_COORD_UNITS)
#define WORLD_SIZE						(MAX_COORD_UNITS*2.0f) // 13,1 km

#define MAX_COORD_METERS				(MAX_COORD_UNITS * METERS_PER_UNIT)