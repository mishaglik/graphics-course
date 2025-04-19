#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "projview.hpp"

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;

layout(location = 0) out vec4 out_fragColor;

layout(binding = 0) uniform sampler2D    wc;
layout(binding = 1) uniform sampler2D depth;
layout(binding = 2) uniform WVPM_t {
  WorldViewProjMatrices world;
};

layout(push_constant) uniform pc_t
{
    vec4 position;
    vec2 shift;
    float horizon;
    float precis;
    float frequency;
    float depth;
    int steps;
} params;

const vec2 resolution = vec2(1280 / 2, 720 / 2);
#include "position.glsl"
vec3 getPos(float depth, float wc) {
  if(depth == 1) {
    return getScreenPos(depth, 1/params.horizon);
  }
  return getScreenPos(depth, wc);
}

float rand(vec2 c){
	return fract(sin(dot(c.xy, vec2(12.7898,78.233)) + cos(dot(c.xy, vec2(-12315.5767, 3524.56)))) * 43718.5453);
}

float noise(vec2 p){
	vec2 ij = floor(p);
	vec2 xy = fract(p);
	xy = 3.*xy*xy-2.*xy*xy*xy;
  float a = rand((ij+vec2(0.,0.)));
	float b = rand((ij+vec2(1.,0.)));
	float c = rand((ij+vec2(0.,1.)));
	float d = rand((ij+vec2(1.,1.)));
	float x1 = mix(a, b, xy.x);
	float x2 = mix(c, d, xy.x);
	return clamp(mix(x1, x2, xy.y), 0., 1.);
}

float shadow(vec3 pos) {
  return 1.f;
}

vec3 box(vec3 pos) {
  return floor(params.precis * pos) / params.precis;
}

float fog(vec3 pos) {
  // return max(0, pos.x / 10);
  float v = noise(params.frequency * pos.xz + params.shift);
  return clamp(params.depth - v, 0, 1) * clamp(exp(-0.1*(pos.y)), 0., 1.);
  // return noise( pos.xz + params.shift) * clamp(exp(-1000*(pos.y)), 0., 1.);
}

const float PI = 3.14159267;
float isothropic(float) {
  return 1 / (4 * PI);
}

float rayleigh(float costh) {
  return 3 * (1 + costh * costh) / (16 * PI);
}

float phase(float costh) {
  return 0.1 * isothropic(costh) + rayleigh(costh);
}

float f(vec3 pos, vec3 dir, vec3 lightPos) {
  pos = box(pos);
  const float costh = dot(dir, normalize(lightPos - pos));
  return phase(costh) * fog(pos) * shadow(pos);
}

float integrate_f(vec3 start, vec3 stop, int steps, vec3 lightPos) {
  const float divn = 1.f / steps;
  const vec3 step = (stop - start) * divn;
  const vec3 dir = normalize(step);

  float res = (f(start, dir, lightPos) + f(stop, dir, lightPos)) / 2;
  vec3 pos = start + step;
  for(int i = 1; i < steps; i++) {
    res += f(pos, dir, lightPos);
    pos += step; 
  }
  res *= length(step);
  return res;
}

void main() {
  const float wc    = texture(wc, surf.texCoord).r;
  const float depthV = texture(depth, surf.texCoord).r;

  const vec3 pos_screen = getPos(depthV, wc);
  const vec3 camPos = getCamWorldPos(world.mIView[0]);
  const vec3 pos = getWorldPos(getCamPos(pos_screen, world.mProj[0]), world.mIView[0]);

  const vec3 lightPos = (vec4(params.position.xyz, 1)).xyz;
  out_fragColor.rgb = vec3(integrate_f(camPos, pos, params.steps, lightPos));
}
