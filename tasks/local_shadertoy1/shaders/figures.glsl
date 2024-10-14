
#define GenerateSphere(metric)                \
float                                         \
dSphere##metric (vec3 x, vec4 params)         \
{                                             \
    return metric(x - params.xyz) - params.w; \
}

#define GenerateEllips(metric)                         \
float                                                  \
dEllips##metric (in vec3 x, in vec3 pos, in vec3 size) \
{                                                      \
    x = (x-pos) / size;                                \
    return metric(x) - 1.0;                            \
}

float 
dPlane (in vec3 pt, in vec4 params )
{
    return dot(pt, params.xyz) + params.w;
}


//This is not true cylinder in metric space.
#define GenerateCylinder(metric)                            \
float                                                       \
dCylinder##metric( in vec3 pos, in vec3 a, in vec3 b, in float r )  \
{                                                           \
    vec3 pt    = pos;                                       \
    vec3 ba    = b  - a;                                    \
    vec3 pa    = pt - a;                                    \
    float baba = dot(ba,ba);                                \
    float paba = dot(pa,ba);                                \
    float x    = metric(pa*baba-ba*paba) - r*baba;          \
    float y    = abs(paba-baba*0.5)-baba*0.5;               \
    float x2 = x*x;                                         \
    float y2 = y*y*baba;                                    \
    float d = (max(x,y)<0.0) ?                              \
                -min(x2,y2)  :                              \
                (((x>0.0) ? x2 : 0.0) +                     \
                 ((y>0.0) ? y2 : 0.0) );                    \
    return sign(d)*sqrt(abs(d))/baba;                       \
}


float 
dConeCapped ( in vec3 pos, in vec3 st, in float h, in float r1, in float r2 )
{
    vec3 pt = pos - st;
    vec2 q  = vec2 ( length ( pt.xz ), pt.y );
    vec2 k1 = vec2 ( r2, h );
    vec2 k2 = vec2 ( r2 - r1, 2.0*h );
    vec2 ca = vec2 ( q.x-min(q.x,(q.y<0.0)?r1:r2), abs(q.y)-h);
    vec2 cb = q - k1 + k2*clamp( dot(k1-q,k2)/dot(k2, k2), 0.0, 1.0 );
    float s = (cb.x<0.0 && ca.y<0.0) ? -1.0 : 1.0;
    return s*sqrt( min(dot(ca, ca),dot(cb, cb)) );
}

float
dAABox ( in vec3 pos, in vec3 size )
{
    vec3 pt = pos - size;

    return length ( max ( pt, 0.0 ) ) + min ( max ( pt.x, max ( pt.y, pt.z ) ), 0.0 );
}

float 
dAABox(in vec3 pos, in vec3 p1, in vec3 p2)
{
    return SubtractFigures(dAABox(pos, p1), dAABox(pos, p2));
}


#define GenerateMetric(n, name) \
float                           \
length##name (vec2 pt)          \
{                               \
    return pow(                 \
            pow(abs(pt.x), n) + \
            pow(abs(pt.y), n) , \
        1./n);                  \
}                               \
float                           \
length##name (vec3 pt)          \
{                               \
    return pow(                 \
            pow(abs(pt.x), n) + \
            pow(abs(pt.y), n) + \
            pow(abs(pt.z), n) , \
        1./n);                  \
}                               \

float
lengthInf (vec2 pt)
{
    return max(
        abs(pt.x) ,
        abs(pt.y) );
}
float
lengthInf (vec3 pt)
{
    return max(max(
        abs(pt.x) ,
        abs(pt.y) ),
        abs(pt.z) );
}

GenerateMetric(8.0, 8)

GenerateSphere(length)
#define dSphere dSpherelength
GenerateEllips(lengthInf)
#define dBox dEllipslengthInf

GenerateCylinder(length)
#define dCylinder dCylinderlength
GenerateCylinder(length8)
