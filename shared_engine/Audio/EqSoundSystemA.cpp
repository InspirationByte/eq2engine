//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium engine sound system
//				OpenAL version
//////////////////////////////////////////////////////////////////////////////////

#include "EqSoundSystemA.h"

CEqSoundSystemA::CEqSoundSystemA()
{

}
CEqSoundSystemA::~CEqSoundSystemA()
{

}

void CEqSoundSystemA::Initialize()
{

}

void CEqSoundSystemA::Shutdown()
{

}

void CEqSoundSystemA::ReleaseEmitters()
{

}

void CEqSoundSystemA::ReleaseSamples()
{

}

bool CEqSoundSystemA::IsInitialized() const
{

}

//---------------------

// samples
IEqSoundSample* CEqSoundSystemA::LoadSample(const char* name, int nFlags)
{

}

void CEqSoundSystemA::FreeSample(IEqSoundSample* pSample)
{

}

// emitters
IEqSoundEmitter* CEqSoundSystemA::AllocSoundEmitter()
{

}

void CEqSoundSystemA::FreeSoundEmitter(IEqSoundEmitter* pEmitter)
{

}

// searches for effect
IEqSoundEffect* CEqSoundSystemA::FindEffect( const char* pszName ) const
{

}
	
//---------------------

// updates sound system
void CEqSoundSystemA::Update(float fDt, float fGlobalPitch)
{

}

// listener's
void CEqSoundSystemA::SetListener(const Vector3D& origin, 
								const Vector3D& zForward, 
								const Vector3D& yUp, 
								const Vector3D& velocity, 
								IEqSoundEffect* pEffect)
{

}

Vector3D CEqSoundSystemA::GetListenerPosition()
{

}

// sets volume (not affecting on ambient sources)
void CEqSoundSystemA::SetVolumeScale( float vol_scale )
{

}