#pragma once

enum EShaderKind : int
{
	SHADERKIND_VERTEX	= (1 << 0),
	SHADERKIND_FRAGMENT	= (1 << 1),
	SHADERKIND_COMPUTE	= (1 << 2),
};

enum EShaderConvStatus : int
{
	SHADERCONV_INIT = 0,
	SHADERCONV_CRC_LOADED,
	SHADERCONV_COMPILED,
	SHADERCONV_FAILED,
	SHADERCONV_SKIPPED
};

enum EShaderSourceType : int
{
	SHADERSOURCE_HLSL = 0,
	SHADERSOURCE_GLSL,
};

enum EShaderModuleType
{
	SHADERMODULE_SPIRV,
	SHADERMODULE_DXBC,
	SHADERMODULE_DXIL,
	SHADERMODULE_WGSL,		// WGPU only

	SHADERMODULE_TYPES,
};

static const char* s_shaderModuleTypeExt[] = {
	".spv",
	".dxbc",
	".dxil",
	".wgsl",
};

enum EBindGroupId : int
{
	BINDGROUP_UNKNOWN = -1,

	BINDGROUP_CONSTANT = 0,
	BINDGROUP_RENDERPASS = 1,
	BINDGROUP_TRANSIENT = 2,
	BINDGROUP_INSTANCES = 3,
};

static const char* s_bindGroupNames[] = {
	"CONSTANT",
	"RENDERPASS",
	"TRANSIENT",
	"INSTANCES",
};

enum EBindEntryType
{
	BINDENTRY_BUFFER = 0,
	BINDENTRY_SAMPLER,
	BINDENTRY_TEXTURE,
	BINDENTRY_STORAGETEXTURE
};

static const char* s_bindingTypeNames[] = {
	"buffer",
	"sampler",
	"texture",
	"storagetexture",
};

enum ERWFlags : int
{
	RWFLAG_UNIFORM	= (1 << 0),
	RWFLAG_READ		= (1 << 1),
	RWFLAG_WRITE	= (1 << 2),
};

static shaderc_source_language s_sourceLanguage[] = {
	shaderc_source_language_hlsl,
	shaderc_source_language_glsl,
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
		EqString		name;
		EBindGroupId	bindGroupId{ BINDGROUP_UNKNOWN };
		int				index{ -1 };
		EBindEntryType	type{ BINDENTRY_BUFFER };
		int				rwFlags{ RWFLAG_READ | RWFLAG_WRITE };
		int				shaderKind{ 0 };
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
	struct Result
	{
		mutable CMemoryStream	data[SHADERMODULE_TYPES];
		uint32					crc32[SHADERMODULE_TYPES]{ 0 };

		EqString		queryStr;
		int				entryPointIdx{ -1 };
		int				kindFlag{ -1 };
		int				refResult{ -1 };
		int				vertLayoutIdx{ -1 };
		Array<Binding>	bindings{ PP_SL };
		bool			isError{ false };
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

	EShaderConvStatus	status{ SHADERCONV_INIT };
	EShaderSourceType	sourceType{ SHADERSOURCE_HLSL };

	uint32				crc32{ 0 };
	int					totalVariationCount{ 0 };

	EType				type{ SHADER_BASE };
	bool				debugInfo{ false };
	bool				skipOptimize{ false };
};