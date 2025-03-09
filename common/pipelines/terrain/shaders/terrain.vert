#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0 ) out VS_OUT
{
  vec2 texCoord;
} vOut;

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

void main() {
  vec2 xy = vec2(0);
  if (params.subChunk == 0) 
    xy += vec2( 0,      0);
  if (params.subChunk == 1) 
    xy += vec2( 0, params.nChunks);
  if (params.subChunk == 2) 
    xy += vec2( params.nChunks,    0);
  if (params.subChunk == 3) 
    xy += vec2( params.nChunks, params.nChunks);

  xy += vec2(gl_InstanceIndex / params.nChunks, gl_InstanceIndex % params.nChunks);
  
  vec2 dxy, tdxy;
  if (gl_VertexIndex == 0) 
    dxy = vec2( 0,  0);
  if (gl_VertexIndex == 1) 
    dxy = vec2( 0,  1);
  if (gl_VertexIndex == 2) 
    dxy = vec2( 1,  0);
  if (gl_VertexIndex == 3) 
    dxy = vec2( 1,  1);
  tdxy = dxy;
  if((params.corner & 0x1) != 0 && (params.subChunk == 0 || params.subChunk == 1) && (gl_InstanceIndex / params.nChunks == 0)) {
    tdxy.x = min(1, tdxy.x + 0.01);
  }
  if((params.corner & 0x2) != 0 && (params.subChunk == 2 || params.subChunk == 3) && (gl_InstanceIndex / params.nChunks == params.nChunks - 1)) {
    tdxy.x = max(0.0, tdxy.x - 0.01);
  }
  if((params.corner & 0x4) != 0 && (params.subChunk == 0 || params.subChunk == 2) && (gl_InstanceIndex % params.nChunks == 0)) {
    tdxy.y = min(1, tdxy.y + 0.01);
  }
  if((params.corner & 0x8) != 0 && (params.subChunk == 1 || params.subChunk == 3) && (gl_InstanceIndex % params.nChunks == params.nChunks - 1)) {
    tdxy.y = max(0, tdxy.y - 0.01);
  }
  vOut.texCoord = (xy + tdxy + params.base / params.extent) / params.nChunks / 8;
  xy += dxy;
  xy = params.base + xy * params.extent;
  gl_Position = vec4(xy.x, 0, xy.y, 1);
}
