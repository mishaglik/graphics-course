#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout (location = 0 ) in VS_OUT
{
  vec3 wPos;
  vec3 wColor;
  vec4 wNorm;
} vIn;

layout(location = 0) out vec4 out_fragColor;
layout(location = 1) out vec4 out_fragNormal;
layout(location = 2) out vec4 out_fragMaterial;
layout(location = 3) out float out_fragWc;

layout(push_constant) uniform pc {
    mat4 mInvPv;
    uint wId;
} params;

void main() {
  out_fragColor = vec4(vIn.wColor, 1);
    
  out_fragNormal = vIn.wNorm;
  out_fragWc = gl_FragCoord.w;
  out_fragMaterial = vec4(0);
}
