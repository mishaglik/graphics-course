#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out float out_fragColor;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;


layout(binding = 0) uniform sampler2D previous;

layout(push_constant) uniform params
{
  vec2 start;
  vec2 extent;
  float frequency;
  float amplitude;
  float pow;
  uint octave;
} pushConstant;


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

// Add octave to work as seed for permutator
vec3 permute(vec3 x) { return mod(((x*34.0)+1.0 + pushConstant.octave)*x, 289.0); }

// Simplex 2D noise.
// Code from internet
float snoise(vec2 v){
  const vec4 C = vec4(0.211324865405187, 0.366025403784439,
           -0.577350269189626, 0.024390243902439);
  vec2 i  = floor(v + dot(v, C.yy) );
  vec2 x0 = v -   i + dot(i, C.xx);
  vec2 i1;
  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;
  i = mod(i, 289.0);
  vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
  + i.x + vec3(0.0, i1.x, 1.0 ));
  vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy),
    dot(x12.zw,x12.zw)), 0.0);
  m = m*m ;
  m = m*m ;
  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 ox = floor(x + 0.5);
  vec3 a0 = x - ox;
  m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
  vec3 g;
  g.x  = a0.x  * x0.x  + h.x  * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;
  return 130.0 * dot(m, g);
}

void main(void)
{  
  out_fragColor = 0.;
  if (pushConstant.octave != 0) {
      out_fragColor += textureLod(previous, surf.texCoord, 0).x;
  }
  const vec2 worldCoord = surf.texCoord * pushConstant.extent + pushConstant.start;
  float noise = snoise(pushConstant.frequency * worldCoord);
//   noise = min(noise, 1.45-noise);
  // noise = (0.4 - abs(noise));
  out_fragColor += pushConstant.amplitude * pow((1 + noise) / 2, pushConstant.pow);
  //out_fragColor += pushConstant.amplitude * snoise(pushConstant.frequency * surf.texCoord);
}