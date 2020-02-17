//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Game basic weapon class
//////////////////////////////////////////////////////////////////////////////////

#ifndef BASEANIMATING_H
#define BASEANIMATING_H

#include "BaseEngineHeader.h"
#include "GameRenderer.h"

#include "Animating.h"

enum EIKAttachType
{
	IK_ATTACH_WORLD = 0,
	IK_ATTACH_LOCAL,	//= 1,
	IK_ATTACH_GROUND,	//= 2,
};


// basic animating class
// for ragdoll use baseragdollanimating
class BaseAnimating : public BaseEntity, public CAnimatingEGF
{
	DEFINE_CLASS_BASE(BaseEntity);
public:

								BaseAnimating();

// standard stuff

	virtual	void				InitScriptHooks();

	// applyes damage to entity (use this)
	virtual void				MakeDecal( const decalmakeinfo_t& info );

	virtual int					FindAttachment(const char* name) const;			// finds attachment
	virtual const Vector3D&		GetLocalAttachmentOrigin(int nAttach) const;		// gets local attachment position
	virtual const Vector3D&		GetLocalAttachmentDirection(int nAttach) const;	// gets local attachment direction

	// sets model for this entity
	void						SetModel(IEqModel* pModel);

	// sets model for this entity
	void						SetModel(const char* pszModelName);

	void						AttachIKChain(int chain, EIKAttachType type);

	void						AddMotions(studioHwData_t::motionData_t* motionData);

protected:
	virtual void				HandleAnimatingEvent(AnimationEvent nEvent, char* options);

	virtual void				Update(float dt);

	void						UpdateBones();

	// renders model
	virtual void				Render(int nViewRenderFlags);

	virtual void				OnRemove();

	DECLARE_DATAMAP();
};

#endif // BASEANIMATING_H