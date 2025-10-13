
#include <nvrhi/nvrhi.h>
#include "core/core_common.h"
#include "core/ConVar.h"

#include "NVRHIBackend.h"

CNVRHIMessageCallback CNVRHIMessageCallback::Instance;

DECLARE_CVAR_G(nvrhi_validation, "0", nullptr, 0);
DECLARE_CVAR_G(nvrhi_breakOnError, "0", nullptr, 0);

void CNVRHIMessageCallback::message(nvrhi::MessageSeverity severity, const char* messageText)
{
	switch (severity)
	{
	case nvrhi::MessageSeverity::Info:
		Msg("[NVRHI] INFO: %s\n", messageText);
		break;
	case nvrhi::MessageSeverity::Warning:
		MsgWarning("[NVRHI] WARN: %s\n", messageText);
		break;
	case nvrhi::MessageSeverity::Error:
		if (nvrhi_breakOnError.GetBool())
		{
			ASSERT_FAIL("NVRHI ERROR: %s", messageText);
		}
		else
		{
			MsgError("[NVRHI] ERROR: %s\n", messageText);
		}
		break;
	case nvrhi::MessageSeverity::Fatal:
		if (nvrhi_breakOnError.GetBool())
		{
			ASSERT_FAIL("NVRHI FATAL: %s", messageText);
		}

		CrashMsg("NVRHI FATAL: %s", messageText);
		break;
	}
}