//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: main control for any streaming sound output device
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_LISTENER_H
#define SND_LISTENER_H

#include "math/Vector.h"
#include "dktypes.h"

#include "soundinterface.h"

#include "snd_dev.h"
#include "snd_channel.h"

class CSoundListener
{
public:

private:
	//  spatialization
	Vector3D			m_origin;
	Matrix3x3			m_orient;

	// mixing
	paintbuffer_t		m_paintBuffer;

	bool				m_active;
};

#endif // SND_LISTENER_H