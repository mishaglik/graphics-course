#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require
#include "projview.hpp"

layout (location = 0 ) out VS_OUT
{
  vec3 wPos;
  vec3 wColor;
  vec4 wNorm;
} vOut;

layout(push_constant) uniform pc {
    mat4 mInvPv;
    uint wId;
} params;

layout(set = 0, binding = 0) uniform WVPM_t {
  WorldViewProjMatrices world;
};

void main() {
  vec3 pos;
  if(gl_VertexIndex ==  0) pos = vec3(-1.f,  1.f,  1.f);  
  if(gl_VertexIndex ==  1) pos = vec3( 1.f,  1.f,  1.f);   
  if(gl_VertexIndex ==  2) pos = vec3(-1.f, -1.f,  1.f); 
  if(gl_VertexIndex ==  3) pos = vec3( 1.f, -1.f,  1.f);  
  if(gl_VertexIndex ==  4) pos = vec3( 1.f, -1.f, -1.f); 
  if(gl_VertexIndex ==  5) pos = vec3( 1.f,  1.f,  1.f);   
  if(gl_VertexIndex ==  6) pos = vec3( 1.f,  1.f, -1.f);  
  if(gl_VertexIndex ==  7) pos = vec3(-1.f,  1.f,  1.f);  
  if(gl_VertexIndex ==  8) pos = vec3(-1.f,  1.f, -1.f); 
  if(gl_VertexIndex ==  9) pos = vec3(-1.f, -1.f,  1.f); 
  if(gl_VertexIndex == 10) pos = vec3(-1.f, -1.f, -1.f);
  if(gl_VertexIndex == 11) pos = vec3( 1.f, -1.f, -1.f); 
  if(gl_VertexIndex == 12) pos = vec3(-1.f,  1.f, -1.f); 
  if(gl_VertexIndex == 13) pos = vec3( 1.f,  1.f, -1.f);

  pos.z = (pos.z + 1) / 2;
  vOut.wColor = mix(vec3(0, 1, 0), vec3(1, 0, 0), pos.z);
  vec4 iPos = params.mInvPv * vec4(pos, 1);
  iPos = iPos / iPos.w;
  vOut.wPos = (iPos).xyz;
  vOut.wNorm = vec4(0, 1, 0, 0);
  gl_Position   = world.mProjView[params.wId] * vec4(vOut.wPos, 1.0);
}

