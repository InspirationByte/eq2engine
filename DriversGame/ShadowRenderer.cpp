//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate shadow renderer
//////////////////////////////////////////////////////////////////////////////////

#include "ShadowRenderer.h"
#include "ConVar.h"

ConVar r_shadowAtlasSize("r_shadowAtlasSize", "512", 256.0f,  512.0f, NULL, CV_ARCHIVE);

CShadowRenderer::CShadowRenderer() : m_shadowTexture(NULL)
{
	m_texAtlasPacker.SetPackPadding( 1.0 );
}

CShadowRenderer::~CShadowRenderer()
{
}

void CShadowRenderer::Init()
{
	m_shadowTextureSize = r_shadowAtlasSize.GetFloat();
	m_shadowTexelSize = 1.0f / m_shadowTextureSize;

	// create shadow render target
	m_shadowTexture = g_pShaderAPI->CreateRenderTarget(m_shadowTextureSize.x,m_shadowTextureSize.y, FORMAT_R8);
	m_shadowTexture->Ref_Grab();

	CSpriteBuilder::Init();
}

void CShadowRenderer::Shutdown()
{
	Clear();

	g_pShaderAPI->FreeTexture(m_shadowTexture);
	m_shadowTexture = NULL;

	CSpriteBuilder::Shutdown();
}

struct shadowInfo_t
{
	CGameObject*	object; // object which casts shadows
	Vector3D		origin; // for fadeout
	float			size;
};

void CShadowRenderer::AddShadowCaster( CGameObject* object )
{
/*
	float shadowSize = length(m_bbox.GetSize());

	CViewParams orthoView;
	orthoView.SetOrigin(m_vecOrigin);
	orthoView.SetAngles(g_pGameWorld->m_envConfig.sunAngles);
	orthoView.SetFOV(0.5f);

	Matrix4x4 proj, view, viewProj;
	orthoView.GetMatrices(proj, view, shadowSize, shadowSize, -shadowSize, 1000.0f, true );
	viewProj = proj*view;

	Volume shadowVolume;
	shadowVolume.LoadAsFrustum(viewProj);

	// debug rendering of shadow
	decalprimitives_t shadowDecal;
	g_pGameWorld->m_level.GetDecalPolygons(shadowDecal, shadowVolume);

	for(int i = 0; i < shadowDecal.indices.numElem(); i+= 3)
	{
		int i1 = shadowDecal.indices[i];
		int i2 = shadowDecal.indices[i+1];
		int i3 = shadowDecal.indices[i+2];

		// add position because physics polys are not moved
		Vector3D p1 = shadowDecal.verts[i1].point;
		Vector3D p2 = shadowDecal.verts[i2].point;
		Vector3D p3 = shadowDecal.verts[i3].point;

		debugoverlay->Polygon3D(p1,p2,p3,ColorRGBA(0,0,0,0.5), 0.0f);
	}
	*/
}

void CShadowRenderer::Clear()
{
	m_texAtlasPacker.Cleanup();
	CSpriteBuilder::ClearBuffers();

	g_pShaderAPI->ChangeRenderTarget(m_shadowTexture);
	g_pShaderAPI->Clear(true,false,false, ColorRGBA(1.0f));

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();
}

inline int AtlasPackComparison(PackerRectangle *const &elem0, PackerRectangle *const &elem1)
{
	return (elem1->width + elem1->height) - (elem0->width + elem0->height);
}

void CShadowRenderer::Render()
{
	Vector2D& neededTexSize = m_shadowTextureSize;

	// process 
	if(!m_texAtlasPacker.AssignCoords(neededTexSize.x,neededTexSize.y, AtlasPackComparison))
		return; // don't render shadows, size overflow

	// copy pixels
	for(int i = 0; i < m_texAtlasPacker.GetRectangleCount(); i++)
	{
		PackerRectangle* rect = m_texAtlasPacker.GetRectangle(i);
		shadowInfo_t* shadowData = (shadowInfo_t*)rect->userdata;

		// render model

		// generate decal geometry

		// push geometry
	}

	// upload to vertex & index buffers

	// render
}