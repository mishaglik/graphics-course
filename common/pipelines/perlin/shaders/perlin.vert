#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0 ) out VS_OUT
{
  vec2 texCoord;
} vOut;


layout(push_constant) uniform params
{
  vec2 start;
  vec2 extent;
  float frequency;
  float amplitude;
  float pow;
  uint octave;
  vec2 texStart;
  vec2 texExtent;
} pushConstant;


void main() {
  vec2 xy = vec2(0, 0);
  if (gl_VertexIndex == 0) xy = vec2(0, 0);
  if (gl_VertexIndex == 1) xy = vec2(1, 0);
  if (gl_VertexIndex == 2) xy = vec2(0, 1);
  if (gl_VertexIndex == 3) xy = vec2(1, 1);
  vec2 xy_tex = xy * pushConstant.texExtent + pushConstant.texStart;
  gl_Position = vec4((xy_tex - vec2(0.5, 0.5)) * 2, 0, 1);
  vOut.texCoord = xy;
}
