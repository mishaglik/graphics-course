#ifndef SHADERS_MATERIAL_HPP
#define SHADERS_MATERIAL_HPP

#include "cpp_glsl_compat.h"

#define N_MAX_MATERIALS 32
#define N_MAX_TEXTURES 36

struct GpuMaterial{
    shader_uint baseColorTexture;
    shader_uint normalTexture;
    shader_uint metallicRoughnessTexture;
    shader_uint emissiveFactorTexture;
    shader_vec4 baseColor;
    shader_vec4 emr_factor;
};



#endif /* SHADERS_MATERIAL_HPP */
