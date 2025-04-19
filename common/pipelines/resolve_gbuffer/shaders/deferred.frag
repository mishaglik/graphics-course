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

layout(binding = 12) uniform sampler2D diffuse_map;

layout(push_constant) uniform pc_t
{
  vec4 position;
  vec4 color;
  int pbr;
  int  gi;
} params;


const vec2 resolution = vec2(1280, 720);
#include "position.glsl"
#include "shadow.glsl"
#include "pbr.glsl"



vec4 getLight(vec3 lightPos, vec3 pos, vec3 normal, vec3 lightColor, vec3 surfaceColor, vec4 material)
{
  const vec3 lightDir   = normalize(lightPos - pos);
  //const vec3 lightColor = texture(skybox, invview);
  return vec4(pbr_light(surfaceColor, pos, normal, normalize(lightPos), material, lightColor, shadow(pos), world.mView[0]), 1.f);
//  return vec4(surfaceColor, 1) * 0.05;
}


void main(void)
{
  const vec3 surfaceColor = texture(albedo, surf.texCoord).rgb;
  const vec4 normal_wc = texture(normal, surf.texCoord);
  const mat3 iv3 = transpose(inverse(mat3(world.mView[0])));
  vec3 normal = normal_wc.xyz;
  if(length(normal) < 0.5)
    normal = vec3(0, 1, 0);
  const vec3 cNormal = normalize(normal);
  const vec3 wNormal = (world.mView[0] * vec4(cNormal, 0)).xyz;

  const float wc    = texture(wc, surf.texCoord).r;
  const float depthV = texture(depth, surf.texCoord).r;
  const vec3 pos_screen = getScreenPos(depthV, wc);
  const vec3 pos = getCamPos(pos_screen, world.mProj[0]);
  
  const vec4 mat = texture(material, surf.texCoord);
  // Only sunlight. Other are in sphere_deferred;
  const vec3 lightPos = (world.mView[0] * normalize(vec4(params.position.xyz, 0))).xyz;

  const vec3 wPos = getWorldPos(pos, world.mIView[0]);
  
  const vec3 reflection = texture(skybox, (wPos - 2 * wNormal * dot(wNormal, wPos))).rgb;

 
  out_fragColor.rgb =  vec3(dot(cNormal, normalize(lightPos)));
  if (params.pbr != 0) {
    out_fragColor.rgb = light_pbr_shadow_gi(
      pos,
      cNormal,
      surfaceColor,
      mat,
      lightPos,
      vec3(1),
      reflection,
      lambertian(surfaceColor) * texture(diffuse_map, surf.texCoord).rgb
    );
  } else {
    out_fragColor.rgb = surfaceColor * max(0.05, shadow(pos) * dot(normalize(lightPos - pos), normal));
  }
  gl_FragDepth = depthV;
}

