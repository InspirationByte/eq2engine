//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor test drive
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORTESTDRIVE_H
#define EDITORTESTDRIVE_H

#include "car.h"
#include "utils/eqstring.h"


class CEditorTestGame
{
public:
	CEditorTestGame();
	~CEditorTestGame();

	void				Init();
	void				Destroy();

	//-------------------------------------------

	bool				IsGameRunning() const;

	bool				BeginGame( const char* carName, const Vector3D& startPos );
	void				EndGame();

	void				Update( float fDt );

	void				OnKeyPress(int keyCode, bool down);

protected:

	CCar*				CreateCar(const char* name);

	EqString					m_carName;
	vehicleConfig_t*			m_carEntry;

	CCar*						m_car;

	int							m_clientButtons;
};

extern CEditorTestGame* g_editorTestGame;

#endif // EDITORTESTDRIVE_H