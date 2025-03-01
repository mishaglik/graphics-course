#version 450 core

layout (quads, fractional_odd_spacing, ccw) in;

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
} params;


layout(binding = 0) uniform sampler2D heightMap;

layout(location = 0) in vec2 TextureCoord[];

layout (location = 0) out TSE_OUT
{
  vec2 texCoord;
  vec4 normal;
  float height;
} surf;

void main()
{
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;


    vec2 t00 = TextureCoord[0];
    vec2 t01 = TextureCoord[1];
    vec2 t10 = TextureCoord[2];
    vec2 t11 = TextureCoord[3];
    

    vec2 t  = (t01 - t00) * u + (t10 - t00) * v + t00;

    vec4 p00 = gl_in[0].gl_Position;
    vec4 p01 = gl_in[1].gl_Position;
    vec4 p10 = gl_in[2].gl_Position;
    vec4 p11 = gl_in[3].gl_Position;
    
    float tstep = (t01.y - t00.y) / params.degree; 
    float pstep = (p01.z - p00.z) / params.degree; 
    
    vec4 p  = (p01 - p00) * u + (p10 - p00) * v + p00;

    float h   = texture(heightMap, t).r * params.maxHeight;
    h = max(h, params.seaLevel);
    // h=14;

    float hn0 = texture(heightMap, t + vec2(tstep,    0)).r * params.maxHeight;
    float hn1 = texture(heightMap, t + vec2(   0, tstep)).r * params.maxHeight;
    hn0 = max(hn0, params.seaLevel);
    hn1 = max(hn1, params.seaLevel);
    // hn0=14;
    // hn1=14;

    vec4 pn0 = p + vec4(pstep, 0, 0, 0);
    vec4 pn1 = p + vec4(0, 0, pstep, 0);

    vec4 vertical = vec4(0, 1, 0, 0);

    p   += vertical * h;
    pn0 += vertical * hn0;
    pn1 += vertical * hn1;

    vec4 normal = vec4(normalize(cross((pn0 - p).xyz, (pn1-p).xyz)), 0);

    surf.texCoord = t;
    surf.height   = texture(heightMap, t).r;
    surf.normal = -normal;

    gl_Position = params.mProjView * vec4(p.xyz, 1);
}