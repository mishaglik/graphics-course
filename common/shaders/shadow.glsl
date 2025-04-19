

float shadowI(vec3 cam_pos, int wId) {

  const vec3 world_pos = getWorldPos(cam_pos, world.mIView[0]);

  const vec4 posLightClipSpace = world.mProjView[wId] * vec4(world_pos, 1.0f);

  const vec3 posLightSpaceNDC = posLightClipSpace.xyz / posLightClipSpace.w;
  
  const vec3 shadowTexCoord = posLightSpaceNDC.xyz*0.5f + vec3(0.5f);

  const bool  outOfView = (shadowTexCoord.x < 0.0001f || shadowTexCoord.x > 0.9999f || shadowTexCoord.y < 0.0001f || shadowTexCoord.y > 0.9999f);
  if(outOfView) {
    return -1.f;
  }
  return ((posLightSpaceNDC.z < textureLod(shadow_depth, vec3(shadowTexCoord.xy, wId-1), 0).x + 0.001f) || outOfView) ? 1.0f : 0.0f;
}

float shadow(vec3 cam_pos) {
  float s = -1.f;
  for(int i = textureSize(shadow_depth, 0).z; i > 0; --i) {
    float ns= shadowI(cam_pos, i);
    if(ns > -1.f) {
      s = ns;
    }
  }
  if(s < 0) {
    s = 1;
  }
  return s;
}

