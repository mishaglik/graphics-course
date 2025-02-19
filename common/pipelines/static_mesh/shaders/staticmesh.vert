#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "unpack_attributes.glsl"


layout(location = 0) in vec4 vPosNorm;
layout(location = 1) in vec4 vTexCoordAndTang;
layout(location = 2) in vec4 vNormTexCoord;

layout(push_constant) uniform params_t
{
  mat4 mProjView;
  uint relemIdx;
} params;


layout (location = 0 ) out VS_OUT
{
  vec3 wPos;
  vec4 wNorm;
  vec4 wTangent;
  vec2 texCoord;
  vec2 normTexCoord;
  flat int material;
} vOut;

layout (std140, set = 0, binding = 0) readonly buffer ims_t {
  mat4 mModels[]; 
} ims;

layout (std140, set = 1, binding = 0) readonly buffer matt_t {
  int mMaterialId[]; 
} materials;

//layout(set = 2, binding = 0) uniform sampler2D materialTextures[];

out gl_PerVertex { vec4 gl_Position; };

void main(void)
{

  const vec4 wNorm = vec4(decode_normal(floatBitsToInt(vPosNorm.w)),     0.0f);
  const vec4 wTang = vec4(decode_normal(floatBitsToInt(vTexCoordAndTang.z)), 0.0f);

  vOut.wPos   = (ims.mModels[gl_InstanceIndex] * vec4(vPosNorm.xyz, 1.0f)).xyz;
  mat3 invModel = transpose(inverse(mat3(ims.mModels[gl_InstanceIndex])));
  vOut.wNorm.xyz    = normalize(invModel * wNorm.xyz);
  vOut.wTangent.xyz = normalize(invModel * wTang.xyz);
  vOut.wNorm.w = 0;
  vOut.wTangent.w = 0;
  vOut.texCoord = vTexCoordAndTang.xy;
  vOut.normTexCoord = vNormTexCoord.xy;
  //vOut.material = materials.mMaterialId[nonuniformEXT(gl_DrawID)];
  gl_Position   = params.mProjView * vec4(vOut.wPos, 1.0);
  vOut.material = gl_DrawID;
}
