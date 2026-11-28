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
class CPhysicsRagdoll;
class CEqStudioGeom;
class CEqRigidBody;
class IGPURenderPassRecorder;

enum ViewerRenderFlags
{
	RFLAG_PHYSICS		= (1 << 0),
	RFLAG_BONES			= (1 << 1),
	RFLAG_ATTACHMENTS	= (1 << 2),
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

	// the transform of the model: the physics body or the ragdoll while physics runs
	Matrix4x4					GetWorldMatrix() const;
	int							GetDrawnLod() const { return m_drawnLod; }

	ArrayCRef<AnimPoseController>	GetPoseControllers() const { return m_poseControllers; }
	ArrayCRef<AnimDataProvider>		GetAnimData() const { return m_animData; }
	ArrayCRef<AnimIkChain>			GetIkChains() const { return m_ikChains; }

	// the transforms (attachments) are edited on a copy of the EGF ones, as a preview;
	// a model space transform is the local one times the bone matrix of the current pose
	const studioTransform_t&	GetTransform(int transformIdx) const { return m_transforms[transformIdx]; }
	Matrix4x4					GetTransformParentMatrix(int transformIdx) const;
	void						SetModelSpaceTransform(int transformIdx, const Matrix4x4& transform);
	bool						IsTransformEdited(int transformIdx) const;
	void						ResetTransform(int transformIdx);

	virtual void				Update(float dt);

	// sets model for this entity
	void						SetModel(CEqStudioGeom* pModel);

	void						TogglePhysicsState();
	bool						IsPhysicsEnabled() const { return m_physicsEnable; }
	void						ResetPhysics();

	int							GetCurrentAnimationFrame() const;
	int							GetCurrentAnimationDurationInFrames() const;

protected:

	void						VisualizeBones(bool all);
	void						VisualizeAttachments(bool all);

	void						UpdateRagdollBones();
	void						DestroyPhysics();

	virtual void				HandleAnimatingEvent(AnimationEvent nEvent, const char* options);

	void						AttachIKChain(int chain, int attach_type);

public:

	CEqStudioGeom*				m_model;
	CEqRigidBody*				m_physObj;
	CPhysicsRagdoll*			m_ragdoll;
	bool						m_physicsEnable;

	Array<AnimSequence*>		m_sequencesList{ PP_SL };

	int							m_bodyGroupFlags;
	int							m_skinIdx{ 0 };
	int							m_selectedPhysObject{ -1 };	// drawn brighter than the others
	int							m_selectedBone{ -1 };		// highlighted, drawn also without RFLAG_BONES
	int							m_selectedAttachment{ -1 };	// highlighted, drawn also without RFLAG_ATTACHMENTS

protected:
	Array<studioTransform_t>	m_editedTransforms{ PP_SL };
	int							m_drawnLod{ 0 };
};