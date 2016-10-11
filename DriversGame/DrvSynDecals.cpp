//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate decal generator code
//////////////////////////////////////////////////////////////////////////////////

#include "DrvSynDecals.h"
#include "math/math_util.h"
#include "ConVar.h"

inline PFXVertex_t lerpVertex(const PFXVertex_t &u, const PFXVertex_t &v, float fac)
{
	PFXVertex_t out;
	out.point = lerp(u.point, v.point, fac);
	out.texcoord = lerp((Vector2D)u.texcoord, (Vector2D)v.texcoord, fac);
	out.color = lerp(u.color, v.color, fac);

	return out;
}

ConVar r_clipdecals("r_clipdecals", "1");
ConVar r_clipdecalplane("r_clipdecalplane", "-1");

void ClipVerts(DkList<PFXVertex_t>& verts, const Plane &plane)
{
	DkList<PFXVertex_t> new_vertices;

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

void DecalClipAndTexture(decalprimitives_t* decal, const Volume& clipVolume, const Matrix4x4& texCoordProj, const Rectangle_t& atlasRect, float alpha)
{
	for(int i = 0; i < decal->verts.numElem(); i++)
	{
		Vector3D pos = decal->verts[i].point;

		Vector4D projCoords = texCoordProj*Vector4D(pos,1.0f)*1.0f;

		projCoords.x *= -1.0f;

		Vector4D proj 		= projCoords + projCoords.w;
		Vector2D texCoord	= proj.xy() / proj.w;

		decal->verts[i].point.y += 0.01f;;
		decal->verts[i].texcoord = lerp(atlasRect.vrightBottom, atlasRect.vleftTop, texCoord);
		decal->verts[i].color.w = alpha;
	}

	if(r_clipdecals.GetBool())
	{
		for(int i = 0; i < 6; i++)
		{
			const Plane& pl = clipVolume.GetPlane(i);

			if(r_clipdecalplane.GetInt() == -1 || r_clipdecalplane.GetInt() == i)
				ClipVerts(decal->verts, pl);
		}
	}
}