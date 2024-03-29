//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Engine version
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConCommand.h"
#include "sys_version.h"

// date i've stated development		"Feb 28 2009"
// Drivers started					"Apr 07 2014"

const char* date = COMPILE_DATE;

const char* months[12] =
{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

char months_days[12] =
{ 31,    28,    31,    30,    31,    30,    31,    31,    30,    31,    30,    31 };

class CEngineBuildNumber
{
public:
			CEngineBuildNumber()	{ComputeBuild();}

	int		GetBuildNumber()		{return m_nBuildNumber;}

protected:

	void ComputeBuild()
	{
		int m = 0;
		int d = 0;
		int y = 0;

		for (m = 0; m < 12; m++)
		{
			// stop on the current month

			if (strncmp( &date[0], months[m], 3 ) == 0)
				break;

			d += months_days[m];
		}

		d += atoi( &date[4] ) - 1;

		y = atoi( &date[7] ) - 1900;

		m_nBuildNumber = d + (int)((y - 1) * 365.25);

		// add 29th february
		if (((y % 4) == 0) && m > 1)
			m_nBuildNumber += 1;

		m_nBuildNumber -= 41734;	// "Apr 07 2014"	- Drivers start
		//m_nBuildNumber -= 39505;	// "Feb 28 2009"	- Engine start
	}

	int		m_nBuildNumber;
};

static CEngineBuildNumber g_build;

int GetEngineBuildNumber()
{
	return g_build.GetBuildNumber();
}

void EngineMessage()
{
	MsgInfo(" \n\"%s\" build %i\nCompilation date: %s %s\n", ENGINE_NAME, BUILD_NUMBER_ENGINE, COMPILE_DATE, COMPILE_TIME);
}
