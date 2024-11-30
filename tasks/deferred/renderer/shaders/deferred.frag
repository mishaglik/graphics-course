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

layout(push_constant) uniform pc_t
{
    mat4 mProjView;
} params;

const vec2 resolution = vec2(1280, 720);

vec3 getPos(float depth, float wc) {
  return vec3(
    (2 * gl_FragCoord.x / resolution.x) - 1,
    (2 * gl_FragCoord.y / resolution.y) - 1,
    depth
  ) / wc;
}

vec4 getLight(vec3 lightPos, vec3 pos, vec3 normal)
{
  const vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
  const vec3 lightDir   = normalize(lightPos - pos);
  const vec3 diffuse = max(dot(normal, lightDir), 0.0f) * lightColor;
  const float ambient = 0.05;
  return vec4( (diffuse + ambient), 1.f);
}

void main(void)
{
  const vec3 wLightPos = (params.mProjView * vec4(0, 2, 0, 1)).rgb;
  const vec3 surfaceColor = texture(albedo, surf.texCoord).rgb;


  const vec4 normal_wc = texture(normal, surf.texCoord);
  const mat3 ipv3 = transpose(inverse(mat3(params.mProjView)));
  const vec3 normal = normalize(ipv3 * normal_wc.xyz);
  const float wc    = normal_wc.w;
  const float depthV = texture(depth, surf.texCoord).w;
  const vec3 pos = getPos(depthV, wc);

  out_fragColor = getLight(wLightPos, pos, normal);
  out_fragColor.rgb *= surfaceColor;

  gl_FragDepth = depthV;
}

