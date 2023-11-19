//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Shared scene parameter definitions
//////////////////////////////////////////////////////////////////////////////////

#pragma once

enum EMatrixMode : int
{
	MATRIXMODE_VIEW = 0,			// view tranformation matrix
	MATRIXMODE_PROJECTION,			// projection mode matrix

	MATRIXMODE_WORLD,				// world transformation matrix
	MATRIXMODE_WORLD2,				// world transformation - used as offset for MATRIXMODE_WORLD

	MATRIXMODE_TEXTURE,
};

// fog parameters
struct FogInfo
{
	Vector3D	viewPos;
	Vector3D	fogColor;
	float		fogdensity;
	float		fognear;
	float		fogfar;
	bool		enableFog{ false };
};

//---------------------------------------------
// structs used in shader

struct MatSysCamera
{
	Matrix4x4	ViewProj;
	Matrix4x4	View;
	Matrix4x4	Proj;
	Vector3D	Pos;
	Vector3D	FogColor;		// .a is factor
	float		padding[2];
	float		FogFactor;
	float		FogNear;
	float		FogFar;
	float		FogScale;
};