//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Animated model for EGFMan - supports physics
//////////////////////////////////////////////////////////////////////////////////


#include "studio_egf.h"
#include "Animating.h"

#include "ragdoll.h"

enum ViewerRenderFlags
{
	RFLAG_PHYSICS	= (1 << 0),
	RFLAG_BONES		= (1 << 1),
	RFLAG_WIREFRAME	= (1 << 2),
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

	virtual int					FindAttachment(const char* name);			// finds attachment
	virtual Vector3D			GetLocalAttachmentOrigin(int nAttach);		// gets local attachment position
	virtual Vector3D			GetLocalAttachmentDirection(int nAttach);	// gets local attachment direction

	// sets model for this entity
	void						SetModel(IEqModel* pModel);

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

	void						UpdateRagdollBones();

	virtual void				HandleAnimatingEvent(AnimationEvent nEvent, char* options);

	void						AttachIKChain(int chain, int attach_type);

public:

	IEqModel*					m_pModel;
	IPhysicsObject*				m_pPhysicsObject;
	ragdoll_t*					m_pRagdoll;
	bool						m_bPhysicsEnable;

	int							m_bodyGroupFlags;
};