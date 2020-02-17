//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Units
//////////////////////////////////////////////////////////////////////////////////

#ifndef DRIVERS_COORD_H
#define DRIVERS_COORD_H

namespace DrvSynUnits
{
	const float UnitsPerMeter				= 1.0f;
	const float MetersPerUnit				= 1.0f;

	// Maximum world size
	const float MaxCoordInUnits				= 32768.0f;
	const float MinCoordInUnits				= -MaxCoordInUnits;
	const float WorldSize					= MaxCoordInUnits*2.0;

	const float MaxCoordInMeters			= MaxCoordInUnits * MetersPerUnit;
};

#endif // DRIVERS_COORD_H