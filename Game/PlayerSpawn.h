//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Camera for Engine
//////////////////////////////////////////////////////////////////////////////////

#ifndef PLAYERSPAWN_H
#define PLAYERSPAWN_H

#include "BaseEngineHeader.h"

class CInfoPlayerStart : public BaseEntity
{
	DEFINE_CLASS_BASE(BaseEntity);
public:
				CInfoPlayerStart();

	void		Spawn();

	float		GetYawAngle();

	const char* GetPlayerScriptName();

	DECLARE_DATAMAP();
protected:

	EqString m_szPlayerScript;
};

#endif PLAYERSPAWN_H