#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require
#include "material.hpp"
layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
  flat uint material;
} vIn;

layout(location = 0) out vec4 out_fragColor;

layout(set = 1, binding = 1) uniform sampler2D textures[N_MAX_TEXTURES];

layout (std140, set = 1, binding = 0) readonly buffer mat_t {
  GpuMaterial materials[N_MAX_MATERIALS]; 
};

layout(push_constant) uniform pc {
    mat4 mProjView;
    uint material;
} params;

void main() {
    out_fragColor = materials[vIn.material].baseColor;
}
