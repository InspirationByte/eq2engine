//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: AI pedestrian manager for Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIPEDESTRIANMANAGER_H
#define AIPEDESTRIANMANAGER_H

#include "math/Vector.h"

class CAIPedestrianManager
{
	friend class				CGameSessionBase;

public:
	CAIPedestrianManager();
	~CAIPedestrianManager();

	void						Init();
	void						Shutdown();

	void						UpdateRespawn(float fDt, const Vector3D& spawnCenter);

	void						RemoveAllPedestrians();
};

#endif // AIPEDESTRIANMANAGER_H