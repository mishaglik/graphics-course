#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "particleInfo.hpp"
#include "material.hpp"

layout (location = 0 ) out VS_OUT
{
  vec2 texCoord;
  flat uint material;
} vOut;

layout(set = 0, binding = 0) readonly buffer pt_t{
  ParticleInfo pinfo[1024];
};

layout (std140, set = 1, binding = 0) readonly buffer mat_t {
  GpuMaterial materials[N_MAX_MATERIALS]; 
};
layout(set = 1, binding = 1) uniform sampler2D textures[N_MAX_TEXTURES];

layout(push_constant) uniform pc {
    mat4 mProjView;
    uint material;
} params;


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
  
  gl_Position = params.mProjView * vec4(pinfo[gl_InstanceIndex].position.xyz, 1) + vec4(pos, 0);
  
  vOut.texCoord = pos.xy;
  vOut.material = params.material;
}

