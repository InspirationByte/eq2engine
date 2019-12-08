//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Film director
//////////////////////////////////////////////////////////////////////////////////

#ifndef DIRECTOR_H
#define DIRECTOR_H

#include "math/Vector.h"

#define			DIRECTOR_DEFAULT_CAMERA_FOV	 (60.0f) // old: 52

class CCar;

struct freeCameraProps_t
{
	freeCameraProps_t();

	Vector3D	position;
	Vector3D	angles;
	Vector3D	velocity;
	float		fov;

	int			buttons;

	bool		zAxisMove;
};

extern freeCameraProps_t g_freeCamProps;

CCar*	Director_GetCarOnCrosshair(bool queryImportantOnly = true);

void	Director_Draw( float fDt );
void	Director_KeyPress(int key, bool down);

bool	Director_FreeCameraActive();
void	Director_UpdateFreeCamera( float fDt );
void	Director_MouseMove( int x, int y, float deltaX, float deltaY  );
void	Director_Reset();

void	Director_Enable( bool enable );
bool	Director_IsActive();

/*
class CReplayDirector
{
public:
	CReplayDirector();
};*/

#endif // DIRECTOR_H