#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out vec4 out_fragColor;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;

layout(binding = 0) uniform sampler2D albedo;
layout(binding = 1) uniform sampler2D normal;
layout(binding = 2) uniform sampler2D  depth;
layout(std140, binding = 3) readonly buffer lp_t
{
  vec4 position[];
} lightPos;

layout(std140, binding = 4) readonly buffer lc_t
{
  vec4 color[];
} lightCol;

layout(push_constant) uniform pc_t
{
    mat4 mProjView;
    int nLights;
} params;

const vec2 resolution = vec2(1280, 720);

vec3 getPos(float depth, float wc) {
  return vec3(
    (2 * gl_FragCoord.x / resolution.x) - 1,
    (2 * gl_FragCoord.y / resolution.y) - 1,
    depth
  ) / wc;
}

vec4 getLight(vec3 lightPos, vec3 pos, vec3 normal, vec3 lightColor)
{
  const vec3 lightDir   = normalize(lightPos - pos);
  const vec3 diffuse = max(dot(normal, lightDir), 0.0f) * lightColor;
  const float ambient = 0.05;
  return vec4( (diffuse + ambient), 1.f);
}

void main(void)
{
  const vec3 surfaceColor = texture(albedo, surf.texCoord).rgb;


  const vec4 normal_wc = texture(normal, surf.texCoord);
  const mat3 ipv3 = transpose(inverse(mat3(params.mProjView)));
  const vec3 normal = normalize(ipv3 * normal_wc.xyz);
  const float wc    = normal_wc.w;
  const float depthV = texture(depth, surf.texCoord).w;
  const vec3 pos = getPos(depthV, wc);
  
  //first light is sun
  out_fragColor = getLight((params.mProjView * vec4(lightPos.position[0].xyz, 1)).xyz, pos, normal, lightCol.color[0].rgb);
  
  for(int i = 1; i < params.nLights; ++i) {
    vec4 lightPosRange = lightPos.position[i];
    const vec3 wLightPos = (params.mProjView * vec4(lightPosRange.xyz, 1)).rgb;
  
    if(length(transpose(ipv3) * (wLightPos - pos)) < lightPosRange.w) {
      out_fragColor += getLight(wLightPos.xyz, pos, normal, lightCol.color[i].rgb);
    }
  }
  out_fragColor.rgb *= surfaceColor;

  gl_FragDepth = depthV;
}

