//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Audio/Video stream source
//////////////////////////////////////////////////////////////////////////////////

#ifndef AVSTREAMSOURCE_H
#define AVSTREAMSOURCE_H

#include "materialsystem/renderers/ITexture.h"
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>

class ISoundStreamPlayable
{

};

class CAVStreamSource
{
public:
	bool					Init(const char* streamFileName, const char* textureName);
	void					Destroy();

	void					Update(float delta);

	void					Stop();
	void					Pause();
	void					Play();

	int						GetLength() const;
	void					Seek() const;

	// the updateable texture
	ITexture*				GetImageSource() const;

	// the updateable sound source
	ISoundStreamPlayable*	GetSoundSource() const;

protected:
};

#endif // AVSTREAMSOURCE_H