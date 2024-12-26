#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "pbr.glsl"

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

layout(push_constant) uniform pc_t
{
    mat4 mProjView;
    vec4 position;
    vec4 color;
} params;

const vec2 resolution = vec2(1280, 720);

vec3 getPos(float depth, float wc) {
  return vec3(
    (2 * gl_FragCoord.x / resolution.x) - 1,
    (2 * gl_FragCoord.y / resolution.y) - 1,
    depth
  ) / wc;
}

vec4 getLight(vec3 lightPos, vec3 pos, vec3 normal, vec3 lightColor, vec3 surfaceColor, vec4 material)
{
  const vec3 lightDir   = normalize(lightPos - pos);
  return vec4(pbr_light(surfaceColor, pos, normal, normalize(lightPos), material), 1.f);
//  return vec4(surfaceColor, 1) * 0.05;
}

void main(void)
{
  const vec3 surfaceColor = texture(albedo, surf.texCoord).rgb;

  const vec4 normal_wc = texture(normal, surf.texCoord);
  const mat3 ipv3 = transpose(inverse(mat3(params.mProjView)));
  vec3 normal = normalize(ipv3 * normal_wc.xyz);
  if(length(normal) < 0.5)
    normal = vec3(0, 1, 0);
  const float wc    = texture(wc, surf.texCoord).r;
  const float depthV = texture(depth, surf.texCoord).r;
  const vec3 pos = getPos(depthV, wc);
  const vec4 mat = texture(material, surf.texCoord);
  // Only sunlight. Other are in sphere_deferred;
  const vec3 lightPos = (params.mProjView * vec4(-150, 100, -200, 1)).xyz;
  
  out_fragColor = getLight(lightPos, pos, normal, params.color.rgb, surfaceColor, mat);
}

