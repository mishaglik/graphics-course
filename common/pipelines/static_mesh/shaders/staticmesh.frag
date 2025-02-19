#version 460
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shader_draw_parameters : require
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) out vec4 out_fragColor;
layout(location = 1) out vec4 out_fragNormal;
layout(location = 2) out vec4 out_fragMaterial;
layout(location = 3) out float out_fragWc;

layout(location = 0) in VS_OUT
{
  vec3 wPos;
  vec4 wNorm;
  vec4 wTangent;
  vec2 texCoord;
  vec2 normTexCoord;
  flat int material;
} surf;

layout (std140, set = 0, binding = 0) readonly buffer ims_t {
  mat4 mModels[]; 
} ims;

layout (std140, set = 1, binding = 0) readonly buffer matt_t {
  int mMaterialId[]; 
} materials;

layout(set = 2, binding = 0) uniform sampler2D materialTextures[];
layout(set = 3, binding = 0) readonly buffer Mat {
  vec4 params[]; 
} materialParams;

layout(push_constant) uniform params_t
{
  mat4 mProjView;
  uint relemIdx;
} params;


const vec2 resolution = vec2(1280, 720);

vec3 getPos() {
  return vec3(
    (2 * gl_FragCoord.x / resolution.x) - 1,
    (2 * gl_FragCoord.y / resolution.y) - 1,
    gl_FragCoord.z
  ) / gl_FragCoord.w;
}

void main()
{
  const uint material = materials.mMaterialId[nonuniformEXT(uint(surf.material))];
  out_fragColor = texture(materialTextures[nonuniformEXT(3 * material)], surf.texCoord) * materialParams.params[nonuniformEXT(2 * material)];
  vec3 bitangent = normalize(cross(surf.wNorm.xyz, surf.wTangent.xyz));
  vec4 normalMap = texture(materialTextures[nonuniformEXT(3 * material + 1)], surf.texCoord); 
  normalMap = 2 * normalMap - 1;
  out_fragNormal.xyz = normalize(surf.wNorm.xyz * normalMap.b + surf.wTangent.xyz * normalMap.g + bitangent * normalMap.r);
  //out_fragNormal = normalMap;
  out_fragWc = gl_FragCoord.w;
  out_fragMaterial = materialParams.params[nonuniformEXT(2 * material + 1)] * texture(materialTextures[nonuniformEXT(3 * material + 2)], surf.normTexCoord);
}
