#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "particleInfo.hpp"
#include "projview.hpp"
#include "material.hpp"

layout (location = 0 ) out VS_OUT
{
  vec2 texCoord;
  float fade;
  flat uint material;
  flat uint drawID;
} vOut;

layout(set = 0, binding = 0) readonly buffer et_t{
  EmitterInfo einfo[N_MAX_EMITTERS];
};

layout(set = 0, binding = 1) readonly buffer pt_t{
  ParticleInfo pinfo[N_MAX_PARTICLES_PER_DRAW];
};

layout (std140, set = 1, binding = 0) readonly buffer mat_t {
  GpuMaterial materials[N_MAX_MATERIALS]; 
};
layout(set = 1, binding = 1) uniform sampler2DArray textures[N_MAX_TEXTURES];
layout(set = 2, binding = 0) uniform WVPM_t {
  WorldViewProjMatrices world;
};
layout(push_constant) uniform pc {
    mat4 mProjView;
    float frameTime;
    float dt;
} params;

const uint wId = 0;
vec2 bezier_pt(vec2 p1, vec2 p2, float t) {
    const vec2 p0 = vec2(0, 0);
    const vec2 p3 = vec2(1, 1);
    return mix( 
        mix(
            mix(p0, p1, t), 
            mix(p1, p2, t), 
            t
        ), 
        mix(
            mix(p1, p2, t), 
            mix(p2, p3, t), 
            t
        ), 
        t
    );
}

float bezier(vec2 p1, vec2 p2, float x) {
  float l = 0, r = 1, m = 0.5;
  vec2 bz;
  for(int i = 0; i < 10; i++) {
    m = (r+l) / 2;
    bz = bezier_pt(p1, p2, m);
    if(bz.x < x) {
      l = m;
    } else {
      r = m;
    }
  }
  return bz.y;
}

void boardParticle() {
  vec3 pos;
  if(gl_VertexIndex ==  0) pos = vec3(-1.f,  1.f,  0.f);  
  if(gl_VertexIndex ==  1) pos = vec3( 1.f,  1.f,  0.f);   
  if(gl_VertexIndex ==  2) pos = vec3(-1.f, -1.f,  0.f); 
  if(gl_VertexIndex ==  3) pos = vec3( 1.f, -1.f,  0.f);  
  vOut.texCoord = 0.5 * pos.xy + 0.5;
  pos.xy *= mix(1, einfo[gl_DrawID].fadeSize_pad.x, vOut.fade) * einfo[gl_DrawID].size;
  gl_Position = world.mProjView[wId] * vec4(pinfo[gl_InstanceIndex].position.xyz, 1) + world.mProj[wId] * vec4(pos, 0);
}

void worldBoardParticle() {
  vec3 pos;
  if(gl_VertexIndex ==  0) pos = vec3(-1.f,  1.f,  0.f);  
  if(gl_VertexIndex ==  1) pos = vec3( 1.f,  1.f,  0.f);   
  if(gl_VertexIndex ==  2) pos = vec3(-1.f, -1.f,  0.f); 
  if(gl_VertexIndex ==  3) pos = vec3( 1.f, -1.f,  0.f);  
  vOut.texCoord = 0.5 * pos.xy + 0.5;
  pos.xy *= mix(1, einfo[gl_DrawID].fadeSize_pad.x, vOut.fade) * einfo[gl_DrawID].size;
  gl_Position = world.mProjView[wId] * vec4(pinfo[gl_InstanceIndex].position.xyz + pos, 1);
}

void screenParticle() {
  vec3 pos;
  if(gl_VertexIndex ==  0) pos = vec3(-1.f,  1.f,  0.f);  
  if(gl_VertexIndex ==  1) pos = vec3( 1.f,  1.f,  0.f);   
  if(gl_VertexIndex ==  2) pos = vec3(-1.f, -1.f,  0.f); 
  if(gl_VertexIndex ==  3) pos = vec3( 1.f, -1.f,  0.f);  
  vOut.texCoord = 0.5 * pos.xy + 0.5;
  pos.xy *= mix(1, einfo[gl_DrawID].fadeSize_pad.x, vOut.fade) * einfo[gl_DrawID].size;
  gl_Position = vec4(pos, 0);
}

void cubeParticle() {
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
  
  vOut.texCoord = 0.5 * pos.xy + 0.5;
  vec3 size = vec3(einfo[gl_DrawID].size, 0);
  size.z = (size.x + size.y) / 2;
  pos *= size;
  pos *= mix(1, einfo[gl_DrawID].fadeSize_pad.x, vOut.fade);
  gl_Position = world.mProjView[wId] * vec4(pinfo[gl_InstanceIndex].position.xyz+pos, 1);
}



void main() {
  uint type = einfo[gl_DrawID].type;
  vOut.fade = pinfo[gl_InstanceIndex].position.w;
  if(type == 1) boardParticle();
  if(type == 2) worldBoardParticle();
  if(type == 3) screenParticle();
  if(type == 4) cubeParticle();
  
  vOut.fade =  bezier(einfo[gl_DrawID].fadeBezier.xy, einfo[gl_DrawID].fadeBezier.yz, vOut.fade);
  
  vOut.material = einfo[gl_DrawID].material;
  vOut.drawID = gl_DrawID;
}

