#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0 ) out VS_OUT
{
  vec3 texCoord;
} vOut;

layout(push_constant) uniform pc {
    mat4 mProjView;
} params;

void main() {
  vec3 pos = vec3(0);
  if (gl_VertexIndex == 0) 
    pos = vec3(-1.0f,  1.0f, -1.0f);
  if (gl_VertexIndex == 1)
    pos = vec3(-1.0f, -1.0f, -1.0f);
  if (gl_VertexIndex == 2)
    pos = vec3( 1.0f, -1.0f, -1.0f);
  if (gl_VertexIndex == 3)
    pos = vec3( 1.0f, -1.0f, -1.0f);
  if (gl_VertexIndex == 4)
    pos = vec3( 1.0f,  1.0f, -1.0f);
  if (gl_VertexIndex == 5)
    pos = vec3(-1.0f,  1.0f, -1.0f);
  
  if(gl_InstanceIndex == 0) {
    pos = pos.xyz;
  }

  if(gl_InstanceIndex == 1) {
    pos = pos.zxy;
  }

  if(gl_InstanceIndex == 2) {
    pos = pos.zxy;
    pos.x *= -1;
  }

  if(gl_InstanceIndex == 3) {
    pos.yz *= -1;
  }

  if(gl_InstanceIndex == 4) {
    pos = pos.yzx;
    pos.xy *= -1;
  }

  if(gl_InstanceIndex == 5) {
    pos = pos.xzy;
    pos.z *= -1;
  }
  gl_Position = (params.mProjView * vec4(20 * pos, 1));
  vOut.texCoord = pos;
}
