#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;

layout(location = 0) out vec4 out_fragColor;

layout(binding = 0) uniform sampler2D fog;

void main() {
    out_fragColor = vec4(texture(fog, surf.texCoord).rgb, 0.5);
}
