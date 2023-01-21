//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Blur filter with vertical and horizontal kernels
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "IMaterialSystem.h"
#include "BaseShader.h"

BEGIN_SHADER_CLASS(VHBlurFilter)

	SHADER_INIT_PARAMS()
	{
		m_pBaseTexture = nullptr;
		m_blurAxes = 0;
		m_blurModes = 0;
		m_texSize = vec4_zero;

		SHADER_PASS(Unlit) = nullptr;

		// set texture setup
		SetParameterFunctor(SHADERPARAM_BASETEXTURE, &ThisShaderClass::SetupBaseTexture0);
	}

	SHADER_INIT_TEXTURES()
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
		SHADER_PARAM_BOOL(BlurX, blurX, false);
		SHADER_PARAM_BOOL(BlurY, blurY, false);

		m_blurAxes |= blurX ? 0x1 : 0;
		m_blurAxes |= blurY ? 0x2 : 0;


		bool blurXLow = false;
		bool blurYLow = false;
		bool blurXHigh = false;
		bool blurYHigh = false;
		SHADER_PARAM_BOOL(XLow, blurXLow, false);
		SHADER_PARAM_BOOL(YLow, blurYLow, false);
		SHADER_PARAM_BOOL(XHigh, blurXHigh, false);
		SHADER_PARAM_BOOL(YHigh, blurYHigh, false);

		m_blurModes |= blurXLow ? 0x1 : 0;
		m_blurModes |= blurYLow ? 0x2 : 0;
		m_blurModes |= blurXHigh ? 0x4 : 0;
		m_blurModes |= blurYHigh ? 0x8 : 0;
	}

	SHADER_INIT_RHI()
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
		SHADER_FIND_OR_COMPILE(Unlit, "VHBlurFilter");

		m_depthtest = false;
		m_depthwrite = false;

		return true;
	}

	SHADER_SETUP_STAGE()
	{
		SHADER_BIND_PASS_SIMPLE(Unlit);
	}

	SHADER_SETUP_CONSTANTS()
	{
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
		const ITexturePtr& pSetupTexture = materials->GetConfiguration().wireframeMode ? materials->GetWhiteTexture() : m_pBaseTexture;
		g_pShaderAPI->SetTexture(pSetupTexture, "BaseTexture", 0);
	}

	const ITexturePtr& GetBaseTexture(int stage) const {return m_pBaseTexture;}
	const ITexturePtr& GetBumpTexture(int stage) const {return nullptr;}

	ITexturePtr		m_pBaseTexture;
	int				m_blurAxes;
	int				m_blurModes;

	Vector4D		m_texSize;

	SHADER_DECLARE_PASS(Unlit);

END_SHADER_CLASS