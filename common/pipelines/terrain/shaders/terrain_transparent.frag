#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out vec4 out_fragColor;


layout (location = 0) in VS_OUT
{
  vec2 texCoord;
  vec2 worldCoord;
  vec4 normal;
  float height;
} surf;

layout(push_constant) uniform params_t
{
  vec2 base; 
  vec2 extent;
  mat4 mProjView;
  vec3 camPos;
  int degree;
  float seaLevel;
  float maxHeight;
  uint nChunks;
  uint subChunk;
  uint corner;
  float time;
} params;

layout(set=0, binding = 0) uniform sampler2D hmap;
layout(set=0, binding = 1) uniform sampler2D normalMap;
layout(set=0, binding = 2) uniform sampler2D tprrMap;
layout(set=0, binding = 3) uniform samplerCube skybox;

//TODO: Bindless
layout(set=1, binding = 0) uniform sampler2D grasTexture;
layout(set=1, binding = 1) uniform sampler2D sandTexture;
layout(set=1, binding = 2) uniform sampler2D snowTexture;
layout(set=1, binding = 3) uniform sampler2D rockTexture;
layout(set=1, binding = 4) uniform sampler2D roadTexture;
layout(set=1, binding = 5) uniform sampler2D gravelTexture;

float depthToDist(float depth)
{
  return depth / gl_FragCoord.w;
}

vec3 hue(float x) {
  return vec3(
    clamp(1-abs(2-3*x), 0, 1), 
    clamp(1-abs(1-3*x), 0, 1), 
    clamp(1-abs(0-3*x), 0, 1)
    );
  
}

#include "pbr.glsl"


vec4 getLight(vec3 lightPos, vec3 pos, vec3 normal, vec3 lightColor, vec3 surfaceColor, vec4 material)
{
  const vec3 lightDir   = normalize(lightPos - pos);
  const mat4 mView = mat4(vec4(1, 0, 0, 0), vec4(0, 1, 0, 0), vec4(0, 0, 1, 0), vec4(0, 0, 0, 1));
  return vec4(pbr_light(surfaceColor, pos, normal, normalize(lightPos), material, lightColor, 1.f, mView), 1.f);
}

vec3 pbrWater(vec3 surfaceColor)
{
  vec3 normal = surf.normal.xyz;
  if(length(normal) < 0.5)
    normal = vec3(0, 1, 0);
  const vec3 absNormal = normalize(normal);

  normal = normalize(normal);
  
  const vec3 pos = vec3(surf.worldCoord.x, surf.height, surf.worldCoord.y);
  
  const vec4 mat = vec4(0.4);
  
  const vec3 lightPos = (vec4(-150, 100, -200, 0)).xyz;

  const vec3 reflection = texture(skybox, (pos - 2 * normal * dot(normal, pos))).rgb;

  return getLight(lightPos, pos, normal, reflection, surfaceColor, mat).rgb;
}

void main(void)
{
  
  vec4 tpxx   = texture(tprrMap, surf.texCoord);
  vec3 color = vec3(0.01, 0.01, 0.71) + vec3(0, tpxx.r / 4, -tpxx.r / 4);
  // color = color * max(0.1, dot(normalize(surf.normal.rgb), vec3(0, 1, 0)));
  color = pbrWater(color);
  out_fragColor = vec4(color, 0.99);
  
}