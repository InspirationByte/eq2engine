//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Constant types for Equilibrium renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef D3DRENDERER_SHADERS_H
#define D3DRENDERER_SHADERS_H

const char *plainVS =
"float4 scaleBias : register( c90 );"
"float4 vs_main(float4 position: POSITION): POSITION {"
"	position.xy = position.xy * scaleBias.xy + scaleBias.zw;"
"	return position;"
"}";

const char *plainPS =
"float4 color : register(c0);"
"float4 ps_main(): COLOR {"
"	return color;"
"}";


const char *flatVS =
"float4x4 mvp : register(c90);"
"float4 vs_main(float4 vPos: POSITION): POSITION{"
"	float4 pos = mul(mvp,vPos);"
"	return pos;"
"}";

const char *texVS =
"struct VsIn {"
"	float4 position: POSITION;"
"	float2 texCoord: TEXCOORD;"
"};"
"float4 scaleBias : register(c90);"
"VsIn vs_main(VsIn In){"
"	In.position.xy = In.position.xy * scaleBias.xy + scaleBias.zw;"
"	return In;"
"}";

const char *texPS =
"sampler2D Base: register(s0);"
"float4 color : register(c0);"
"float4 ps_main(float2 texCoord: TEXCOORD): COLOR {"
"	return tex2D(Base, texCoord) * color;"
"}";

#endif //D3DRENDERER_SHADERS_H