#ifndef SHADERS_PARTICLEINFO_HPP
#define SHADERS_PARTICLEINFO_HPP

#include "cpp_glsl_compat.h"

struct ParticleInfo {
    shader_vec4 position;
};

struct EmitterInfo {
    shader_vec4 position;
    shader_uint type;
    shader_uint material;
};


#endif /* SHADERS_PARTICLEINFO_HPP */
