#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require


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
layout(binding = 5) uniform samplerCube skybox;

layout(binding = 6) uniform sampler2D shadowMap;

layout(push_constant) uniform pc_t
{
    mat4 mProj;
    mat4 mView;
    mat4 lightMatrix;
    vec4 position;
    vec4 color;
    int pbr;
} params;

#include "pbr.glsl"

const vec2 resolution = vec2(1280, 720);

vec3 getPos(float depth, float wc) {
  return vec3(
    (2 * gl_FragCoord.x / resolution.x) - 1,
    (2 * gl_FragCoord.y / resolution.y) - 1,
    depth
  ) / wc;
}

float shadow(vec3 pos) {
  pos = inverse(mat3(params.mView)) * (pos - (params.mView * vec4(0,0,0,1)).xyz);
  const vec4 posLightClipSpace = params.lightMatrix*vec4(pos, 1.0f);

  const vec3 posLightSpaceNDC = posLightClipSpace.xyz/posLightClipSpace.w;
  
  const vec2 shadowTexCoord = posLightSpaceNDC.xy*0.5f + vec2(0.5f, 0.5f);

  const bool  outOfView = (shadowTexCoord.x < 0.0001f || shadowTexCoord.x > 0.9999f || shadowTexCoord.y < 0.0091f || shadowTexCoord.y > 0.9999f);
  // if(outOfView)
  //   out_fragColor.g = 1;
  return ((posLightSpaceNDC.z < textureLod(shadowMap, shadowTexCoord, 0).x + 0.001f) || outOfView) ? 1.0f : 0.0f;
}


vec4 getLight(vec3 lightPos, vec3 pos, vec3 normal, vec3 lightColor, vec3 surfaceColor, vec4 material)
{
  const vec3 lightDir   = normalize(lightPos - pos);
  //const vec3 lightColor = texture(skybox, invview);
  return vec4(pbr_light(surfaceColor, pos, normal, normalize(lightPos), material, lightColor, shadow(pos)), 1.f);
//  return vec4(surfaceColor, 1) * 0.05;
}


void main(void)
{
  const vec3 surfaceColor = texture(albedo, surf.texCoord).rgb;
  const vec4 normal_wc = texture(normal, surf.texCoord);
  const mat3 iv3 = transpose(inverse(mat3(params.mView)));
  vec3 normal = normal_wc.xyz;
  if(length(normal) < 0.5)
    normal = vec3(0, 1, 0);
  const vec3 absNormal = normalize(normal);
  normal = normalize(iv3 * normal);
  const float wc    = texture(wc, surf.texCoord).r;
  const float depthV = texture(depth, surf.texCoord).r;
  const vec3 pos_screen = getPos(depthV, wc);
  const vec3 pos = inverse(mat3(params.mProj)) * (pos_screen - vec3(0, 0, params.mProj[3][2]));
  
  const vec4 mat = texture(material, surf.texCoord);
  // Only sunlight. Other are in sphere_deferred;
  const vec3 lightPos = (params.mView * vec4(params.position.xyz, 1)).xyz;
  const vec3 absPos = normalize(inverse(mat3(params.mView)) * pos);
  const vec3 reflection = texture(skybox, (absPos - 2 * absNormal * dot(absNormal, absPos))).rgb;
  //const vec3 reflection = texture(skybox, -absPos).rgb;
  if (params.pbr != 0) {
    out_fragColor = getLight(lightPos, pos, normal, reflection, surfaceColor, mat);
  } else {
    out_fragColor.rgb = surfaceColor * max(0.05, shadow(pos) * dot(normalize(lightPos - pos), normal));
  }
  gl_FragDepth = depthV;
}

