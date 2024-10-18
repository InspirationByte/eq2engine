#pragma once

enum EShaderKindFlags : int
{
	SHADERKIND_VERTEX = (1 << 0),
	SHADERKIND_FRAGMENT = (1 << 1),
	SHADERKIND_COMPUTE = (1 << 2),
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
	SHADERSOURCE_UNDEFINED = -1,
	SHADERSOURCE_HLSL,
	SHADERSOURCE_GLSL,
	//SHADERSOURCE_WGSL,
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
		shaderc::SpvCompilationResult data;
		EqString		queryStr;
		int				entryPointId{ -1 };
		int				kindFlag{ -1 };
		int				refResult{ -1 };
		int				vertLayoutIdx{ -1 };
		uint32			crc32{ 0 };
	};
	struct SkipCombo
	{
		Array<EqString>	defines{ PP_SL };
	};
	struct EntryPoint
	{
		EqString		name;
		int				kind{ 0 };
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
	EShaderSourceType	sourceType{ SHADERSOURCE_UNDEFINED };

	uint32				crc32{ 0 };
	int					totalVariationCount{ 0 };

	EType				type{ SHADER_BASE };
};