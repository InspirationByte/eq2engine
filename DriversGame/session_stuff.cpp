//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Various game session stuff
//////////////////////////////////////////////////////////////////////////////////

#include "session_stuff.h"

static CReplayData		s_replayData;
CReplayData*			g_replayData = &s_replayData;

bool ParseVehicleConfig( vehicleConfig_t* conf, const kvkeybase_t* kvs );

DECLARE_CMD(car_loosehubcaps, "looses hubcaps on current car", 0)
{
	if(g_pGameSession && g_pGameSession->GetPlayerCar())
	{
		CCar* car = g_pGameSession->GetPlayerCar();

		for(int i = 0; i < car->GetWheelCount(); i++)
			car->ReleaseHubcap(i);
	}
}

DECLARE_CMD(car_reload, "reload current car", 0)
{
	if(g_pGameSession && g_pGameSession->GetPlayerCar())
	{
		EqString& fileName = g_pGameSession->GetPlayerCar()->m_conf->carScript;
		vehicleConfig_t* conf = g_pGameSession->GetPlayerCar()->m_conf;

		conf->scriptCRC = g_fileSystem->GetFileCRC32(fileName.c_str(), SP_MOD);

		Msg("Reloading car script '%s'...\n", fileName.c_str());

		if(conf->scriptCRC != 0)
		{
			kvkeybase_t kvs;
			if( !KV_LoadFromFile(conf->carScript.c_str(), SP_MOD,&kvs) )
			{
				MsgError("can't load default car script '%s'\n", conf->carScript.c_str());
				return;
			}

			conf->m_wheels.clear();
			conf->m_colors.clear();
			conf->m_gears.clear();

			if(!ParseVehicleConfig(conf, &kvs))
			{
				ASSERTMSG(false, varargs("Car configuration '%s' is invalid!\n", conf->carScript.c_str()));
				return;
			}
		}

		g_pGameSession->GetPlayerCar()->DebugReloadCar();
		g_pGameSession->GetPlayerCar()->SetColorScheme(RandomInt(0,conf->m_colors.numElem()-1));
	}
}

DECLARE_CMD(car_savereplay, "Saves current car replay", 0)
{
	if(CMD_ARGC == 0)
	{
		Msg("Usage: car_savereplay <filename>\n");
		return;
	}

	if(g_pGameSession && g_pGameSession->GetPlayerCar())
	{
		g_replayData->SaveVehicleReplay(g_pGameSession->GetPlayerCar(), CMD_ARGV(0).c_str());
	}
}

DECLARE_CMD(save, "Saves current replay", 0)
{
	if(CMD_ARGC == 0)
	{
		Msg("Usage: save <filename>\n");
		return;
	}

	if(g_pGameSession)
	{
		g_fileSystem->MakeDir("UserReplays", SP_MOD);
		g_replayData->SaveToFile( ("UserReplays/" + CMD_ARGV(0)).c_str() );
	}
}

DECLARE_CMD(car_spawn, "spawns new car", 0)
{
	if(CMD_ARGC > 0 && g_pGameSession->GetPlayerCar())
	{
		CCar* pCar = g_pGameSession->CreateCar(CMD_ARGV(0).c_str());

		if(pCar)
		{
			pCar->SetOrigin(g_pGameSession->GetPlayerCar()->GetOrigin() + g_pGameSession->GetPlayerCar()->GetRightVector()*2.0f);

			pCar->Spawn();

			g_pGameWorld->AddObject( pCar );

			pCar->SetControlButtons(IN_ACCELERATE | IN_TURNLEFT);
		}
	}
}

DECLARE_CMD(replay, "starts specified replay", 0)
{
	if(CMD_ARGC > 0)
	{
		g_State_Game->StartReplay( ("UserReplays/" + CMD_ARGV(0)).c_str() );
	}
	else
	{
		MsgWarning("usage: replay <replayName>");
	}
}