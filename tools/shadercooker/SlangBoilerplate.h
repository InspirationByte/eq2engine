#pragma once

static const char s_boilerPlateStrSlang[] = R"~(

public static const uint BINDGROUP_CONSTANT = 0;
public static const uint BINDGROUP_RENDERPASS = 1;
public static const uint BINDGROUP_TRANSIENT = 2;
public static const uint BINDGROUP_INSTANCES = 3;

// below declarations from glsl.meta.slang
// P.S.     
//      I kinda find it pretty nice that you can extend Slang in this way
//      as both HLSL and GLSL sucks HLSL sucks even more

internal in int __sv_InstanceIndex : SV_InstanceID;
internal in int __sv_VertexIndex : SV_VertexID;
internal in int __sv_VulkanInstanceIndex : SV_VulkanInstanceID;
internal in int __sv_VulkanVertexIndex : SV_VulkanVertexID;

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


)~";