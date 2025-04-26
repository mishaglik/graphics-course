#ifndef SHADERS_DRAWCMD_HPP
#define SHADERS_DRAWCMD_HPP

#include "cpp_glsl_compat.h"

struct DrawCmd {
    shader_uint indexCount;
    shader_uint instanceCount;
    shader_uint firstIndex;
    shader_uint vertexOffset;
    shader_uint firstInstance;
    shader_uint material;
    shader_uint pad1;
    shader_uint pad2;
};



#endif /* SHADERS_DRAWCMD_HPP */
