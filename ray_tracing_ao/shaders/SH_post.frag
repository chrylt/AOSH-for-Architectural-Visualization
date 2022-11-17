/*
 * Copyright (c) 2019-2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-FileCopyrightText: Copyright (c) 2019-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */

#version 450

#extension GL_GOOGLE_include_directive : enable
#include "raycommon.glsl"
#include "SH_hash_tools.glsl"

layout(location = 0) in vec2 outUV;
layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D noisyTxt;
layout(set = 0, binding = 1) uniform sampler2D aoTxt;

layout(set = 0, binding = 2) readonly buffer HashMap {
    HashCell hashMap[HASH_MAP_SIZE];
};

layout(set = 0, binding = 3) uniform UniformBuffer {
    ConfigurationValues config;
};


layout(set = 0, binding = 4, rgba32f) uniform image2D _gBuffer;


layout(push_constant) uniform shaderInformation
{
  float aspectRatio;
}
pushc;


void main()
{
  vec2  uv    = outUV;
  float gamma = 1. / 2.2;
  //vec4  color = texture(noisyTxt, uv);
  float ao    = texture(aoTxt, uv).x;
  vec4 color = vec4(1.0, 1.0, 1.0, 1.0);

  // Retrieving position and normal
  vec4 gBuffer = imageLoad(_gBuffer, ivec2(gl_FragCoord.xy));

  // Shooting rays only if a fragment was rendered
  if(gBuffer != vec4(0))
  {
    vec3 origin = gBuffer.xyz;
    vec3 normal = DecompressUnitVec(floatBitsToUint(gBuffer.w));

    //uint hash = H7D(config, origin, normal);
    //uint checksum = H7D_checksum(config, origin, normal);
    uint hash = H4D(config, origin);
    uint checksum = H4D_checksum(config, origin);

    if(checksum == hashMap[hash].checksum){
        //ao = hashMap[hash].ao_value;
        color = vec4(0.0, 1.0, 0.0, 1.0);
    } else{
        //ao = hashMap[hash].ao_value;
        color = vec4(1.0, 0.0, 0.0, 1.0);
    }
    

    
  }else{
    ao = 0.5;
  }

  

  
  fragColor = pow(color * ao, vec4(gamma));
}
