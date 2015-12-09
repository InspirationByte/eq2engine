//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Units
//////////////////////////////////////////////////////////////////////////////////

#if !defined(DRIVERS_COORD_H) && !defined(METERS_PER_UNIT)
#define DRIVERS_COORD_H

#define METERS_PER_UNIT					(0.5f)
#define CUBIC_METERS_PER_CUBIC_UNIT		(METERS_PER_UNIT*METERS_PER_UNIT*METERS_PER_UNIT)
#define METERS_PER_UNIT_INV				(1.0f/05)

// Maximum world size
#define MAX_COORD_UNITS 32768
#define MIN_COORD_UNITS (-MAX_COORD_UNITS)
#define WORLD_SIZE (MAX_COORD_UNITS*2) // 13,1 km

#define MAX_COORD_METERS (MAX_COORD_UNITS * METERS_PER_UNIT)


#endif // DRIVERS_COORD_H