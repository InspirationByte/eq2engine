//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics constraint
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQPHYSICS_CONSTRAINT_H
#define EQPHYSICS_CONSTRAINT_H

enum EConstraintFlags
{
	CONSTRAINT_FLAG_BODYA_NOIMPULSE		= (1 << 0),
	CONSTRAINT_FLAG_BODYB_NOIMPULSE		= (1 << 1),
	CONSTRAINT_FLAG_SKIP_INTERCOLLISION = (1 << 2),
};

class IEqPhysicsConstraint
{
	friend class CEqRigidBody;

public:
					IEqPhysicsConstraint() : m_enabled(false), m_satisfied(true), m_flags(0) {}
	virtual			~IEqPhysicsConstraint() {}

	void			SetEnabled(bool enable) { m_enabled = enable; }
	bool			IsEnabled()	const		{ return m_enabled; }

    // prepare for applying constraints - the subsequent calls to
    // apply will all occur with a constant position i.e. precalculate
    // everything possible
    virtual void	PreApply(float dt) { m_satisfied = false; }
    
    // apply the constraint by adding impulses. Return value
    // indicates if any impulses were applied. If impulses were applied
    // the derived class should set m_satisfied to false on each
    // body that is involved.
    virtual bool	Apply(float dt) = 0;
    
    // implementation should remove all references to bodies etc - they've 
    // been destroyed.
    virtual void	Destroy() = 0;

protected:
	bool			m_enabled;
	bool			m_satisfied;
	int				m_flags;
};

#endif // EQPHYSICS_CONSTRAINT_H