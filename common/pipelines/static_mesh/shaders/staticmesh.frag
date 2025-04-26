#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "projview.hpp"
#include "material.hpp"

layout(location = 0) out vec4 out_fragColor;
layout(location = 1) out vec4 out_fragNormal;
layout(location = 2) out vec4 out_fragMaterial;
layout(location = 3) out float out_fragWc;

layout(location = 0) in VS_OUT
{
  vec3 wPos;
  vec4 wNorm;
  vec4 wTangent;
  vec2 texCoord;
  vec2 normTexCoord;
  flat uint material;
} surf;

layout (std140, set = 0, binding = 0) readonly buffer ims_t {
  mat4 mModels[]; 
} ims;

layout(set = 1, binding = 1) uniform sampler2D textures[N_MAX_TEXTURES];

layout (std140, set = 1, binding = 0) readonly buffer mat_t {
  GpuMaterial materials[N_MAX_MATERIALS]; 
};

layout(set = 2, binding = 0) uniform WVPM_t {
  WorldViewProjMatrices world;
};



layout(push_constant) uniform params_t
{
  mat4 mModel;
  vec4 color;
  vec4 emr_;
  vec3 pos;
  uint relemIdx;
  uint wId;
  uint material;
} params;


const vec2 resolution = vec2(1280, 720);

vec3 getPos() {
  return vec3(
    (2 * gl_FragCoord.x / resolution.x) - 1,
    (2 * gl_FragCoord.y / resolution.y) - 1,
    gl_FragCoord.z
  ) / gl_FragCoord.w;
}

void main()
{
  const uint material = surf.material;
  out_fragColor = texture(textures[materials[material].baseColorTexture], surf.texCoord) * materials[material].baseColor;
  vec3 bitangent = normalize(cross(surf.wNorm.xyz, surf.wTangent.xyz));
  vec4 normalMap = texture(textures[materials[material].normalTexture], surf.texCoord); 
  normalMap = 2 * normalMap - 1;
  vec4 wNormal = vec4(normalize(surf.wNorm.xyz * normalMap.b + surf.wTangent.xyz * normalMap.g + bitangent * normalMap.r), 0);

  out_fragNormal = vec4(normalize(transpose(mat3(world.mIView[0])) * wNormal.xyz), 0);

  out_fragWc = gl_FragCoord.w;
  out_fragMaterial =  materials[material].emr_factor * texture(textures[materials[material].metallicRoughnessTexture], surf.normTexCoord);
}
