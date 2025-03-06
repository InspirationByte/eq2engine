//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Pacifier
//////////////////////////////////////////////////////////////////////////////////

#include "core_common.h"
#include "cmd_pacifier.h"

static int g_LastPacifierDrawn = -1;


void StartPacifier( char const *pPrefix )
{
	MsgWarning( "%s", pPrefix );
	g_LastPacifierDrawn = -1;
	UpdatePacifier( 0.001f );
}

void UpdatePacifier( float flPercent )
{
	int iCur = (int)(flPercent * 40.0f);
	if( iCur != g_LastPacifierDrawn )
	{
		for( int i=g_LastPacifierDrawn+1; i <= iCur; i++ )
		{
			if ( !( i % 4 ) )
			{
				Msg("%d", i/4);
			}
			else
			{
				if( i != 40 )
				{
					Msg(".");
				}
			}
		}
		
		g_LastPacifierDrawn = iCur;
	}
}

void EndPacifier( bool bCarriageReturn )
{
	UpdatePacifier(1);
	
	if( bCarriageReturn )
		Msg("\n");
}
