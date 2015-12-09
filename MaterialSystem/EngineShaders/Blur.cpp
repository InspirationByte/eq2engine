//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Unlit Shader with fog support
//////////////////////////////////////////////////////////////////////////////////

#include "BaseShader.h"

class CBlurFilter : public CBaseShader
{
public:
	CBlurFilter()
	{
		m_pBaseTexture = NULL;
		m_blurAxes = 0;
		m_blurModes = 0;
		SHADER_PASS(Unlit) = NULL;
		m_texSize = vec4_zero;
	}

	void InitParams()
	{
		if(!m_pAssignedMaterial->IsError() && !m_bInitialized && !m_bIsError)
		{
			CBaseShader::InitParams();
		}
	}

	void InitTextures()
	{
		// parse material variables
		SHADER_PARAM_RENDERTARGET_FIND(BaseTexture, m_pBaseTexture);

		if(m_pBaseTexture)
		{
			m_texSize = Vector4D(m_pBaseTexture->GetWidth(), 
								m_pBaseTexture->GetHeight(),
								1.0f / (float)m_pBaseTexture->GetWidth(), 
								1.0f / (float)m_pBaseTexture->GetHeight());
		}

		bool blurX = false;
		bool blurY = false;
		SHADER_PARAM_BOOL(BlurX, blurX);
		SHADER_PARAM_BOOL(BlurY, blurY);

		m_blurAxes |= blurX ? 0x1 : 0;
		m_blurAxes |= blurY ? 0x2 : 0;


		bool blurXLow = false;
		bool blurYLow = false;
		bool blurXHigh = false;
		bool blurYHigh = false;
		SHADER_PARAM_BOOL(XLow, blurXLow);
		SHADER_PARAM_BOOL(YLow, blurYLow);
		SHADER_PARAM_BOOL(XHigh, blurXHigh);
		SHADER_PARAM_BOOL(YHigh, blurYHigh);

		m_blurModes |= blurXLow ? 0x1 : 0;
		m_blurModes |= blurYLow ? 0x2 : 0;
		m_blurModes |= blurXHigh ? 0x4 : 0;
		m_blurModes |= blurYHigh ? 0x8 : 0;

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &CBlurFilter::SetupBaseTexture0);
	}

	bool InitShaders()
	{
		if(SHADER_PASS(Unlit))
			return true;

		// begin shader definitions
		SHADERDEFINES_BEGIN;

		SHADER_DECLARE_SIMPLE_DEFINITION((m_blurAxes & 0x1) > 0, "BLUR_X");
		SHADER_DECLARE_SIMPLE_DEFINITION((m_blurAxes & 0x2) > 0, "BLUR_Y");

		SHADER_DECLARE_SIMPLE_DEFINITION((m_blurModes & 0x1) > 0, "BLUR_X_LOW");
		SHADER_DECLARE_SIMPLE_DEFINITION((m_blurModes & 0x2) > 0, "BLUR_Y_LOW");
		SHADER_DECLARE_SIMPLE_DEFINITION((m_blurModes & 0x4) > 0, "BLUR_X_HIGH");
		SHADER_DECLARE_SIMPLE_DEFINITION((m_blurModes & 0x8) > 0, "BLUR_Y_HIGH");

		// compile without fog
		SHADER_FIND_OR_COMPILE(Unlit, "BlurFilter");

		m_depthtest = false;
		m_depthwrite = false;

		return true;
	}

	void SetupShader()
	{
		if(IsError())
			return;

		SHADER_BIND_PASS_SIMPLE(Unlit);
	}


	void SetupConstants()
	{
		if(IsError())
			return;

		SetupDefaultParameter(SHADERPARAM_TRANSFORM);

		SetupDefaultParameter(SHADERPARAM_BASETEXTURE);

		SetupDefaultParameter(SHADERPARAM_ALPHASETUP);
		SetupDefaultParameter(SHADERPARAM_DEPTHSETUP);
		SetupDefaultParameter(SHADERPARAM_RASTERSETUP);
		SetupDefaultParameter(SHADERPARAM_COLOR);

		g_pShaderAPI->SetShaderConstantVector4D("TEXSIZE", m_texSize);
	}

	void SetColorModulation()
	{
		g_pShaderAPI->SetShaderConstantVector4D("AmbientColor", materials->GetAmbientColor());
	}

	void SetupBaseTexture0()
	{
		ITexture* pSetupTexture = materials->GetConfiguration().wireframeMode ? materials->GetWhiteTexture() : m_pBaseTexture;
		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTextureSampler", 0);
	}

	const char* GetName()
	{
		return "BlurFilter";
	}

	ITexture*	GetBaseTexture(int stage)
	{
		return m_pBaseTexture;
	}

	ITexture*	GetBumpTexture(int stage)
	{
		return NULL;
	}

	// returns main shader program
	IShaderProgram*	GetProgram()
	{
		return SHADER_PASS(Unlit);
	}
private:

	ITexture*			m_pBaseTexture;
	int					m_blurAxes;
	int					m_blurModes;

	Vector4D			m_texSize;

	SHADER_DECLARE_PASS(Unlit);
};

DEFINE_SHADER(BlurFilter, CBlurFilter)