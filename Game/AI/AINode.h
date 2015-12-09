//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: AI node and hint entities
//////////////////////////////////////////////////////////////////////////////////

#ifndef AINODE_H
#define AINODE_H

#include "BaseEntity.h"

enum HintType_e
{
	HINT_INVALID = -1,
	HINT_JUMP = 0,
	HINT_COVER,
	HINT_COVER_CROUCH,
};

inline bool IsCover(HintType_e type)
{
	return (type >= HINT_COVER) && (type <= HINT_COVER_CROUCH);
}

//-------------------------------------------------------
// Hint node entity
//-------------------------------------------------------
class CAIHint : public BaseEntity
{
	DEFINE_CLASS_BASE( BaseEntity );
public:

	CAIHint();
	~CAIHint();

	EntType_e			EntType();
	HintType_e			GetHintType();

	void				SetIsUsed(bool bInUse);
	bool				IsUsed();

protected:
	DECLARE_DATAMAP();

	HintType_e			m_nHintType;
	bool				m_bInUse;
};

//-------------------------------------------------------
// AI ground node entity
//-------------------------------------------------------
class CAINodeGround : public BaseEntity
{
	DEFINE_CLASS_BASE( BaseEntity );

	friend class CAINavigator;
public:

	CAINodeGround();
	~CAINodeGround();

	void				OnMapSpawn();

	void				Spawn();

	EntType_e			EntType();

	// TODO: check visibility from one tho another entity

protected:
	DECLARE_DATAMAP();

	DkList<CAINodeGround*> m_pLinkedNodes;
};


#endif // AINODE_H