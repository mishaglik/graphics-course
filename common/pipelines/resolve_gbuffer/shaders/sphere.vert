#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "projview.hpp"

layout(push_constant) uniform pc_t
{
  vec4 pos;
  vec4 color;
  float degree;
  int pbr;
} params;

layout (location = 0 ) out VS_OUT {
  vec3 lightDir;
  vec4 lightSrc;
} vOut;

layout(binding = 5) uniform WVPM_t {
  WorldViewProjMatrices world;
};
const float PI = 3.1415926535897932384626433832795;

void main() {
  int layer = gl_InstanceIndex + (gl_VertexIndex % 2);
  int step  = gl_VertexIndex / 2;
  const float xy_r = max(sin(params.degree * layer), 0.001);
  const vec3 rad = vec3(cos(2 * params.degree * step) * xy_r, sin(2 * params.degree * step) * xy_r, cos(params.degree * layer));
  gl_Position = (world.mProjView[0] * vec4(params.pos.xyz + params.pos.w * rad, 1));
  
  vOut.lightSrc.xyz = (world.mProjView[0] * vec4(params.pos.xyz, 1)).xyz;
 
  vOut.lightDir = normalize(gl_Position.xyz - vOut.lightSrc.xyz);
  vOut.lightSrc.w = 2 * length(gl_Position.xyz - vOut.lightSrc.xyz);
}
