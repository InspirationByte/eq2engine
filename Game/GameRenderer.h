//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Client game renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAMERENDERER_H
#define GAMERENDERER_H

#include "math/Vector.h"
#include "IShaderAPI.h"
#include "RenderList.h"
#include "RenderDefs.h"

#include "ConVar.h"
#include "Utils/eqstring.h"
#include "IViewRenderer.h"
#include "dsm_obj_loader.h"

class IMaterial;
class BaseEntity;

//extern CBasicRenderList*	g_staticProps;
extern CBasicRenderList*	g_modelList;
extern CBasicRenderList*	g_viewModels;
extern CBasicRenderList*	g_transparentsList;

// rope vertex
struct eqropevertex_t
{
	Vector3D point;
	Vector3D texCoord_segIdx; // x,y as texcoord, z as segment/bone index
	Vector3D normal;
};

struct detail_node_t
{
	int			nDetailType;
	int			nDetailSubmodel;

	Vector3D	position;
	float		scale;
};

struct hwdetail_vertex_t
{
	Vector3D v;
	Vector2D t;
};

// comparsion operator
inline bool operator == (const hwdetail_vertex_t &u, const hwdetail_vertex_t &v)
{
	if(u.v == v.v && u.t == v.t)
		return true;

	return false;
}


struct detail_model_t
{
	DkList<hwdetail_vertex_t>	verts;
	DkList<int16>				indices;

	int							startIndex;
	int							startVertex;

	IMaterial*					material;
};

struct detail_type_t
{
	EqString				detailname;
	DkList<detail_model_t*>	models;
};

class CDetailSystemTest
{
public:
	CDetailSystemTest()
	{
		m_pDetailFormat = NULL;
		m_pDetailVertexBuffer = NULL;
		m_pDetailIndexBuffer = NULL;

		m_bInit = false;
	}

	void Init()
	{
		if(m_bInit)
			return;

		// init detail types
		KeyValues kv;

		if(kv.LoadFromFile("detailstypes.edt"))
		{
			kvkeybase_t* pKB = kv.GetRootSection();

			for(int i = 0; i < pKB->keys.numElem(); i++)
			{
				kvkeybase_t* pDetTypeBase = pKB->keys[i];

				detail_type_t* det_type = new detail_type_t;
				det_type->detailname = pDetTypeBase->name;
				m_pDetailTypes.append(det_type);

				for(int j = 0; j < pDetTypeBase->keys.numElem(); j++)
				{
					// check for models
					if(pDetTypeBase->keys[j]->IsSection())
					{
						detail_model_t* model = new detail_model_t;

						if(LoadOBJIntoDetail(pDetTypeBase->keys[j], model))
							det_type->models.append(model);
						else
							delete model;
					}

					
				}
			}

			BuildVBO();

			m_bInit = true;
		}
	}

	void AddNode(const Vector3D& position, int type)
	{
		if(!m_bInit)
			return;

		detail_type_t* detType = m_pDetailTypes[type];

		if(detType->models.numElem() == 0)
			return;

		int nRandomModel = RandomInt(0, detType->models.numElem()-1);

		detail_node_t newNode;
		newNode.nDetailSubmodel = nRandomModel;
		newNode.nDetailType = type;
		newNode.position = position;
		newNode.scale = RandomFloat(0.8f, 1.2f);

		m_pNodes.append(newNode);

		Msg("Added node\n");
	}

	void ClearNodes()
	{
		m_pNodes.clear();
	}

	void DrawNodes();

private:
	void BuildVBO()
	{
		if(!m_pDetailFormat)
		{
			VertexFormatDesc_t pFormat[] = {
				{ 0, 3, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_FLOAT },	  // position
				{ 0, 2, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // texcoord 0
			};

			m_pDetailFormat = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));
		}

		if(m_pDetailIndexBuffer)
		{
			g_pShaderAPI->DestroyIndexBuffer(m_pDetailIndexBuffer);
			m_pDetailIndexBuffer = NULL;
		}

		if(m_pDetailVertexBuffer)
		{
			g_pShaderAPI->DestroyVertexBuffer(m_pDetailVertexBuffer);
			m_pDetailVertexBuffer = NULL;
		}

		DkList<hwdetail_vertex_t>	verts;
		DkList<int16>				indices;

		for(int i = 0; i < m_pDetailTypes.numElem(); i++)
		{
			for(int j = 0; j < m_pDetailTypes[i]->models.numElem(); j++)
			{
				detail_model_t* pMod = m_pDetailTypes[i]->models[j];

				pMod->startVertex = verts.numElem();
				pMod->startIndex = indices.numElem();
				
				verts.append( pMod->verts );

				for(int k = 0; k < pMod->indices.numElem(); k++)
					indices.append( pMod->indices[k] + pMod->startVertex );
			}
		}

		m_pDetailVertexBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_STATIC, verts.numElem(), sizeof(hwdetail_vertex_t), verts.ptr());
		m_pDetailIndexBuffer = g_pShaderAPI->CreateIndexBuffer(indices.numElem(), INDEX_SIZE_SHORT, BUFFER_STATIC, indices.ptr());
	}

	bool LoadOBJIntoDetail(kvkeybase_t* pSec, detail_model_t* pDest)
	{
		kvkeybase_t* pModel = pSec->FindKeyBase("model");
		kvkeybase_t* pScale = pSec->FindKeyBase("scale");

		dsmmodel_t tempModel;
		if( LoadOBJ( &tempModel, KV_GetValueString(pModel) ) )
		{
			// get first group
			dsmgroup_t* pGroup = tempModel.groups[0];

			pDest->material = materials->FindMaterial( pGroup->texture );

			// create index and vertex data
			for(int16 i = 0; i < pGroup->verts.numElem(); i++)
			{
				hwdetail_vertex_t vtx;
				vtx.v = pGroup->verts[i].position*KV_GetValueFloat(pScale);
				vtx.t = pGroup->verts[i].texcoord;

				int16 idx = pDest->verts.addUnique(vtx);
				pDest->indices.append(idx);
			}

			FreeDSM(&tempModel);
		}
		else
			return false;

		return true;
	}

protected:
	IVertexFormat*			m_pDetailFormat;
	IVertexBuffer*			m_pDetailVertexBuffer;
	IIndexBuffer*			m_pDetailIndexBuffer;

	DkList<detail_node_t>	m_pNodes;
	DkList<detail_type_t*>	m_pDetailTypes;

	bool					m_bInit;
};

//extern CDetailSystemTest* g_pDetailSystem;

// rope verts
extern  DkList<eqropevertex_t>	g_rope_verts;
extern  DkList<uint16>			g_rope_indices;

void	GR_SetupDynamicsIBO();

void	GR_DrawScene( int nRenderFlags = 0 );
void	GR_SetupScene();
void	GR_PostDraw();
void	GR_Init();

void	GR_InitDynamics();
void	GR_Cleanup();

void	GR_Shutdown();
void	GR_Postrender();
void	GR_CopyFramebuffer();
void	GR_Draw2DMaterial(IMaterial* pMaterial);
void	GR_SetViewEntity(BaseEntity* pEnt);

struct PFXBillboard_t;

void	GR_DrawBillboard(PFXBillboard_t* effect);

#endif // GAMERENDERER_H