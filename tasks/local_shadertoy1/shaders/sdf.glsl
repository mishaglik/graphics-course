float
UnionFigures(float d1)
{
    return d1;
}

float 
UnionFigures(float d1, float d2) 
{
    return min(d1, d2);
}

float 
UnionFigures(float d1, float d2, float d3) 
{
    return min(d1, min(d2, d3));
}

float 
UnionFigures(float d1, float d2, float d3, float d4)
{
    return min(min(d1, d2), min(d3, d4));
}

vec4 
UnionFigures(vec4 d1)
{
    return d1;
}

vec4 
UnionFigures(vec4 d1, vec4 d2)
{
    return (d1.w < d2.w) ? d1 : d2;
}

vec4 
UnionFigures(vec4 d1, vec4 d2, vec4 d3)
{
    return UnionFigures(d1, UnionFigures(d2, d3));
}

vec4 
UnionFigures(vec4 d1, vec4 d2, vec4 d3, vec4 d4)
{
    return UnionFigures(UnionFigures(d1, d2), UnionFigures(d3, d4));
}

float 
IntersectFigures(float d1, float d2)
{
    return max(d1, d2);
}

float 
SubtractFigures(float d1, float d2)
{
    return max(-d1, d2);
}

vec4 
IntersectFigures(vec4 d1, vec4 d2)
{
    return (d1.w < d2.w) ? d2 : d1;
}

vec4 
SubtractFigures(vec4 d1, vec4 d2)
{
    d1.w = -d1.w;
    return (d1.w < d2.w) ? d2 : d1;
}

float smin ( float a, float b, float k )
{
        float res = exp ( -k*a ) + exp ( -k*b );
        return -log ( res ) / k;
}


float smoothUnion ( float d1, float d2, float k ) 
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );

    return mix( d2, d1, h ) - k*h*(1.0-h); 
}

// Rotation matrix around the X axis.
mat3 rotateX(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(1, 0,  0),
        vec3(0, c, -s),
        vec3(0, s,  c)
    );
}

// Rotation matrix around the Y axis.
mat3 rotateY(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c,  0, s),
        vec3(0,  1, 0),
        vec3(-s, 0, c)
    );
}