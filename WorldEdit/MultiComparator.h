//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Multi-type, universal value comparator
//////////////////////////////////////////////////////////////////////////////////

#ifndef MULTICOMPARATOR_H
#define MULTICOMPARATOR_H

template< class T >
class CMultiTypeComparator
{
public:
	CMultiTypeComparator()
	{
		m_bDiffers = false;
		m_bIsInit = false;
	}

	void CompareValue(T val)
	{
		if(m_bDiffers)
			return;

		if(!m_bIsInit)
		{
			m_bIsInit = true;
			m_StandardObject = val;
		}
		else
		{
			if(m_StandardObject != val)
				m_bDiffers = true;
		}
	}

	bool IsDiffers()
	{
		if(!m_bIsInit)
			return true;

		return m_bDiffers;
	}

	bool IsInit()
	{
		return m_bIsInit;
	}

	void Reset()
	{
		m_bDiffers = false;
		m_bIsInit = false;
	}

	T	GetValue()
	{
		return m_StandardObject;
	}

private:
	bool	m_bDiffers;
	bool	m_bIsInit;

	T		m_StandardObject;
};

#endif // MULTICOMPARATOR_H