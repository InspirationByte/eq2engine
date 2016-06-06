//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine Sound system
//
//				Sound emitter
//////////////////////////////////////////////////////////////////////////////////

#ifndef ALSND_EMITTER_H
#define ALSND_EMITTER_H

class DkSoundEmitterLocal : public ISoundEmitter
{
public:
	friend class		DkSoundSystemLocal;

	DkSoundEmitterLocal();
	~DkSoundEmitterLocal();

	void				SetSample(ISoundSample* sample);
	ISoundSample*		GetSample() const;

	void				SetVolume(float val);
	void				SetPitch(float val);

	void				SetPosition(const Vector3D &position);
	void				SetVelocity(const Vector3D &velocity);

	void				Play();
	void				Stop();

	void				StopLoop();

	void				Pause();

	ESoundState			GetState() const;

	bool				IsStopped() const;
	bool				IsVirtual() const;

	void				Update();

	void				GetParams(soundParams_t *param) const;
	void				SetParams(soundParams_t *param);

protected:

	bool				m_virtual;
	ALenum				m_virtualState;

	int					m_nChannel;

	Vector3D			vPosition;
	//Vector3D			vVelocity;

	DkSoundSampleLocal*	m_sample;

	soundParams_t		m_params;
};

#endif // ALSND_EMITTER_H