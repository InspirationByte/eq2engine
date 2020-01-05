//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate decal generator code
//////////////////////////////////////////////////////////////////////////////////

#include "world.h"
#include "DrvSynDecals.h"
#include "eqParallelJobs.h"
#include "eqGlobalMutex.h"

ConVar r_clipdecals("r_clipdecals", "1");
ConVar r_clipdecalplane("r_clipdecalplane", "-1");

bool DefaultDecalTriangleProcessFunc(struct decalSettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3)
{
	if(dot(NormalOfTriangle(v1.point,v2.point,v3.point), settings.facingDir) < 0.0f)
		return false;

	return true;
}

bool LightDecalTriangleProcessFunc(struct decalSettings_t& settings, PFXVertex_t& v1, PFXVertex_t& v2, PFXVertex_t& v3)
{
	if(dot(NormalOfTriangle(v1.point,v2.point,v3.point), settings.facingDir) < 0.0f)
		return false;

	const Plane& pl1 = settings.clipVolume.GetPlane( VOLUME_PLANE_NEAR );
	const Plane& pl2 = settings.clipVolume.GetPlane( VOLUME_PLANE_FAR );

	float scaleN1 = pl1.Distance(v1.point);
	float scaleN2 = pl1.Distance(v2.point);
	float scaleN3 = pl1.Distance(v3.point);

	float scaleT1 = pl2.Distance(v1.point);
	float scaleT2 = pl2.Distance(v2.point);
	float scaleT3 = pl2.Distance(v3.point);

	scaleN1 = min(scaleN1, 1.0f);
	scaleN2 = min(scaleN2, 1.0f);
	scaleN3 = min(scaleN3, 1.0f);

	scaleT1 = min(scaleT1, 1.0f);
	scaleT2 = min(scaleT2, 1.0f);
	scaleT3 = min(scaleT3, 1.0f);

	v1.color = ColorRGBA(scaleN1*scaleT1);
	v2.color = ColorRGBA(scaleN2*scaleT2);
	v3.color = ColorRGBA(scaleN3*scaleT3);

	return true;
}

decalSettings_t::decalSettings_t()
{
	avoidMaterialFlags = 0;
	facingDir = vec3_up;
	customClipVolume = false;
	skipTexCoords = false;
	userData = nullptr;
	processFunc = DefaultDecalTriangleProcessFunc;
}

//-------------------------------------------------------------------------------------

decalPrimitives_t::decalPrimitives_t()
{
	dirty = true;
}

void decalPrimitives_t::Clear()
{
	verts.clear(false);
	bbox.Reset();
	dirty = true;
}

void decalPrimitives_t::AddTriangle(const Vector3D& p1, const Vector3D& p2, const Vector3D& p3)
{
	PFXVertex_t v1(p1, vec2_zero, color4_white);
	PFXVertex_t v2(p2, vec2_zero, color4_white);
	PFXVertex_t v3(p3, vec2_zero, color4_white);

	if((*settings.processFunc)(settings, v1, v2, v3) == false)
		return;

	verts.append(v1);
	verts.append(v2);
	verts.append(v3);

	dirty = true;
}

decalPrimitivesRef_t::decalPrimitivesRef_t() : 
	verts(nullptr),
	indices(nullptr),
	numVerts(0),
	numIndices(0),
	userData(nullptr)
{
}

//-------------------------------------------------------------------------------------

inline PFXVertex_t lerpVertex(const PFXVertex_t &u, const PFXVertex_t &v, float fac)
{
	PFXVertex_t out;
	out.point = lerp(u.point, v.point, fac);
	out.texcoord = lerp((Vector2D)u.texcoord, (Vector2D)v.texcoord, fac);
	out.color = lerp(u.color, v.color, fac);

	return out;
}

void ClipVerts(DkList<PFXVertex_t>& verts, const Plane &plane)
{
	DkList<PFXVertex_t> new_vertices;
	new_vertices.resize(verts.numElem()*2);

	for(int i = 0; i < verts.numElem(); i += 3)
	{
		PFXVertex_t v[3];
		float d[3];

		for (int j = 0; j < 3; j++)
		{
			v[j] = verts[i + j];
			d[j] = plane.Distance(v[j].point);
		}

		if (d[0] >= 0 && d[1] >= 0 && d[2] >= 0)
		{
			new_vertices.append(verts[i]);
			new_vertices.append(verts[i+1]);
			new_vertices.append(verts[i+2]);
		}
		else if (d[0] < 0 && d[1] < 0 && d[2] < 0)
		{
		}
		else
		{
			int front0 = 0;
			int prev = 2;
			for (int k = 0; k < 3; k++)
			{
				if (d[prev] < 0 && d[k] >= 0)
				{
					front0 = k;
					break;
				}
				prev = k;
			}

			int front1 = (front0 + 1) % 3;
			int front2 = (front0 + 2) % 3;
			if (d[front1] >= 0)
			{
				float x0 = d[front1] / dot(plane.normal, v[front1].point - v[front2].point);
				float x1 = d[front0] / dot(plane.normal, v[front0].point - v[front2].point);

				new_vertices.append(verts[i+front0]);
				new_vertices.append(verts[i+front1]);
				new_vertices.append(lerpVertex(verts[i+front1], verts[i+front2], x0));

				new_vertices.append(verts[i+front0]);
				new_vertices.append(lerpVertex(verts[i+front1], verts[i+front2], x0));
				new_vertices.append(lerpVertex(verts[i+front0], verts[i+front2], x1));
			}
			else
			{
				float x0 = d[front0] / dot(plane.normal, v[front0].point - v[front1].point);
				float x1 = d[front0] / dot(plane.normal, v[front0].point - v[front2].point);

				new_vertices.append(verts[i+front0]);
				new_vertices.append(lerpVertex(verts[i+front0], verts[i+front1], x0));
				new_vertices.append(lerpVertex(verts[i+front0], verts[i+front2], x1));
			}
		}
	}

	// swap the lists
	new_vertices.swap(verts);
}

//--------------------------------------------------------------------------------------------------------------

void DecalClip(decalPrimitives_t& decal)
{
	if (r_clipdecals.GetBool())
	{
		for (int i = 0; i < 6; i++)
		{
			const Plane& pl = decal.settings.clipVolume.GetPlane(i);

			if (r_clipdecalplane.GetInt() == -1 || r_clipdecalplane.GetInt() == i)
				ClipVerts(decal.verts, pl);
		}
	}

	for (int i = 0; i < decal.verts.numElem(); i++)
		decal.bbox.AddVertex(decal.verts[i].point);
}

void _DecalTextureVertex(PFXVertex_t& vert, const Matrix4x4& texCoordProj, const Rectangle_t& atlasRect, const ColorRGBA& color)
{
	Vector4D projCoords = texCoordProj * Vector4D(vert.point, 1.0f);

	projCoords.x *= -1.0f;

	Vector4D proj = projCoords + projCoords.w;
	Vector2D texCoord = proj.xy() / proj.w;

	vert.texcoord = lerp(atlasRect.vrightBottom, atlasRect.vleftTop, texCoord);
	vert.color = ColorRGBA(vert.color) * color; // multiply
}

// clips decals by specified planes and calculates texture coordinates
void DecalTexture(decalPrimitives_t& decal, const Matrix4x4& texCoordProj, const Rectangle_t& atlasRect, const ColorRGBA& color)
{
	for(int i = 0; i < decal.verts.numElem(); i++)
	{
		PFXVertex_t& vert = decal.verts[i];
		_DecalTextureVertex(vert, texCoordProj, atlasRect, color);
	}
}

void DecalTexture(decalPrimitivesRef_t& decal, const Matrix4x4& texCoordProj, const Rectangle_t& atlasRect, const ColorRGBA& color)
{
	for (int i = 0; i < decal.numVerts; i++)
	{
		PFXVertex_t& vert = decal.verts[i];
		_DecalTextureVertex(vert, texCoordProj, atlasRect, color);
	}
}

decalPrimitivesRef_t ProjectDecalToSpriteBuilder(decalPrimitives_t& decal, CSpriteBuilder<PFXVertex_t>* group, const Rectangle_t& rect, const Matrix4x4& viewProj, const ColorRGBA& color)
{
	decalPrimitivesRef_t ref;
	ref.userData = decal.settings.userData;

	if (decal.dirty)	// if marked dirty specially
	{
		// make shadow volume and get our shadow polygons from world
		if (!decal.settings.customClipVolume)
			decal.settings.clipVolume.LoadAsFrustum(viewProj);

		g_pGameWorld->m_level.GetDecalPolygons(decal, &g_pGameWorld->m_occludingFrustum);

		// apply projection coords
		if(!decal.settings.skipTexCoords)
			DecalTexture(decal, viewProj, rect, color);

		// clip decal polygons by volume
		DecalClip(decal);

		// it's dirty if there are still no verts
		decal.dirty = decal.verts.numElem() == 0;
	}
	
	int numVerts = decal.verts.numElem();

	if (!numVerts)
		return ref;

	PFXVertex_t* verts;
	int startIdx = -1;

	{
		CScopedMutex m(GetGlobalMutex(MUTEXPURPOSE_RENDERER));
		startIdx = group->AllocateGeom(numVerts, 0, &verts, NULL, false);
	}

	// push geometry
	if (startIdx != -1)
	{
		memcpy(verts, decal.verts.ptr(), numVerts * sizeof(PFXVertex_t));
		ref.verts = verts;
		ref.numVerts = numVerts;
	}

	return ref;
}

// JOB version for eqParallelJobs

struct eqDecalPolygonJob_t
{
	eqParallelJob_t					base;
	decalSettings_t					settings;
	CSpriteBuilder<PFXVertex_t>*	group;
	Rectangle_t						rect;
	Matrix4x4						viewProj;
	ColorRGBA						color;
};

void DecalJobCompleted(eqParallelJob_t* job)
{
	eqDecalPolygonJob_t* decalData = (eqDecalPolygonJob_t*)job;

	// required to delete job manually here
	// becuase eqDecalPolygonJob_t would have destructors
	delete decalData;
}

void DecalPolygonsJob(void* data, int i)
{
	eqDecalPolygonJob_t* decalData = (eqDecalPolygonJob_t*)data;
	decalPrimitives_t decal;
	decal.settings = decalData->settings;
	ProjectDecalToSpriteBuilder(decal, decalData->group, decalData->rect, decalData->viewProj, decalData->color);
}

void ProjectDecalToSpriteBuilderAddJob(const decalSettings_t& settings, CSpriteBuilder<PFXVertex_t>* group, const Rectangle_t& rect, const Matrix4x4& viewProj, const ColorRGBA& color)
{
	eqDecalPolygonJob_t* job = new eqDecalPolygonJob_t;
	job->base.func = DecalPolygonsJob;
	job->base.onComplete = DecalJobCompleted;
	job->base.arguments = job;
	job->settings = settings;
	job->group = group;
	job->rect = rect;
	job->viewProj = viewProj;
	job->color = color;

	g_parallelJobs->AddJob((eqParallelJob_t*)job);
}