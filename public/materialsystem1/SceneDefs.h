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
	Matrix4x4	viewProj;
	Matrix4x4	invViewProj;
	Matrix4x4	view;
	Matrix4x4	proj;
	Vector3D	position; float _pad0;
	Vector3D	fogColor; float _pad1;
	Vector2D	zNearFar;
	Vector2D	_pad2;
	float		fogFactor;
	float		fogNear;
	float		fogFar;
	float		fogScale;
};

