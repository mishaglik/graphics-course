#ifndef SHADERS_PARTICLEINFO_HPP
#define SHADERS_PARTICLEINFO_HPP

#include "cpp_glsl_compat.h"

struct ParticleInfo {
    shader_vec4 position;
};
#define N_MAX_EMITTERS 64
#define PARTICLE_SIMULATOR_BLOCK_SIZE 32
#define N_MAX_PARTICLES_PER_EMITTER 256 
#define N_MAX_PARTICLES_TOTAL (N_MAX_EMITTERS * N_MAX_PARTICLES_PER_EMITTER)
#define N_MAX_PARTICLES_PER_DRAW N_MAX_PARTICLES_TOTAL

#if N_MAX_PARTICLES_PER_EMITTER < 2 * PARTICLE_SIMULATOR_BLOCK_SIZE 
#error "Particle capacity must be at least 2 BLOCK_SIZE"
#endif
//NOTE: IMPORTANT: When changing this struct change next structs accordingly!
//LINK common/scene/ParticlesEmitter.hpp:26
//LINK common/scene/ParticlesEmitter.cpp:72
struct EmitterInfo {
    shader_vec4  position;
    shader_vec2  size;
    shader_uint  type;
    shader_uint  material;
    shader_vec4  fadeColor;
    shader_vec4  fadeSize_pad;
    shader_vec4  fadeBezier;
    shader_uint  count;
    shader_uint  pad[3];
};

//NOTE: IMPORTANT: When changing this struct change next structs accordingly!
//LINK common/scene/ParticlesEmitter.hpp:26
//LINK common/scene/ParticlesEmitter.cpp:72
struct EmitterSpawnInfo {
    shader_vec4  direction;         //a is direction factor
    
    shader_float rate;
    shader_float maxSpeed;
    shader_float speedRandomFactor;
    shader_float spawnRadius;

    shader_float lifetime;
    shader_float lifetimeRandomFactor;
    shader_float lastSpawnTime;
    shader_float gravity;
};

#endif /* SHADERS_PARTICLEINFO_HPP */
