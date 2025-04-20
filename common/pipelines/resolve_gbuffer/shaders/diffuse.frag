#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "projview.hpp"

layout(location = 0) out vec4 out_fragColor;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;

layout(binding = 0) uniform sampler2D albedo;
layout(binding = 1) uniform sampler2D normal;
layout(binding = 2) uniform sampler2D material;
layout(binding = 3) uniform sampler2D wc;
layout(binding = 4) uniform sampler2D depth;

layout(binding = 5) uniform sampler2DArray shadow_albedo  ;
layout(binding = 6) uniform sampler2DArray shadow_normal  ;
layout(binding = 7) uniform sampler2DArray shadow_material;
layout(binding = 8) uniform sampler2DArray shadow_wc      ;
layout(binding = 9) uniform sampler2DArray shadow_depth   ;


layout(binding = 10) uniform samplerCube skybox;
layout(binding = 11) uniform WVPM_t {
  WorldViewProjMatrices world;
};

layout (std140, binding = 12) readonly buffer points_t {
  vec4 samplingPoints[]; 
};



layout(push_constant) uniform pc_t
{
  vec4 position;
  vec4 color;
  int pbr;
  int gi;
} params;


const vec2 resolution = vec2(1280 / 2, 720 / 2);
#include "position.glsl"
#include "shadow.glsl"
#include "pbr.glsl"

vec3 diffuseRSM_I(vec3 cam_pos, vec3 cam_normal, int wId) {

  const vec3 world_pos = getWorldPos(cam_pos, world.mIView[0]);

  const vec4 posLightClipSpace = world.mProjView[wId] * vec4(world_pos, 1.0f);

  const vec3 posLightSpaceNDC = posLightClipSpace.xyz / posLightClipSpace.w;
  
  const vec3 shadowTexCoord = vec3(posLightSpaceNDC.xy*0.5f + vec2(0.5f), wId-1);

  const bool  outOfView = (shadowTexCoord.x < 0.0001f || shadowTexCoord.x > 0.9999f || shadowTexCoord.y < 0.0001f || shadowTexCoord.y > 0.9999f);
  if(outOfView) {
    return vec3(0, 0, 0);
  }

  vec3 diffuse = vec3(0);

  int samples = 0;
  for(int i = 0; i < 400; i++) {
    const vec3 offs = 0.015 * vec3(samplingPoints[i].xy, 0);
    const vec3 texCoord = shadowTexCoord + offs;
    if(texCoord.x > 1 || texCoord.x < 0 || texCoord.y > 1 || texCoord.y < 0 || texture(shadow_depth, texCoord).r > 0.9999) {
        continue;
    }
    samples++;
    vec3 second_world_coord = getWorldPos(
                                getCamPos(
                                  getScreenPos(
                                    texCoord.xy, 
                                    texture(shadow_depth, texCoord).r, 
                                    texture(shadow_wc, texCoord).r), 
                                  world.mProj[wId]), 
                                world.mIView[wId]
                              );

    vec3 second_cam_coord   = (world.mView[0] * vec4(second_world_coord, 1)).xyz;
    vec3 second_cam_normal  = normalize(texture(shadow_normal, texCoord).xyz);
    vec3 between = second_cam_coord - cam_pos;
    vec3 baseColor = texture(shadow_albedo, texCoord).rgb;
    float roughness = texture(shadow_material, texCoord).g;
    float dist = dot(between, between);
    vec3 local_diffuse = lambertian(baseColor) * dot(offs, offs) * max(dot(second_cam_normal, -between), 0) * max(dot(cam_normal, between), 0) / (dist * dist);
    diffuse += local_diffuse;
  }
  diffuse *= 100000;

  if(posLightSpaceNDC.z < textureLod(shadow_depth, vec3(shadowTexCoord), 0).w + 0.001f) {
    const vec3 n = normalize(cam_normal);
    const vec3 lightPos = (world.mView[0] * normalize(vec4(params.position.xyz, 0))).xyz;
    const vec3 l = normalize(lightPos);
    const float ndotl = clamp(dot(n, l), 0, 1); 

    diffuse += vec3(ndotl);
  }

  return diffuse;
}


vec3 diffuseRSM(vec3 cam_pos, vec3 cam_normal) {
  float s = -1.f;
  vec3 diffuse = vec3(0);
  for(int i = textureSize(shadow_depth, 0).z; i > 0; --i) {
    diffuse = max(diffuse, diffuseRSM_I(cam_pos, cam_normal, i));
  }
  return max(diffuse, vec3(0.05));
}

void main(void)
{
    const vec3 baseColor = texture(albedo, surf.texCoord).rgb;
    const vec3 cNormal = normalize(texture(normal, surf.texCoord).xyz);
    
    const float wc    = texture(wc, surf.texCoord).r;
    const float depthV = texture(depth, surf.texCoord).r;
    const vec3 pos_screen = getScreenPos(depthV, wc);
    const vec3 pos = getCamPos(pos_screen, world.mProj[0]);

    const vec3 lightPos = (world.mView[0] * normalize(vec4(params.position.xyz, 0))).xyz;
    
    const vec3 n = normalize(cNormal);
    const vec3 l = normalize(lightPos);
    const float ndotl = clamp(dot(n, l), 0, 1); 


    if(params.gi > 0) {
        out_fragColor.rgb = min(diffuseRSM(pos, cNormal), vec3(1));
    } else {
        out_fragColor.rgb = vec3(max(ndotl, 0.05));
    }
}

