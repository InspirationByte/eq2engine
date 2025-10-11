#pragma once

enum EShaderKind : int
{
	SHADERKIND_VERTEX	= (1 << 0),
	SHADERKIND_FRAGMENT	= (1 << 1),
	SHADERKIND_COMPUTE	= (1 << 2),
};

enum EShaderModuleType
{
	SHADERMODULE_SPIRV,		// Vulkan & WGPU
	SHADERMODULE_DXBC,		// D3D10+
	SHADERMODULE_DXIL,		// D3D12
	SHADERMODULE_WGSL,		// WGPU only

	SHADERMODULE_TYPES,
};

static const char* s_shaderModuleTypeExt[] = {
	".spv",
	".dxbc",
	".dxil",
	".wgsl",
};
static_assert(SHADERMODULE_TYPES == elementsOf(s_shaderModuleTypeExt), "BINDENTRY_TYPES doesn't match SHADERMODULE_TYPES count");

static const char* s_shaderModuleTypeName[] = {
	"SPIRV",
	"DXBC",
	"DXIL",
	"WGSL",
};
static_assert(SHADERMODULE_TYPES == elementsOf(s_shaderModuleTypeName), "SHADERMODULE_TYPES doesn't match s_shaderModuleTypeName count");

enum ERWFlags : int
{
	RWFLAG_UNIFORM = (1 << 0),
	RWFLAG_READ = (1 << 1),
	RWFLAG_WRITE = (1 << 2),
};

enum EBindingRangeType : int
{
	BINDING_RANGE_SRV = 0,
	BINDING_RANGE_UAV,
	BINDING_RANGE_CBV,
	BINDING_RANGE_SAMPLER
};

//-----------------------------------------

enum EBindEntryType : int
{
	BINDENTRY_BUFFER = 0,
	BINDENTRY_SAMPLER,
	BINDENTRY_TEXTURE,
	BINDENTRY_STORAGETEXTURE,

	BINDENTRY_TYPES,
};

static const char* s_bindingTypeNames[] = {
	"buffer",
	"sampler",
	"texture",
	"storagetexture",
};
static_assert(BINDENTRY_TYPES == elementsOf(s_bindingTypeNames), "BINDENTRY_TYPES doesn't match s_bindingTypeNames count");

//-----------------------------------------

enum EShaderSourceType : int
{
	SHADERSOURCE_SLANG = 0,
	SHADERSOURCE_HLSL,
	SHADERSOURCE_GLSL,

	SHADERSOURCE_COUNT,
};

struct ShaderInfo
{
	enum EType
	{
		SHADER_BASE,
		SHADER_EXT,
		SHADER_PACKAGE
	};

	struct Binding
	{
		EqString			name;
		EBindEntryType		type{ BINDENTRY_BUFFER };
		int					rwFlags{ RWFLAG_UNIFORM };

		int					descriptorSetIdx{ -1 };
		int					index{ -1 };
		EBindingRangeType	rangeType{};
		int					registerIdx{ 0 };
	};
	struct VertLayout
	{
		EqString name;
		int aliasOf{ -1 };
		Array<EqString>	excludeDefines{ PP_SL };
	};
	struct Variant
	{
		EqString		name;
		int				baseVariant{ -1 };
		Array<EqString>	defines{ PP_SL };
	};
	struct VertexAttrib
	{
		EqString name;
		EqString semantic;
		int location;
	};
	struct Result
	{
		mutable CMemoryStream	data[SHADERMODULE_TYPES];
		uint32					crc32[SHADERMODULE_TYPES]{ 0 };

		EqString			queryStr;
		int					entryPointIdx{ -1 };
		int					kindFlag{ -1 };
		int					refResult{ -1 };
		int					vertLayoutIdx{ -1 };
		Array<Binding>		bindings{ PP_SL };
		Array<VertexAttrib>	vertexAttribs{ PP_SL };
		bool				isError{ false };
	};
	struct SkipCombo
	{
		Array<EqString>	defines{ PP_SL };
	};
	struct EntryPoint
	{
		EqString		name;
		EShaderKind		kind{ };
	};
	struct AddFile
	{
		EqString		fileName;
		Array<EqString>	values{ PP_SL };
	};

	Array<Result>		results{ PP_SL };
	Array<EntryPoint>	entryPoints{ PP_SL };
	Array<VertLayout>	vertexLayouts{ PP_SL };
	Array<Variant>		variants{ PP_SL };
	Array<SkipCombo>	skipCombos{ PP_SL };
	Array<AddFile>		addedFiles{ PP_SL };

	EqString			name;
	EqString			sourceText;
	EqString			sourceFilename;

	EShaderSourceType	sourceType{ SHADERSOURCE_SLANG };

	uint32				crc32{ 0 };
	int					totalVariationCount{ 0 };

	EType				type{ SHADER_BASE };
	bool				debugInfo{ false };
	bool				skipOptimize{ false };
};