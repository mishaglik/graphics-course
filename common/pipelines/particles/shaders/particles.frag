#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require
#include "particleInfo.hpp"
#include "material.hpp"
layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
  float fade;
  flat uint material;
  flat uint drawID;
} vIn;

layout(location = 0) out vec4 out_fragColor;

layout(set = 0, binding = 0) readonly buffer et_t{
  EmitterInfo einfo[N_MAX_EMITTERS];
};

layout(set = 1, binding = 1) uniform sampler2DArray textures[N_MAX_TEXTURES];

layout (std140, set = 1, binding = 0) readonly buffer mat_t {
  GpuMaterial materials[N_MAX_MATERIALS]; 
};

layout(push_constant) uniform pc {
    mat4 mProjView;
    uint material;
} params;


void main() {
    uint material = vIn.material;
    float pos = floor(vIn.fade * textureSize(textures[materials[material].baseColorTexture], 0).z + 0.1);
    out_fragColor = texture(textures[materials[material].baseColorTexture], vec3(vIn.texCoord, pos)) * materials[material].baseColor;
    out_fragColor *= mix(vec4(1), einfo[vIn.drawID].fadeColor, vIn.fade);
}
