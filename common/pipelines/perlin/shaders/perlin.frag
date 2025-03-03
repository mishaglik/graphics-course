#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out float out_fragColor;
layout(location = 1) out vec4 out_fragNormal;
layout(location = 2) out vec4 out_fragTPxx;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;


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

vec2 hash( vec2 p ) // replace this by something better
{
	p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float unoise( in vec2 p )
{
  const float K1 = 0.366025404; // (sqrt(3)-1)/2;
  const float K2 = 0.211324865; // (3-sqrt(3))/6;

	vec2  i = floor( p + (p.x+p.y)*K1 );
  vec2  a = p - i + (i.x+i.y)*K2;
  float m = step(a.y,a.x); 
  vec2  o = vec2(m,1.0-m);
  vec2  b = a - o + K2;
	vec2  c = a - 1.0 + 2.0*K2;
  vec3  h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
	vec3  n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
  return dot( n, vec3(70.0) );
}

float mainNoise(vec2 p)
{
  //   noise = min(noise, 1.45-noise);
  // noise = (0.4 - abs(noise));
  return pow((1 + snoise(p)) / 2, pushConstant.pow);
}

void main(void)
{  
  
  out_fragColor = 0.;
  const vec2 worldCoord = surf.texCoord * pushConstant.extent + pushConstant.start;
  out_fragColor = (sin(worldCoord.x + worldCoord.y)+1)/2;
  return;
  float frequency = pushConstant.frequency;
  float amplitude = 1;
  float cumAmplitude = 0;
  for(int i = 0; i < pushConstant.octave; ++i) {
    float noise = mainNoise(frequency * worldCoord);
    out_fragColor += amplitude * noise;
    cumAmplitude += amplitude;
    frequency *= 2;
    amplitude /= 2;
  }
  

}