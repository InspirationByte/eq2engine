//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: EGFMan2 - eqPhysics world
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqPhysicsWorld;

// registers studio shape cache, must be called before loading models
void				PhysicsLibInit();
void				PhysicsLibShutdown();

// creates world with the floor
void				PhysicsSceneInit();
void				PhysicsSceneShutdown();

// runs fixed timestep simulation
void				PhysicsSimulate(float frameTime);

CEqPhysicsWorld&	PhysicsGetWorld();
