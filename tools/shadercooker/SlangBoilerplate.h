#pragma once

static const char s_boilerPlateStrSlang[] = R"~(

// below declarations from glsl.meta.slang
// P.S.     
//      I kinda find it pretty nice that you can extend Slang in this way
//      as both HLSL and GLSL sucks HLSL sucks even more

internal in uint __sv_InstanceIndex : SV_InstanceID;
internal in uint __sv_VertexIndex : SV_VertexID;
internal in uint __sv_VulkanInstanceIndex : SV_VulkanInstanceID;
internal in uint __sv_VulkanVertexIndex : SV_VulkanVertexID;

// SPIRV InstanceIndex builtin for vertex shader
public property int _InstanceIndex
{
    [require(vertex)]
    get
    {
        __target_switch
        {
        case glsl:
        case spirv:
        case metal:
        case wgsl:
            return __sv_VulkanInstanceIndex;
        default:
            return __sv_InstanceIndex;
        }
    }
}

// SPIRV VertexIndex builtin for vertex shader
public property int _VertexIndex
{
    [require(vertex)]
    get
    {
        __target_switch
        {
        case glsl:
        case spirv:
        case metal:
        case wgsl:
            return __sv_VulkanVertexIndex;
        default:
            return __sv_VertexIndex;
        }
    }
}

//------------------------------------

// Depth texture mapping hacks to WGSL (https://github.com/shader-slang/slang/issues/8503)
// TODO: remove this once Slang properly supports texture_depth_xxx for WGSL

__generic<let sampleCount : int = 0, let format : int = 0>
public typealias DepthTexture2D = _Texture<
    float,
    __Shape2D,
    0, // isArray
    0, // isMS
    sampleCount,
    0, // access
    1, // isShadow
    0, // isCombined
    format
>;

__generic<let sampleCount : int = 0, let format : int = 0>
public typealias DepthTexture2DArray = _Texture<
    float,
    __Shape2D,
    1, // isArray
    0, // isMS
    sampleCount,
    0, // access
    1, // isShadow
    0, // isCombined
    format
>;

__generic<let sampleCount : int = 0, let format : int = 0>
public typealias DepthTexture3D = _Texture<
    float,
    __Shape3D,
    0, // isArray
    0, // isMS
    sampleCount,
    0, // access
    1, // isShadow
    0, // isCombined
    format
>;

__generic<let sampleCount : int = 0, let format : int = 0>
public typealias DepthTexture3DArray = _Texture<
    float,
    __Shape3D,
    1, // isArray
    0, // isMS
    sampleCount,
    0, // access
    1, // isShadow
    0, // isCombined
    format
>;

__generic<let sampleCount : int = 0, let format : int = 0>
public typealias DepthTextureCube = _Texture<
    float,
    __ShapeCube,
    0, // isArray
    0, // isMS
    sampleCount,
    0, // access
    1, // isShadow
    0, // isCombined
    format
>;

__generic<let sampleCount : int = 0, let format : int = 0>
public typealias DepthTextureCubeArray = _Texture<
    float,
    __ShapeCube,
    1, // isArray
    0, // isMS
    sampleCount,
    0, // access
    1, // isShadow
    0, // isCombined
    format
>;

)~";