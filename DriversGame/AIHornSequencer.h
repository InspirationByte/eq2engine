//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: AI driver horn sequencer
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIHORNSEQUENCER_H
#define AIHORNSEQUENCER_H

struct signalVal_t
{
	float start;
	float end;
};

struct signalSeq_t
{
	signalVal_t*	seq;
	int				numVal;
	float			length;
};

//
// Little helper
//
class CTimedRelay
{
public:
	CTimedRelay() : m_time(0), m_delay(0) {}

	void Set(float time, float delay = 0.0f)
	{
		m_time = time;
		m_delay = delay;
	}

	void SetIfNot(float time, float delay = 0.0f)
	{
		if (GetTotalTime() > 0)
			return;

		m_time = time;
		m_delay = delay;
	}

	bool IsOn()
	{
		return (m_delay <= 0.0f && m_time > 0.0f);
	}

	float GetTotalTime()
	{
		return m_delay + m_time;
	}

	float GetRemainingTime()
	{
		return m_time;
	}

	void Update(float fDt)
	{
		if (m_delay <= 0.0f)
		{
			m_delay = 0.0f;
			if (m_time > 0.0f)
				m_time -= fDt;
		}
		else
			m_delay -= fDt;
	}

protected:

	float m_delay;
	float m_time;
};

// the sequencer itself
class CAIHornSequencer
{
public:
	CAIHornSequencer();

	bool	Update(float fDt);

	void	ShutUp();

	void	SignalRandomSequence(float delayBeforeStart);
	void	SignalNoSequence(float time, float delayBeforeStart);

private:
	CTimedRelay			m_hornTime;
	signalSeq_t*		m_signalSeq;
	int					m_signalSeqFrame;
};

#endif // AIHORNSEQUENCER_H