#ifndef SHADERS_PARTICLEINFO_HPP
#define SHADERS_PARTICLEINFO_HPP

#include "cpp_glsl_compat.h"

struct ParticleInfo {
    shader_vec4 position;
};
#define N_MAX_EMITTERS 64
#define N_MAX_PARTICLES_PER_DRAW 1024
struct EmitterInfo {
    shader_vec4  position;
    shader_vec2  size;
    shader_uint  type;
    shader_uint  material;
    shader_vec4  fadeColor;
    shader_vec4  fadeSize_pad;
    shader_vec4  fadeBezier;
};


#endif /* SHADERS_PARTICLEINFO_HPP */
