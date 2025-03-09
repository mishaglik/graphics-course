#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : require

layout(location = 0) out vec4 out_fragColor;

layout (location = 0 ) in VS_OUT
{
  vec2 texCoord;
} surf;

layout(binding = 0) uniform sampler2D image;

float FxaaLuma(vec3 rgb) {
  return rgb.y * (0.587/0.299) + rgb.x; 
}

#define FXAA_EDGE_THRESHOLD 0.25f
#define FXAA_EDGE_THRESHOLD_MIN 0.03125f

#define QUALITY(q) ((q) < 5 ? 1.0 : ((q) > 5 ? ((q) < 10 ? 2.0 : ((q) < 11 ? 4.0 : 8.0)) : 1.5))
#define ITERATIONS 12
#define SUBPIXEL_QUALITY 0.75

#define STOP_AT 3

vec3 localContrast() {
  const vec2 delta = 1. / textureSize(image, 0);
  vec3 rgbM = texture(image, surf.texCoord).rgb;
  float lumaM = FxaaLuma(rgbM);

  float lumaN = FxaaLuma(texture(image, surf.texCoord + vec2(0, -delta.y)).rgb);
  float lumaS = FxaaLuma(texture(image, surf.texCoord + vec2(0,  delta.y)).rgb);
  float lumaW = FxaaLuma(texture(image, surf.texCoord + vec2(-delta.x, 0)).rgb);
  float lumaE = FxaaLuma(texture(image, surf.texCoord + vec2( delta.x, 0)).rgb);

  float lumaNW = FxaaLuma(texture(image, surf.texCoord + vec2(-delta.x, -delta.y)).rgb);
  float lumaSW = FxaaLuma(texture(image, surf.texCoord + vec2( delta.x, -delta.y)).rgb);
  float lumaNE = FxaaLuma(texture(image, surf.texCoord + vec2(-delta.x,  delta.y)).rgb);
  float lumaSE = FxaaLuma(texture(image, surf.texCoord + vec2( delta.x,  delta.y)).rgb);

  float lumaNS = lumaS + lumaN;
	float lumaWE = lumaW + lumaE;
	
	// Same for corners
	float lumaWNS = lumaSW + lumaNW;
	float lumaSEW = lumaSW + lumaSE;
	float lumaENS = lumaSE + lumaNE;
	float lumaNEW = lumaNE + lumaNW;

  float rangeMin = min(lumaM, min(min(lumaN, lumaW), min(lumaS, lumaE)));
  float rangeMax = max(lumaM, max(max(lumaN, lumaW), max(lumaS, lumaE)));
  float range = rangeMax - rangeMin;
  if(range > max(FXAA_EDGE_THRESHOLD_MIN, rangeMax * FXAA_EDGE_THRESHOLD)) {
    //ANCHOR - STEP 1 - localContrast
    #if STOP_AT == 0
    //return vec3(1, 0, 0); 
    #endif
    // float lumaL = (lumaN + lumaW + lumaE + lumaS) * 0.25;
    // float rangeL = abs(lumaL - lumaM);
    // float blendL = max(0.0, (rangeL / range) - FXAA_SUBPIX_TRIM) * FXAA_SUBPIX_TRIM_SCALE;
    // blendL = min(FXAA_SUBPIX_CAP, blendL);

    float edgeVert =
      abs((0.25 * lumaNW) + (-0.5 * lumaN) + (0.25 * lumaNE)) +
      abs((0.50 * lumaW ) + (-1.0 * lumaM) + (0.50 * lumaE )) +
      abs((0.25 * lumaSW) + (-0.5 * lumaS) + (0.25 * lumaSE));
    float edgeHorz =
      abs((0.25 * lumaNW) + (-0.5 * lumaW) + (0.25 * lumaSW)) +
      abs((0.50 * lumaN ) + (-1.0 * lumaM) + (0.50 * lumaS )) +
      abs((0.25 * lumaNE) + (-0.5 * lumaE) + (0.25 * lumaSE));
    bool isHorizontal = edgeHorz >= edgeVert;
  #if STOP_AT == 1
    if(isHorizontal) {
      return vec3(1, 1, 0);
    } else {
      return vec3(0, 0, 1);
    }
  #endif

    float stepLength = isHorizontal ? delta.y : delta.x;
    
    float luma1 = isHorizontal ? lumaS : lumaW;
    float luma2 = isHorizontal ? lumaN : lumaE;

    float gradient1 = luma1 - lumaM;
    float gradient2 = luma2 - lumaM;
    
    bool is1Steepest = abs(gradient1) >= abs(gradient2);

  #if STOP_AT == 2 
    if(is1Steepest) {
      return vec3(0, 1, 1);
    } else {
      return vec3(0, 0, 1);
    }
  #endif

    // Gradient in the corresponding direction, normalized.
    float gradientScaled = 0.25*max(abs(gradient1),abs(gradient2));
    
    // Average luma in the correct direction.
    float lumaLocalAverage = 0.0;
    if(is1Steepest){
      // Switch the direction
      stepLength = - stepLength;
      lumaLocalAverage = 0.5*(luma1 + lumaM);
    } else {
      lumaLocalAverage = 0.5*(luma2 + lumaM);
    }
    
    // Shift UV in the correct direction by half a pixel.
    vec2 currentUv = surf.texCoord;
    if(isHorizontal){
      currentUv.y += stepLength * 0.5;
    } else {
      currentUv.x += stepLength * 0.5;
    }
    
    // Compute offset (for each iteration step) in the right direction.
    vec2 offset = isHorizontal ? vec2(delta.x,0.0) : vec2(0.0,delta.y);
    // Compute UVs to explore on each side of the edge, orthogonally. The QUALITY allows us to step faster.
    vec2 uv1 = currentUv - offset * QUALITY(0);
    vec2 uv2 = currentUv + offset * QUALITY(0);
    
    // Read the lumas at both current extremities of the exploration segment, and compute the delta wrt to the local average luma.
    float lumaEnd1 = FxaaLuma(texture(image, uv1, 0.0).rgb);
    float lumaEnd2 = FxaaLuma(texture(image, uv2, 0.0).rgb);
    lumaEnd1 -= lumaLocalAverage;
    lumaEnd2 -= lumaLocalAverage;
    
    // If the luma deltas at the current extremities is larger than the local gradient, we have reached the side of the edge.
    bool reached1 = abs(lumaEnd1) >= gradientScaled;
    bool reached2 = abs(lumaEnd2) >= gradientScaled;
    bool reachedBoth = reached1 && reached2;
    
    // If the side is not reached, we continue to explore in this direction.
    if(!reached1){
      uv1 -= offset * QUALITY(1);
    }
    if(!reached2){
      uv2 += offset * QUALITY(1);
    }

    if(!reachedBoth){
      
      for(int i = 2; i < ITERATIONS; i++){
        // If needed, read luma in 1st direction, compute delta.
        if(!reached1){
          lumaEnd1 = FxaaLuma(texture(image, uv1, 0.0).rgb);
          lumaEnd1 = lumaEnd1 - lumaLocalAverage;
        }
        // If needed, read luma in opposite direction, compute delta.
        if(!reached2){
          lumaEnd2 = FxaaLuma(texture(image, uv2, 0.0).rgb);
          lumaEnd2 = lumaEnd2 - lumaLocalAverage;
        }
        // If the luma deltas at the current extremities is larger than the local gradient, we have reached the side of the edge.
        reached1 = abs(lumaEnd1) >= gradientScaled;
        reached2 = abs(lumaEnd2) >= gradientScaled;
        reachedBoth = reached1 && reached2;
        
        // If the side is not reached, we continue to explore in this direction, with a variable quality.
        if(!reached1){
          uv1 -= offset * QUALITY(i);
        }
        if(!reached2){
          uv2 += offset * QUALITY(i);
        }
        
        // If both sides have been reached, stop the exploration.
        if(reachedBoth){ break;}
      }
      
    }
    
    // Compute the distances to each side edge of the edge (!).
    float distance1 = isHorizontal ? (surf.texCoord.x - uv1.x) : (surf.texCoord.y - uv1.y);
    float distance2 = isHorizontal ? (uv2.x - surf.texCoord.x) : (uv2.y - surf.texCoord.y);
    
    // In which direction is the side of the edge closer ?
    bool isDirection1 = distance1 < distance2;
    float distanceFinal = min(distance1, distance2);
    
    // Thickness of the edge.
    float edgeThickness = (distance1 + distance2);
    
    // Is the luma at center smaller than the local average ?
    bool isLumaCenterSmaller = lumaM < lumaLocalAverage;
    
    // If the luma at center is smaller than at its neighbour, the delta luma at each end should be positive (same variation).
    bool correctVariation1 = (lumaEnd1 < 0.0) != isLumaCenterSmaller;
    bool correctVariation2 = (lumaEnd2 < 0.0) != isLumaCenterSmaller;
    
    // Only keep the result in the direction of the closer side of the edge.
    bool correctVariation = isDirection1 ? correctVariation1 : correctVariation2;
    
    // UV offset: read in the direction of the closest side of the edge.
    float pixelOffset = - distanceFinal / edgeThickness + 0.5;
    
    // If the luma variation is incorrect, do not offset.
    float finalOffset = correctVariation ? pixelOffset : 0.0;
    
    // Sub-pixel shifting
    // Full weighted average of the luma over the 3x3 neighborhood.
    float lumaAverage = (1.0/12.0) * (2.0 * (lumaNS + lumaWE) + lumaWNS + lumaENS);
    // Ratio of the delta between the global average and the center luma, over the luma range in the 3x3 neighborhood.
    float subPixelOffset1 = clamp(abs(lumaAverage - lumaM)/range,0.0,1.0);
    float subPixelOffset2 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;
    // Compute a sub-pixel offset based on this delta.
    float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;
    
    // Pick the biggest of the two offsets.
    finalOffset = max(finalOffset,subPixelOffsetFinal);
    
    // Compute the final UV coordinates.
    vec2 finalUv = surf.texCoord;
    if(isHorizontal){
      finalUv.y += finalOffset * stepLength;
    } else {
      finalUv.x += finalOffset * stepLength;
    }
    return texture(image, finalUv).rgb;
  }
  return rgbM;
}

void main(void)
{
  vec4 color = texture(image, surf.texCoord);
  out_fragColor = vec4(localContrast(), 0);
  // out_fragColor = vec4(color.rgb, 1);
}