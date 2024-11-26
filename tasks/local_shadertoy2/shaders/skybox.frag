#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out vec4 color;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;

layout(push_constant) uniform params
{
  vec3 forward;
  vec3 up;
  float iTime;
} pushConstant;

layout(binding = 0) uniform sampler2D texStars;
layout(binding = 1) uniform sampler2D texRust;
layout(binding = 2) uniform sampler2D texCobble;



vec3 moonColor(in vec3 rd)
{
    float r = length(rd - normalize(vec3(0.35, 0.20, -1))) / 0.02;
    if (r > 1.)
    {
        return vec3(0, 0, 0);
    }
    
    vec4 texColor = texture(texCobble, 20. * rd.yz);
    return pow(sin(1. - r), 0.2) * vec3(1, 1.0, 0.8) * pow(texColor.r, 0.1);
}

vec3 blinkingStars(in vec3 r)
{
    vec3 rd = r * 100. * abs(sin(0.0004 * pushConstant.iTime));;
    return pow(clamp(0.17 * 
                    (   1.1 * sin(1000. * r.z * r.x)  + 
                        1.2 * sin(10. * rd.z + rd.x * 2.)  + 
                        2.  * cos(100. * rd.x)  + 
                        1.7  * cos(pow(abs(1000. * r.x), 3.)) +
                        0.1  * sin(pow(abs(1000. * r.z), 3.))
                        ), 0., 1.), 100.) * vec3(0.6, 0.6, 0.6);
}

vec3 normalStars(in vec3 r)
{
    vec3 rd = r * 100.;
    return pow(clamp(0.36 * 
                    (   sin(1000. * r.z * r.x)  + 
                        sin(100. * rd.z + rd.x * 2.)  + 
                        cos(100. * rd.x)
                        ), 0., 1.), 100.) * abs(sin(1000. * (r.x + r.z))) * vec3(0.4, 0.4, 0.4);
}

void mainCubemap( out vec4 fragColor, in vec2 fragCoord, in vec3 rayOri, in vec3 rayDir )
{
    // Ray direction as color
    vec3 baseColor = vec3(0.03, 0.03, 0.03);

    baseColor += moonColor(rayDir);
    if (length(baseColor) < 0.3)
    {
        baseColor += normalStars(rayDir) + blinkingStars(rayDir);
    }
    
    // Output to cubemap
    
    fragColor = 0.1 * vec4(baseColor.rgb, 1.0);
}

void main()
{
  //color = vec4(surf.texCoord, pushConstant.layer / 6., 1);
  vec2 tc = 2. * surf.texCoord;
  tc.y = 1 - tc.y;
  tc.x = 1 - tc.x;

  mainCubemap(color, surf.texCoord, vec3(0, 0, 0), normalize(vec3(tc.x, tc.y, -1.)));

}
