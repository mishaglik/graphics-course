#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

#include "sdf.glsl"
#include "figures.glsl"

layout(location = 0) out vec4 color;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;

layout(push_constant) uniform params
{
  float iTime;
} pushConstant;

layout(binding = 0, set = 0) uniform data
{
    int _;
};

const vec3  eye      = vec3  (0, 0, 10);
const float period   = 22.;
const vec3  light    = vec3  ( 5.3,  -1.6 + 6.0 + 0.32, 0.5 * period - 0. * period);
const int   maxSteps = 1000;
const float eps      = 1e-4;
const float renderDistance = 1e4;


float dRazmetka(in vec3 p, in vec3 pos, in vec3 size)
{
    p = p - pos;
    p.z = mod(p.z, size.z * 2.);
    p = p / size;
    
    return length8(p) - 1.;
    
}

vec4 lamp(in vec3 p, in vec3 pos)
{
    vec3 height = vec3(0.0, 6.0, 0.0);
    float topR = 0.1;
    
    return vec4(1., 1., 1., UnionFigures( 
            smoothUnion(
                dCylinder(p, pos, pos + 0.3 * height, topR),
                dCylinder(p, pos, pos +       height, topR),
                0.3),
            UnionFigures(
                UnionFigures(
                    dCylinderlength8(p, pos + height + vec3(  0.0,  0.0,  topR), pos + height + vec3(         0.0, 0.5,  topR + 0.3), 0.01),
                    dCylinderlength8(p, pos + height + vec3(  0.0,  0.0, -topR), pos + height + vec3(         0.0, 0.5, -topR - 0.3), 0.01),
                    dCylinderlength8(p, pos + height + vec3(  topR, 0.0,   0.0), pos + height + vec3(  topR + 0.3, 0.5,         0.0), 0.01),
                    dCylinderlength8(p, pos + height + vec3( -topR, 0.0,   0.0), pos + height + vec3( -topR - 0.3, 0.5,         0.0), 0.01)
                ),
                SubtractFigures(
                    dConeCapped(p, pos + height + vec3(0, 0.699, 0), 0.2, topR + 0.28, 0.02),
                    dConeCapped(p, pos + height + vec3(0, 0.700, 0), 0.2, topR + 0.30, 0.10)
                )
            )
        ));
}


vec4 road(in vec3 p)
{
    return UnionFigures(
        vec4(0.1, 0.1, 0.1, dPlane    (p, vec4 (0, 1, 0, 1.8))),
        vec4(0.8, 0.8, 0.0, dRazmetka (p, vec3(2.5, -1.8, 1), vec3(0.2, 0.1, 2))),
        vec4(0.3, 0.3, 0.3, UnionFigures(
            SubtractFigures(dAABox (p, vec3(-100, 0.1, 50.1)), dAABox(p, vec3(0,   -1.6, 50))),
            SubtractFigures(dAABox (p, vec3(   5, 0.1, 50.1)), dAABox(p, vec3(100, -1.6, 50)))
            ))
        );
    
}

vec4 housing(in vec3 p)
{
    vec3 pr = p; 
    pr.z = mod(pr.z, 3.1); 
    pr.y -= 1.;
    if (pr.y > 0.) {
        pr.y = mod(pr.y, 4.1);
    }
    return SubtractFigures(
        vec4(0.0, 0.0, 0.0, dBox(pr, vec3(-1., 1, 0.7), vec3(0.1, 1.4, 0.7))),
        vec4(0.4, 0.2, 0.0, dAABox(p, vec3(-1, 30, 50)))
        );
}

vec4 sdf ( in vec3 p, in mat3 m)
{
   vec3 q = m * p;
   vec3 qr = q;
   qr.z = mod(qr.z, period);
   return UnionFigures(
        road(q), 
        lamp(qr, vec3(5.3, -1.6, 0.5 * period)),
        housing(q)
    );
}


vec3 trace ( in vec3 from, in vec3 dir, out bool hit, in mat3 m, out vec4 surf)
{
        vec3    p         = from;
        float   totalDist = 0.0;
        float  prec = 0.99;
        hit = false;
        
        for ( int steps = 0; steps < maxSteps; steps++ )
        {
                surf = sdf ( p, m);
                float dist = prec * surf.w;
        
                if ( dist < eps )
                {
                    hit = true;
                    break;
                }
    
                totalDist += dist;

                if ( totalDist > renderDistance )
                        break;

                p += dist * dir;
        }

        return p;
}

vec3 generateNormal ( vec3 z, float d, in mat3 m )
{
    float e   = max (d * 0.5, eps );
    vec4 sf;
    float dx1 = sdf(z + vec3(e, 0, 0), m).w;
    float dx2 = sdf(z - vec3(e, 0, 0), m).w;
    float dy1 = sdf(z + vec3(0, e, 0), m).w;
    float dy2 = sdf(z - vec3(0, e, 0), m).w;
    float dz1 = sdf(z + vec3(0, 0, e), m).w;
    float dz2 = sdf(z - vec3(0, 0, e), m).w;
    
    return normalize ( vec3 ( dx1 - dx2, dy1 - dy2, dz1 - dz2 ) );
}

float ambientOcclusion ( in vec3 pos, in vec3 normal, in mat3 m)
{
    float occ = 0.0;
    float sca = 1.0;
    vec4 sf;
    for ( int i = 0; i < 5; i++ )
    {
        float h = 0.01 + 0.12*float(i)/4.0;
        float d = sdf ( pos + h*normal, m).w;

        occ += (h-d)*sca;
        sca *= 0.95;

        if ( occ > 0.35 ) 
            break;
    }

    return clamp ( 1.0 - 3.0*occ, 0.0, 1.0 ) * (0.5+0.5*normal.y);
}

vec4 getLight(vec3 p, vec3 src, mat3 m, vec3 n)
{
    bool hit;
    vec3  l  = normalize( src - p );
    vec4 sf;
    vec3 s = trace(src, -l, hit, m, sf);
    vec4 shade = vec4(1., 1., 1., 1.);
    
    if(length(s - p) > 0.4)
    {
        shade = 0.33 * sf;
    }
    
    
    vec3  eyer = eye;
    eyer.z = mod(eyer.z, period) - 7.;
    vec3  v  = normalize        ( eyer - p );
    float nl = max ( 0.0, dot ( n, l ) );
    vec3  h  = normalize ( l + v );
    float hn = max ( 0.0, dot ( h, n ) );
    float sp = pow ( hn, 150.0 );

    return shade * (0.7*ambientOcclusion(p, n, m)*vec4 ( nl ) + 0.5 * sp * vec4 ( 1, 1, 1, 1 ));
    
}

int hash( int x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}

const vec2 resultImageSize = vec2(1280, 720);

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    bool hit;
    vec3 meye =  eye;
    mat3 m = mat3(vec3(1,0,0), vec3(0,1,0), vec3(0,0,1));
    
    vec2 scale = 9.0 * resultImageSize.xy / max ( resultImageSize.x, resultImageSize.y ) ;
    vec2 uv    = scale * ( fragCoord / resultImageSize.xy - vec2 ( 0.5 ) );
    vec3 dir   = normalize ( vec3 ( uv, 0 ) - meye );
    vec4 color = vec4 ( 0, 0, 0, 1 );
    vec4 surf  = vec4 ( 1, 1, 1, 1 );
    vec3 p     = trace ( meye, dir, hit, m, surf);
    float iTime = pushConstant.iTime;

        if ( hit )
        {       
                vec3 pr = p;
                pr.z = mod(pr.z, period);
                
                color = surf * getLight(pr, light, m, generateNormal( p, 1e-6, m )) ;//+ getLight(pr, light + vec3(0, 0, 15), m);
                float id =  -floor(p.z / period);
                if (hash(int(iTime)) % 100 == int(id)) {
                    color = color * pow(min(1., abs(sin(30. * iTime) + cos(2. * iTime))), 100.);
                }
                
        
        if (dot(color, vec4(1,1,1,0)) < 0.1) {
            color =  0.1 * ambientOcclusion(p, generateNormal( p, 1e-5, m ), m) * surf;
        }
        } 

    // Output to screen
    fragColor = color;
}

void main()
{
  // comptatibility with previous
  ivec2 uv = ivec2(vec2(surf.texCoord.x, 1. - surf.texCoord.y) * vec2(resultImageSize));
  mainImage(color, uv);
  //color = vec4(surf.texCoord, 0, 1);
}
