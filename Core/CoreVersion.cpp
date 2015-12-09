//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Core version
//////////////////////////////////////////////////////////////////////////////////

#include "CoreVersion.h"
#include "DebugInterface.h"

void CoreMessage()
{
#ifdef _WIN32
	MsgInfo(" \n\"%s\" build %i (WIN32) of date %s\n\n",ENGINE_NAME ,BUILD_NUMBER ,COMPILE_DATE);
#else
    MsgInfo(" \n\"%s\" build %i (LINUX) of date %s\n\n",ENGINE_NAME ,BUILD_NUMBER ,COMPILE_DATE);
#endif
}

void CC_coreversion_f(DkList<EqString> *args)
{
	MsgInfo("\"%s\" %s v%s build %i\n Compilation date: %s %s\n",ENGINE_NAME,ENGINE_DEVSTATE ,ENGINE_VERSION,BUILD_NUMBER ,COMPILE_DATE,COMPILE_TIME);
}
ConCommand cmd_coreversion("coreversion",CC_coreversion_f,"Print out full engine version",CV_UNREGISTERED);

void CC_info_f(DkList<EqString> *args)
{
	MsgInfo("\"%s\" by Inspiration Byte\nCopyright (C) 2009-2015 Inspiration Byte L.L.C Kazakhstan. Original engine written by Shurumov Ilia\n",ENGINE_NAME);
}
ConCommand cmd_info("info",CC_info_f,"Print out engine info",CV_UNREGISTERED);
