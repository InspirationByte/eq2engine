//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate player data holder
//////////////////////////////////////////////////////////////////////////////////

#ifndef PLAYER_H
#define PLAYER_H

#include "session_base.h"

#include "car.h"
#include "utils/eqstring.h"

class CPlayer
{
public:
	CPlayer();
	~CPlayer();

	CCar*				GetCar();
	void				SetCar(CCar* car);

	bool				IsCarAlive();

	virtual void		SetControls(const playerControl_t& controls);

protected:

	CCar*				m_ownCar;
	playerControl_t		m_controls;
};

#endif // PLAYER