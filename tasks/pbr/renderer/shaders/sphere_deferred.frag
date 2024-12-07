#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out vec4 out_fragColor;

layout (location = 0 ) in VS_OUT
{
  vec3 lightDir;
  vec4 lightSrc;
} surf;

layout(binding = 0) uniform sampler2D albedo;
layout(binding = 1) uniform sampler2D normal;
layout(binding = 2) uniform sampler2D depth;

layout(push_constant) uniform pc_t
{
    mat4 mProjView;
    vec4 pos;
    vec4 color;
    float degree;
} params;

const vec2 resolution = vec2(1280, 720);

vec3 getPos(float depth, float wc) {
  return vec3(
    (2 * gl_FragCoord.x / resolution.x) - 1,
    (2 * gl_FragCoord.y / resolution.y) - 1,
    depth
  ) / wc;
}

vec4 getLight(vec3 lightDir, vec3 normal, vec3 lightColor)
{
  const vec3 diffuse = max(dot(normal, normalize(-lightDir)), 0.0f) * lightColor;
  return vec4(diffuse, 0);
}

void main(void)
{
  out_fragColor = vec4(0, 0, 0, 0);
  vec2 texCoord = gl_FragCoord.xy / resolution;
  const vec3 surfaceColor = texture(albedo, texCoord).rgb;

  const vec4 normal_wc = texture(normal, texCoord);
  const mat3 ipv3 = transpose(inverse(mat3(params.mProjView)));
  const vec3 normal = normalize(ipv3 * normal_wc.xyz);
  const float wc    = normal_wc.w; 
  
  const float depth = texture(depth, texCoord).w;
  const vec3 lightDir = getPos(depth, wc) - surf.lightSrc.xyz;
  const float dist = length(lightDir);
  if (dist - surf.lightSrc.w < 0.00) {
    out_fragColor.rgb = getLight(surf.lightDir, normal, params.color.rgb).rgb * surfaceColor;
    out_fragColor.a = max(sin(3.14 * (1 - dist / surf.lightSrc.w)), 0);
  }
}

