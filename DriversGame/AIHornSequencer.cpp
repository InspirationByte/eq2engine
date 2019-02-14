//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: AI driver horn sequencer
//////////////////////////////////////////////////////////////////////////////////

#include "AIHornSequencer.h"
#include "math/Random.h"
#include "platform/Platform.h"

// here's some signal sequences i've recorded

signalVal_t g_sigSeq1[] = {
	{0, 0.133333},
	{0.183333, 0.266667},
	{0.35, 0.433333},
	{0.516667, 1.28333},
};

signalVal_t g_sigSeq2[] = {
	{0, 0.0833333},
	{0.166667, 0.283333},
	{0.466667, 1.36667},
};

signalVal_t g_sigSeq3[] = {
	{0, 0.216667},
	{0.283333, 1.05},
};

signalVal_t g_sigSeq4[] = {
	{0, 0.683333},
	{0.766667, 1.55},
	{1.65, 2.61666},
};

signalVal_t g_sigSeq5[] = {
	{0, 0.316667},
	{0.416667, 0.733333},
	{0.883333, 1},
	{1.1, 1.2},
	{1.3, 1.66667},
	{1.8, 1.9},
	{2, 2.1},
	{2.18333, 2.3},
	{2.4, 2.76666},
	{2.86666, 3},
	{3.1, 3.73333},
};

signalSeq_t g_signalSequences[] = {
	//{g_sigSeq5, elementsOf(g_sigSeq5), g_sigSeq5[elementsOf(g_sigSeq5)-1].end },
	{g_sigSeq1, elementsOf(g_sigSeq1), g_sigSeq1[elementsOf(g_sigSeq1) - 1].end },
	{g_sigSeq2, elementsOf(g_sigSeq2), g_sigSeq2[elementsOf(g_sigSeq2) - 1].end },
	{g_sigSeq3, elementsOf(g_sigSeq3), g_sigSeq3[elementsOf(g_sigSeq3) - 1].end },
	{g_sigSeq4, elementsOf(g_sigSeq4), g_sigSeq4[elementsOf(g_sigSeq4) - 1].end },
};

const int SIGNAL_SEQUENCE_COUNT = elementsOf(g_signalSequences);

CAIHornSequencer::CAIHornSequencer()
{
	m_signalSeq = nullptr;
	m_signalSeqFrame = 0;
}

void CAIHornSequencer::SignalRandomSequence(float delayBeforeStart)
{
	if (m_hornTime.IsOn())
		return;

	int seqId = RandomInt(0, SIGNAL_SEQUENCE_COUNT - 1);

	m_signalSeq = &g_signalSequences[seqId];
	m_signalSeqFrame = 0;

	m_hornTime.SetIfNot(m_signalSeq->length, delayBeforeStart);
}

void CAIHornSequencer::SignalNoSequence(float time, float delayBeforeStart)
{
	m_signalSeq = nullptr;

	m_hornTime.SetIfNot(time, delayBeforeStart);
}

void CAIHornSequencer::ShutUp()
{
	SignalNoSequence(0.0f, 0.0f);
}

bool CAIHornSequencer::Update(float fDt)
{
	bool state = false;

	// play signal sequence
	if (m_hornTime.IsOn())
	{
		if (m_signalSeq && m_signalSeqFrame < m_signalSeq->numVal)
		{
			float curSeqTime = m_signalSeq->length - m_hornTime.GetRemainingTime();

			state = (curSeqTime >= m_signalSeq->seq[m_signalSeqFrame].start &&
				curSeqTime <= m_signalSeq->seq[m_signalSeqFrame].end);

			if (curSeqTime > m_signalSeq->seq[m_signalSeqFrame].end)
				m_signalSeqFrame++;
		}
		else if (!m_signalSeq) // simple signal
			state = true;
	}

	m_hornTime.Update(fDt);

	return state;
}