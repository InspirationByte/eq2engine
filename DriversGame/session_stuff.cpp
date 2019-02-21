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

DECLARE_CMD(car_loosehubcaps, "looses hubcaps on current car", CV_CHEAT)
{
	if(g_pGameSession && g_pGameSession->GetPlayerCar())
	{
		CCar* car = g_pGameSession->GetPlayerCar();

		for(int i = 0; i < car->GetWheelCount(); i++)
			car->ReleaseHubcap(i);
	}
}

DECLARE_CMD(car_reload, "reload current car", CV_CHEAT)
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
				MsgError("can't load car script '%s'\n", conf->carScript.c_str());
				return;
			}

			conf->DestroyCleanup();

			if(!ParseVehicleConfig(conf, &kvs))
			{
				ASSERTMSG(false, varargs("Car configuration '%s' is invalid!\n", conf->carScript.c_str()));
				return;
			}
		}

		g_pGameSession->GetPlayerCar()->DebugReloadCar();
		g_pGameSession->GetPlayerCar()->SetColorScheme(RandomInt(0,conf->numColors-1));
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
		g_fileSystem->MakeDir(USERREPLAYS_PATH, SP_MOD);
		g_replayData->SaveToFile( (USERREPLAYS_PATH + CMD_ARGV(0)).c_str() );
	}
}

void fnreplay_variants(DkList<EqString>& list, const char* query)
{
	DKFINDDATA* findData = nullptr;
	char* fileName = (char*)g_fileSystem->FindFirst(USERREPLAYS_PATH "*.rdat", &findData, SP_MOD);

	if(fileName)
	{
		list.append(_Es(fileName).Path_Strip_Ext());

		while(fileName = (char*)g_fileSystem->FindNext(findData))
		{
			if(!g_fileSystem->FindIsDirectory(findData))
				list.append(_Es(fileName).Path_Strip_Ext());
		}

		g_fileSystem->FindClose(findData);
	}
}

DECLARE_CMD_VARIANTS(replay, "starts specified replay", fnreplay_variants, 0)
{
	if(CMD_ARGC > 0)
	{
		g_State_Game->StartReplay( (USERREPLAYS_PATH + CMD_ARGV(0)).c_str(), false );
	}
	else
	{
		MsgWarning("usage: replay <replayName>");
	}
}