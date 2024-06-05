//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Animated model for EGFMan - supports physics
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "animating/Animating.h"

struct AnimSequence;
struct AnimPoseController;
struct PhysRagdollData;
class CEqStudioGeom;
class IPhysicsObject;
class IGPURenderPassRecorder;

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
	virtual void				Render(int nViewRenderFlags, float fDist, int startLod, bool overrideLod, float dt, IGPURenderPassRecorder* rendPassRecorder);
	virtual void				RenderPhysModel(IGPURenderPassRecorder* rendPassRecorder);
	virtual void				Update(float dt);

	// sets model for this entity
	void						SetModel(CEqStudioGeom* pModel);

	void						TogglePhysicsState();
	void						ResetPhysics();

	int							GetCurrentAnimationFrame() const;
	int							GetCurrentAnimationDurationInFrames() const;

	ArrayCRef<AnimPoseController>	GetPoseControllers() const;
	ArrayCRef<AnimSequence*>		GetSequencesList() const;

protected:

	void						VisualizeBones();
	void						VisualizeAttachments();

	void						UpdateRagdollBones();

	virtual void				HandleAnimatingEvent(AnimationEvent nEvent, const char* options);

	void						AttachIKChain(int chain, int attach_type);

public:

	CEqStudioGeom*				m_pModel;
	IPhysicsObject*				m_physObj;
	PhysRagdollData*			m_pRagdoll;
	bool						m_bPhysicsEnable;

	Array<AnimSequence*>		m_sequencesList{ PP_SL };

	int							m_bodyGroupFlags;
};