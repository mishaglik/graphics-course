#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out vec4 out_fragColor;
layout(location = 1) out vec4 out_fragNormal;

layout (location = 0) in VS_OUT
{
  vec2 texCoord;
  vec4 normal;
  float height;
} surf;

layout(binding = 0) uniform sampler2D hmap;

float depthToDist(float depth)
{
  return depth / gl_FragCoord.w;
}

void main(void)
{
  out_fragColor = fract(surf.texCoord.x * 64.) < 0.5 ? vec4(1., 0., 0., 0) : vec4(0., 1., 0., 0);
  //out_fragColor = vec4(gl_FragCoord.z, depthToDist(gl_FragCoord.z) / far, 1., 0);

  out_fragNormal = vec4(surf.normal.rgb, gl_FragCoord.w);

}