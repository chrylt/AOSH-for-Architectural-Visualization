#version 450

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_scalar_block_layout : enable
#include "raycommon.glsl"
#include "SH_hash_tools.glsl"

layout(location = 0) in vec2 outUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D noisyTxt;
layout(set = 0, binding = 1) uniform sampler2D aoTxt;

layout(scalar, set = 0, binding = 2) readonly buffer HashMap {
    HashCell hashMap[HASH_MAP_SIZE];
};

layout(scalar, set = 0, binding = 3) uniform UniformBuffer {
    ConfigurationValues config;
};

layout(set = 0, binding = 4, rgba32f) uniform image2D _gBuffer;

layout(push_constant) uniform shaderInformation
{
  float aspectRatio;
}
pushc;

vec2 fragCoordToUV(vec2 fragCoord){
    return vec2(fragCoord.xy) / vec2(config.res.xy);
}


void main()
{
  vec2  uv    = outUV;
  float gamma = 1. / 2.2;
  vec4 color = texture(noisyTxt, uv);

  // Retrieving position and normal
  vec4 gBuffer = imageLoad(_gBuffer, ivec2(gl_FragCoord.xy));

  if(gBuffer != vec4(0))
  {
    vec3 position_mid = gBuffer.xyz;
    vec3 normal_mid = DecompressUnitVec(floatBitsToUint(gBuffer.w));

    // Calculate cell size to obtain the size of the sampling kernel
    float s_wd = s_wd_calc(config, position_mid) * pow2[config.coarseness_level_increase];
    
    //give hash-cells colors
    if(config.debug_color)
       color = hash_to_color(H7D_SWD(config, position_mid, normal_mid, s_wd));
  }
  else{
    color = vec4(0,0,0,1);
  }

  float final_ao = texture(aoTxt, uv).x;
  fragColor = pow(color * final_ao, vec4(gamma));
}
