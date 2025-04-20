const float M_PI = 3.14159267;
const float ior = 1.5;

vec3 specular_D(float roughnessSq, float NdotH) {
    float d = (NdotH * NdotH) * (roughnessSq - 1) + 1;
    return vec3(roughnessSq / (M_PI * d * d));
}

float specular_Smith(float roughnessSq, float NdotX) {
    float d = abs(NdotX) + sqrt(mix(NdotX * NdotX, 1, roughnessSq));
    return d;
}

vec3 specular_Vis(float roughnessSq, float NdotL, float NdotV) {
    float vis = specular_Smith(roughnessSq, NdotL) * specular_Smith(roughnessSq, NdotV);
    if(vis > 0) {
        return vec3(1. / vis);
    }
    return vec3(0);
}

vec3 specular_brdf(float roughnessSq, float NdotH, float NdotL, float NdotV) {
    return specular_D(roughnessSq, NdotH) * specular_Vis(roughnessSq, NdotL, NdotV);
}

vec3 conductor_fresnel(vec3 bsdf, vec3 f0, float VdotH) {
  return bsdf * (f0 + (vec3(1) - f0) * pow(clamp(1.0 - abs(VdotH), 0.0, 1.0), 5.0));
}

vec3 fresnel_mix(vec3 base, vec3 layer, float VdotH) {
  //f0 = ((1-ior)/(1+ior));
  //f0 = f0 * f0;
  float fr = 0.04 + (1 - 0.04) * pow((1 - abs(VdotH)), 5);
  return mix(base, layer, fr);
}


vec3 lambertian(vec3 color) {
    return color / M_PI;
}

vec3 pbr_light(vec3 baseColor, vec3 pos, vec3 normal, vec3 lightDir, vec4 material, vec3 reflection, float shadow, mat4 mView)
{
    const float roughness = material.g; 
    const float metallic  = material.b; 

    const vec3 v = normalize(-pos);
    const vec3 l = normalize(lightDir);
    const vec3 n = normalize(normal);
    const vec3 h = normalize(l + v);
    const mat3 iv3 = inverse(mat3(mView));
    const float vdoth = dot(normalize(iv3 * v), normalize(iv3 * h)); 
    const float ndotl = clamp(dot(n, l), 0, 1); 

    vec3 spec = vec3(0);
    if(dot(h, l) >= 0 && dot(h, v) >= 0 && dot(n, h) >= 0) {
        spec = specular_brdf(roughness * roughness, dot(n, h), dot(n, l), dot(n, v));
    }
    vec3 metal_brdf = conductor_fresnel(spec, baseColor * reflection, vdoth);
    vec3 dielectric_brdf = fresnel_mix(max(shadow * ndotl, 0.05) * lambertian(baseColor), shadow * ndotl == 0 ? (max(shadow * ndotl, 0.05) * lambertian(baseColor)) : spec, vdoth);
    return mix(
        dielectric_brdf,
        metal_brdf,
        metallic
    );
}

/**
    Required: params.usePbr
*/

vec3 pbr_specular(vec3 pos, vec3 normal, vec3 lightDir, float roughness) {
    const vec3 v = normalize(-pos);
    const vec3 l = normalize(lightDir);
    const vec3 n = normalize(normal);
    const vec3 h = normalize(l + v);
    const float vdoth = clamp(dot(v, h), 0, 1); 
    const float ndotl = clamp(dot(n, l), 0, 1); 

    vec3 spec = vec3(0);
    if(dot(h, l) >= 0 && dot(h, v) >= 0 && dot(n, h) >= 0) {
        spec = specular_brdf(roughness * roughness, dot(n, h), dot(n, l), dot(n, v));
    }
    return spec;
}

vec3 pbr_light_mixing(vec3 baseColor, float vdoth, vec3 diffuse, vec3 reflection, vec3 specular, float metallic)
{    
    vec3 metal_brdf = conductor_fresnel(specular, baseColor * reflection, 1);
    vec3 dielectric_brdf = fresnel_mix(baseColor * diffuse, specular, vdoth);

    return mix(
        dielectric_brdf,
        metal_brdf,
        metallic
    );
}

vec3 light_pbr_no_gi(vec3 pos, vec3 normal, vec3 baseColor, vec4 material, vec3 lightDir, vec3 lightColor, vec3 reflection) {
    const float roughness = material.g; 
    const float metallic  = material.b; 

    const vec3 v = normalize(-pos);
    const vec3 n = normalize(normal);
    const vec3 l = normalize(lightDir);
    const vec3 h = normalize(l + v);
    const float vdoth = dot(v, h); 
    const float ndotl = clamp(dot(n, l), 0, 1); 

    const vec3 diffuse = max(ndotl, 0.05) * lambertian(baseColor);

    const vec3 specular = pbr_specular(
        pos,
        normal,
        lightDir,
        roughness
    );

    return pbr_light_mixing(
        baseColor,
        vdoth,
        diffuse,
        reflection,
        specular,
        metallic
    );
}

vec3 light_pbr_shadow_no_gi(vec3 pos, vec3 normal, vec3 baseColor, vec4 material, vec3 lightDir, vec3 lightColor, vec3 reflection) {
    
    const float roughness = material.g; 
    const float metallic  = material.b; 

    const vec3 v = normalize(-pos);
    const vec3 n = normalize(normal);
    const vec3 l = normalize(lightDir);
    const vec3 h = normalize(l + v);
    const float vdoth = dot(v, h); 
    const float ndotl = clamp(dot(n, l), 0, 1); 

    const vec3 diffuse = max(shadow(pos) * ndotl, 0.05) * lambertian(baseColor);

    const vec3 specular = shadow(pos) * pbr_specular(
        pos,
        normal,
        lightDir,
        roughness
    );

    return pbr_light_mixing(
        baseColor,
        vdoth,
        diffuse,
        reflection,
        specular,
        metallic
    );
}


vec3 light_pbr_shadow_gi(vec3 pos, vec3 normal, vec3 baseColor, vec4 material, vec3 lightDir, vec3 lightColor, vec3 reflection, vec3 diffuse) {
    return diffuse;
    const float roughness = material.g; 
    const float metallic  = material.b; 

    const vec3 v = normalize(-pos);
    const vec3 n = normalize(normal);
    const vec3 l = normalize(lightDir);
    const vec3 h = normalize(l + v);
    const float vdoth = dot(v, h); 
    const float ndotl = clamp(dot(n, l), 0, 1); 

    const vec3 specular = shadow(pos) * pbr_specular(
        pos,
        normal,
        lightDir,
        roughness
    );

    return pbr_light_mixing(
        baseColor,
        vdoth,
        diffuse,
        reflection,
        specular,
        metallic
    );
}


vec3 light(vec3 pos, vec3 normal, vec3 baseColor, vec4 material, vec3 lightDir, vec3 lightColor, vec3 reflection) {
    return light_pbr_no_gi(pos, normal, baseColor, material, lightDir, lightColor, reflection);
}