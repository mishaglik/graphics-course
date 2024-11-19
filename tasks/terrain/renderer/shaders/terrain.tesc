#version 450

layout (vertices=4) out;

layout(location = 0) in vec2 TexCoord[];

layout(location = 0) out vec2 TextureCoord[];

layout(push_constant) uniform params_t
{
  vec2 base; 
  vec2 extent;
  mat4 mProjView;
  int degree;
} params;

void main()
{
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    TextureCoord[gl_InvocationID] = TexCoord[gl_InvocationID];
    vec3 camPos = vec3(0, 10, 0);
    vec4 dists = vec4(
        length(gl_in[0].gl_Position.xyz - camPos),
        length(gl_in[1].gl_Position.xyz - camPos),
        length(gl_in[2].gl_Position.xyz - camPos),
        length(gl_in[3].gl_Position.xyz - camPos)
    );

    float tscoef = 1024.;

    if (gl_InvocationID == 0)
    {
        gl_TessLevelOuter[0] = tscoef / (dists.x + dists.y);
        gl_TessLevelOuter[1] = tscoef / (dists.y + dists.z);
        gl_TessLevelOuter[2] = tscoef / (dists.z + dists.w);
        gl_TessLevelOuter[3] = tscoef / (dists.w + dists.x);

        gl_TessLevelInner[0] = 2 * tscoef / (dot(dists, vec4(1, 1, 1, 1)));
        gl_TessLevelInner[1] = 2 * tscoef / (dot(dists, vec4(1, 1, 1, 1)));
    }
}