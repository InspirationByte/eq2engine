//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Animated model for EGFMan - supports physics
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "animating/Animating.h"

struct gsequence_t;
struct gposecontroller_t;
struct ragdoll_t;
class CEqStudioGeom;
class IPhysicsObject;

enum ViewerRenderFlags
{
	RFLAG_PHYSICS		= (1 << 0),
	RFLAG_BONES			= (1 << 1),
	RFLAG_ATTACHMENTS	= (1 << 2),
	RFLAG_WIREFRAME		= (1 << 2),
};

// basic animating class
// for ragdoll use baseragdollanimating
class CAnimatedModel : public CAnimatingEGF
{
public:
								CAnimatedModel();

	// renders model
	virtual void				Render(int nViewRenderFlags, float fDist, int startLod, bool overrideLod, float dt);
	virtual void				RenderPhysModel();
	virtual void				Update(float dt);

	// sets model for this entity
	void						SetModel(CEqStudioGeom* pModel);

	void						TogglePhysicsState();
	void						ResetPhysics();

	int							GetCurrentAnimationFrame() const;
	int							GetCurrentAnimationDurationInFrames() const;

	int							GetNumSequences() const;
	int							GetNumPoseControllers() const;

	const gsequence_t&			GetSequence(int seq) const;
	const gposecontroller_t&	GetPoseController(int seq) const;

protected:

	void						VisualizeBones();
	void						VisualizeAttachments();

	void						UpdateRagdollBones();

	virtual void				HandleAnimatingEvent(AnimationEvent nEvent, const char* options);

	void						AttachIKChain(int chain, int attach_type);

public:

	CEqStudioGeom*					m_pModel;
	IPhysicsObject*				m_physObj;
	ragdoll_t*					m_pRagdoll;
	bool						m_bPhysicsEnable;

	int							m_bodyGroupFlags;
};