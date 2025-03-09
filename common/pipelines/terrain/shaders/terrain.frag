#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out vec4 out_fragColor;
layout(location = 1) out vec4 out_fragNormal;
layout(location = 2) out vec4 out_fragMaterial;
layout(location = 3) out float out_fragWc;

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
} params;

layout(set=0, binding = 0) uniform sampler2D hmap;
layout(set=0, binding = 1) uniform sampler2D normalMap;
layout(set=0, binding = 2) uniform sampler2D tprrMap;

//TODO: Bindless
layout(set=1, binding = 0) uniform sampler2D grasTexture;
layout(set=1, binding = 1) uniform sampler2D sandTexture;
layout(set=1, binding = 2) uniform sampler2D snowTexture;
layout(set=1, binding = 3) uniform sampler2D rockTexture;
layout(set=1, binding = 4) uniform sampler2D roadTexture;

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

vec3 heightColor(float height)
{
  height = clamp(texture(hmap, surf.texCoord).r, 0, 1);
  vec4 tpxx   = texture(tprrMap, surf.texCoord);
  
  if(height * params.maxHeight < params.seaLevel)
  {
    return vec3(0.01, 0.01, 0.71) + vec3(0, tpxx.r / 4, -tpxx.r / 4);
  }
  
  if(height > 0.8)
  {
    return texture(snowTexture, 0.05 * surf.worldCoord).rgb;
  }
  
  // vec3 color = vec3(pow(tpxx.r, 2), 0.9, pow(1-tpxx.r, 2));
  vec3 color = texture(grasTexture, 10 * surf.texCoord).rgb;
  float rockiness = clamp(((height - 0.6) / 0.1), 0, 1);
  color = mix(color, texture(rockTexture, 0.05*surf.worldCoord).rgb, rockiness);
  color = mix(color, texture(roadTexture, surf.worldCoord).rgb, sin(3.1415 / 2 * tpxx.g));
  float sandiness = clamp((params.seaLevel / params.maxHeight - height + 0.01) / 0.01, 0, 1);
  sandiness = pow(sandiness, 2);
  color = mix(color, texture(sandTexture, surf.worldCoord).rgb, sandiness);
  return color;
}

vec4 heightMaterial(float height)
{
  if(height > 25)
    return vec4(0, 0.5, 0.0, 1);
  return vec4(0, 0.8, 0.0, 1);
}

void main(void)
{
  //out_fragColor = fract(surf.texCoord.x * 64.) < 0.5 ? vec4(1., 0., 0., 0) : vec4(0., 1., 0., 0);
  //out_fragColor = vec4(gl_FragCoord.z, depthToDist(gl_FragCoord.z) / far, 1., 0);
  
  // out_fragColor.rgb = heightColor(surf.height);
  out_fragColor.rgb = heightColor(texture(tprrMap, surf.texCoord).r);
  // out_fragColor.rgb = surf.normal.rgb;
  // out_fragColor.g = 0;
  out_fragNormal = vec4(surf.normal.rgb, 0);
  out_fragWc = gl_FragCoord.w;
  out_fragMaterial  = heightMaterial(surf.height);
}