//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base AI driver, affectors, manipulators
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMANIPULATORS_H
#define AIMANIPULATORS_H

class CAIBaseDriver;

//
class CAIManipulatorBase
{
public:
	CAIManipulatorBase() {}
	virtual ~CAIManipulatorBase() {}

	virtual void UpdateAffector(struct ai_handling_t& handling, CAIBaseDriver* driver, float deltaTime) = 0;
};

#endif // AIMANIPULATORS_H