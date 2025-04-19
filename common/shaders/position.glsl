

vec3 getScreenPos(float depth, float wc) {
  return vec3(
    (2 * gl_FragCoord.x / resolution.x) - 1,
    (2 * gl_FragCoord.y / resolution.y) - 1,
    depth
  ) / wc;
}

vec3 getScreenPos(vec2 texCoord, float depth, float wc) {
  return vec3(
    (2 * texCoord.x / resolution.x) - 1,
    (2 * texCoord.y / resolution.y) - 1,
    depth
  ) / wc;
}

vec3 getCamPos(vec3 pos_screen, mat4 mProj) {
    return inverse(mat3(mProj)) * (pos_screen - vec3(0, 0, mProj[3][2]));
}

vec3 getWorldPos(vec3 pos_cam, mat4 mIView) {
    const vec3 pos = (mIView * vec4(pos_cam, 1)).xyz;
    return pos;
}

vec3 getCamWorldPos(mat4 mIView) {
    return (mIView * vec4(0,0,0,1)).xyz;
}