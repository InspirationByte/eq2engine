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
		scale = 1.0f;
	}

	Vector3D			origin;
	float				velocity;
	float				angle;
	float				scale;
	float				weight;

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
	void					Draw(int nRenderFlags);
	bool					CheckVisibility(const occludingFrustum_t& frustum) const;

	static void				SheetsUpdateJob(void *data, int i);
	void					Simulate( float fDt );
	void					UpdateSheets( float fDt );

	int						ObjType() const		{return GO_DEBRIS;}

	bool					InitSheets();

protected:

	void					OnPhysicsCollide(const CollisionPairData_t& pair);

	DkList<sheetpart_t>		m_sheets;
	EqString				m_smashSound;

	CPhysicsHFObject*		m_hfObject;

	float					m_initDelay;
	bool					m_wasInit;
	bool					m_active;
};

#endif // OBJECT_SHEETS_H