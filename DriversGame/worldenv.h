//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Environment definitions
//////////////////////////////////////////////////////////////////////////////////

#ifndef WORLDENV_H
#define WORLDENV_H

#include "math/Vector.h"
#include "utils/eqstring.h"

enum EWorldLights
{
	WLIGHTS_NONE = 0,

	WLIGHTS_CARS = (1 << 0),
	WLIGHTS_LAMPS = (1 << 1),
	WLIGHTS_CITY = (1 << 2),
};

enum EWeatherType
{
	WEATHER_TYPE_CLEAR = 0,		// fair weather
	WEATHER_TYPE_RAIN,			// slight raining
	WEATHER_TYPE_STORM,

	WEATHER_COUNT,
};

class ITexture;

struct worldEnvConfig_t
{
	worldEnvConfig_t()
	{
		skyTexture = nullptr;
	}

	EqString		name;

	ColorRGB		sunColor;
	ColorRGB		ambientColor;
	ColorRGBA		shadowColor;

	float			lensIntensity;

	float			brightnessModFactor;
	float			streetLightIntensity;
	float			headLightIntensity;

	float			moonBrightness;

	float			rainBrightness;
	float			rainDensity;

	Vector3D		sunAngles;
	Vector3D		sunLensAngles;

	int				lightsType;
	EWeatherType	weatherType;

	bool			thunder;

	bool			fogEnable;
	float			fogNear;
	float			fogFar;

	Vector3D		fogColor;

	EqString		skyboxPath;
	ITexture*		skyTexture;
};

#endif // WORLDENV_H