//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: occluder set
//////////////////////////////////////////////////////////////////////////////////

#include "occluderSet.h"
#include "IDebugOverlay.h"

#include "ConVar.h"

ConVar r_debugOccluders("r_debugOccluders", "0", NULL, CV_CHEAT);
ConVar r_occlusion("r_occlusion", "1", NULL, CV_CHEAT);

occludingVolume_t::occludingVolume_t(const Vector3D& cameraPos, CLevelRegion* srcReg, levOccluderLine_t* srcOcc)
{
	if(!r_occlusion.GetBool())
		return;

#ifdef EDITOR
	sourceOccluder = srcOcc;
	sourceRegion = srcReg;
#endif // EDITOR

	Plane pl(srcOcc->start, srcOcc->end, srcOcc->end+Vector3D(0, srcOcc->height,0));

	Vector3D a,b;

	if( pl.Distance(cameraPos) < 0 )
	{
		a = srcOcc->start;
		b = srcOcc->end;
	}
	else
	{
		a = srcOcc->end;
		b = srcOcc->start;

		pl.normal *= -1.0f;
		pl.offset *= -1.0f;
	}

	Vector3D aDir = -cross(vec3_up, fastNormalize(cameraPos-a));
	Vector3D bDir = cross(vec3_up, fastNormalize(cameraPos-b));

	Vector3D aTopPos = a + Vector3D(0, srcOcc->height, 0);
	Vector3D bTopPos = b + Vector3D(0, srcOcc->height, 0);

	Vector3D topDir = fastNormalize(cross(aTopPos-bTopPos, cameraPos-bTopPos));

	if(r_debugOccluders.GetBool())
	{
		debugoverlay->Line3D(a, aTopPos, color4_white, color4_white );
		debugoverlay->Line3D(b, bTopPos, color4_white, color4_white );
		debugoverlay->Line3D(aTopPos, bTopPos, color4_white, color4_white );

		Vector3D ml,mr,mt, mid;

		ml = lerp(a, aTopPos, 0.5f);
		mr = lerp(b, bTopPos, 0.5f);
		mt = lerp(aTopPos, bTopPos, 0.5f);
		mid = lerp(a, bTopPos, 0.5f);

		debugoverlay->Line3D(ml, ml+aDir, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1) );
		debugoverlay->Line3D(mr, mr+bDir, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1) );
		debugoverlay->Line3D(mt, mt+topDir, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1) );
		debugoverlay->Line3D(mid, mid+pl.normal, ColorRGBA(0,1,1,1), ColorRGBA(0,1,1,1) );

	}

	planes[0] = pl;
	planes[1] = Plane(aDir, -dot(cameraPos, aDir));
	planes[2] = Plane(bDir, -dot(cameraPos, bDir));
	planes[3] = Plane(topDir, -dot(cameraPos, topDir));
}

bool occludingVolume_t::IsBeingOccluded( const BoundingBox& bbox, float eps) const
{
	// just like frustum check
	for (int i = 0; i < 4; i++)
	{
		if (planes[i].Distance( Vector3D(bbox.minPoint.x, bbox.minPoint.y, bbox.minPoint.z) ) < -eps) return false;
		if (planes[i].Distance( Vector3D(bbox.minPoint.x, bbox.minPoint.y, bbox.maxPoint.z) ) < -eps) return false;
		if (planes[i].Distance( Vector3D(bbox.minPoint.x, bbox.maxPoint.y, bbox.minPoint.z) ) < -eps) return false;
		if (planes[i].Distance( Vector3D(bbox.minPoint.x, bbox.maxPoint.y, bbox.maxPoint.z) ) < -eps) return false;
		if (planes[i].Distance( Vector3D(bbox.maxPoint.x, bbox.minPoint.y, bbox.minPoint.z) ) < -eps) return false;
		if (planes[i].Distance( Vector3D(bbox.maxPoint.x, bbox.minPoint.y, bbox.maxPoint.z) ) < -eps) return false;
		if (planes[i].Distance( Vector3D(bbox.maxPoint.x, bbox.maxPoint.y, bbox.minPoint.z) ) < -eps) return false;
		if (planes[i].Distance( Vector3D(bbox.maxPoint.x, bbox.maxPoint.y, bbox.maxPoint.z) ) < -eps) return false;

		//return true;
	}

	return true;
}

bool occludingVolume_t::IsBeingOccluded( const Vector3D& pos, float radius) const
{
	// just like frustum check
	for (int i = 0; i < 4; i++)
		if (planes[i].Distance( pos ) < radius) return false;

	return true;
}

//-------------------------------------------------------------------------------------

bool occludingFrustum_t::IsBoxVisible( const BoundingBox& bbox) const
{
	if(bypass)
		return true;

	if(!frustum.IsBoxInside(bbox.minPoint, bbox.maxPoint))
		return false;

	if(!r_occlusion.GetBool())
		return true;

	// here is negated
	for(int i = 0; i < occluderSets.numElem(); i++)
	{
		if(occluderSets[i]->IsBeingOccluded(bbox, 0.0f))
			return false;
	}

	return true;
}

bool occludingFrustum_t::IsSphereVisible( const Vector3D& pos, float radius ) const
{
	if(bypass)
		return true;

	if(!frustum.IsSphereInside(pos, radius))
		return false;

	if(!r_occlusion.GetBool())
		return true;

	// here is negated
	for(int i = 0; i < occluderSets.numElem(); i++)
	{
		if(occluderSets[i]->IsBeingOccluded(pos, radius))
			return false;
	}

	return true;
}

bool occludingFrustum_t::IsBoxVisibleThruOcc( const BoundingBox& bbox) const
{
	if(!r_occlusion.GetBool())
		return true;

	if(bypass)
		return true;

	// here is negated
	for(int i = 0; i < occluderSets.numElem(); i++)
	{
		if(occluderSets[i]->IsBeingOccluded(bbox, 0.0f))
			return false;
	}

	return true;
}

bool occludingFrustum_t::IsSphereVisibleThruOcc( const Vector3D& pos, float radius ) const
{
	if(!r_occlusion.GetBool())
		return true;

	if(bypass)
		return true;

	// here is negated
	for(int i = 0; i < occluderSets.numElem(); i++)
	{
		if(occluderSets[i]->IsBeingOccluded(pos, radius))
			return false;
	}

	return true;
}
