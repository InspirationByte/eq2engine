//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics constraint
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQPHYSICS_CONSTRAINT_H
#define EQPHYSICS_CONSTRAINT_H

class IEqPhysicsConstraint
{
	friend class CEqRigidBody;

public:
					IEqPhysicsConstraint() : m_enabled(false), m_satisfied(true) {}
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
};

#endif // EQPHYSICS_CONSTRAINT_H