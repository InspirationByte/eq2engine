//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: flying sheets, leaves and newspapers
//////////////////////////////////////////////////////////////////////////////////

#ifndef OBJECT_SHEETS_H
#define OBJECT_SHEETS_H

#include "GameObject.h"
#include "state_game.h"

class CPFXAtlasGroup;
struct TexAtlasEntry_t;

struct sheetpart_t
{
	sheetpart_t()
	{
		velocity = 0.0f;
		angle = 0.0f;
	}

	Vector3D			origin;
	float				velocity;
	float				angle;

	CPFXAtlasGroup*		atlas;
	TexAtlasEntry_t*	entry;
};

class CObject_Sheets : public CGameObject
{
public:
	DECLARE_CLASS( CObject_Sheets, CGameObject )

	CObject_Sheets( kvkeybase_t* kvdata );
	~CObject_Sheets();

	void					OnRemove();
	void					Spawn();

	void					Simulate( float fDt );

	int						ObjType() const		{return GO_DEBRIS;}

	bool					InitSheets();

protected:

	EqString				m_smashSound;

	DkList<sheetpart_t>		m_sheets;

	CEqCollisionObject*		m_ghostObject;

	bool					m_wasInit;

	float					m_initDelay;
};

#endif // OBJECT_SHEETS_H