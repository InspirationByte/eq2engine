#include "core/core_common.h"
#include "sys_esl.h"

#include "sys_esl_matsystem.h"
#include "materialsystem1/IMaterialSystem.h"

EQSCRIPT_TYPE_BEGIN(IMaterialSystem)

	EQSCRIPT_BIND_FUNC(GetMaterialPath)
	EQSCRIPT_BIND_FUNC(GetMaterialSRCPath)

	EQSCRIPT_BIND_FUNC(CreateMaterial)
	EQSCRIPT_BIND_FUNC(GetMaterial)

	EQSCRIPT_BIND_FUNC(QueueLoading)
	EQSCRIPT_BIND_FUNC(PreloadNewMaterials)
	EQSCRIPT_BIND_FUNC(WaitAllMaterialsLoaded)
	EQSCRIPT_BIND_FUNC(GetLoadingQueue)
	EQSCRIPT_BIND_FUNC(ReleaseUnusedMaterials)

	// TODO: material var bindings
EQSCRIPT_TYPE_END

EQSCRIPT_TYPE_BEGIN(IMaterial)
	EQSCRIPT_BIND_FUNC(GetName)
	EQSCRIPT_BIND_FUNC(GetShaderName)

	EQSCRIPT_BIND_FUNC(GetFlags)

	EQSCRIPT_BIND_FUNC(GetState)
	EQSCRIPT_BIND_FUNC(IsError)

	EQSCRIPT_BIND_FUNC(LoadShaderAndTextures)
	EQSCRIPT_BIND_FUNC(WaitForLoading)

	// TODO: material var bindings
EQSCRIPT_TYPE_END

bool eslSysMaterialSystemInit(const esl::ScriptState& state)
{
	state.RegisterClass<IMaterialSystem>();
	state.RegisterClass<IMaterial>();

	state.SetGlobal("matSystem", g_matSystem);

	return true;
}