#pragma once
/*
#include "obj_loader.h"
#include "stb_image.h"

#include "hello_vulkan.h"
#include "nvh/alignment.hpp"
#include "nvh/cameramanipulator.hpp"
#include "nvh/fileoperations.hpp"
#include "nvvk/commands_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/images_vk.hpp"
#include "nvvk/pipeline_vk.hpp"
#include "nvvk/renderpasses_vk.hpp"
#include "nvvk/shaders_vk.hpp"
#include "nvvk/buffers_vk.hpp"
#include "shaders/host_device.h"
#include "SH_includes.h"
*/
struct HashTester
{
  int a = 0;


};
/*

	std::array<uint32_t, 100'000> hashTable;

	static void test_and_print_stats() {



	}

	//-----------------------------
#define HASH_MAP_SIZE 100000
#define S_MIN 0.0000000001

  struct HashCell
  {
    float ao_value;  // the averaged ambient occlusion value in the given hash cell
    uint32_t contribution_counter;  // number of samples contributing to value for blending in new values (old * cc/(cc+1) + new * 1 / (cc+1))
    uint32_t checksum;  // checksum for deciding if cell should be resetted or contribution added
  };

  struct ConfigurationValues
  {
    vec3  camera_position;  // camera position
    int   s_nd;             // normal coarseness
    int   s_p;              // user-defined level of coarseness in pixel
    float f;                // camera aperture
    nvmath::vec2ui res;              // screen resolution in pixel
  };

  //
  //  Hash Function: Fowler-Noll-Vo (FNV-1 hash 32-bit version)
  //
  const uint32_t FNV_offset_basis = 0x811c9dc5;
  const uint32_t FNV_prime        = 0x01000193;

  uint32_t[4] _dismemberBytes(uint32_t x)
  {
    uint32_t[4] res;
    res[0] = x & 0xFF000000 >> 24;
    res[1] = x & 0x00FF0000 >> 16;
    res[2] = x & 0x0000FF00 >> 8;
    res[3] = x & 0x000000FF;
    return res;
  }

  uint32_t _encode(uint32_t hash, uint32_t byte)
  {
    hash = hash * FNV_prime;
    hash = hash ^ byte;
    return hash;
  }

  uint32_t h1(float x)
  {
    uint32_t bytes[4] = _dismemberBytes(reinterpret_cast<uint32_t>(x)));
    uint32_t hash     = 1;

    for(int i = 0; i < 4; i++)
      hash = _encode(hash, bytes[i]);

    return hash;
  }

  //
  //  Jenkings Hash Function
  //

  uint32_t h2(float x)
  {
    uint32_t bytes[4] = _dismemberBytes(floatBitsToUint(x));
    uint32_t hash     = 0;

    for(int i = 0; i < 4; i++)
    {
      hash += bytes[i];
      hash += hash << 10;
      hash = hash ^ (hash >> 6);
    }
    hash += hash << 3;
    hash = hash ^ (hash >> 11);
    hash += hash << 15;
    return hash;
  }

  uint32_t roundhash(vec3 position)
  {
    ivec3 posi = ivec3(position.x, position.y, position.z);
    return floatBitsToUint(intBitsToFloat(posi.x * posi.y * posi.z)) % HASH_MAP_SIZE;
  }

  //
  //  Multiple Dimension Hash Functions
  //

  //function to specifically adress different levels for blurr later ?
  uint32_t H4D_SWD(vec3 position, uint s_wd)
  {
    return h1(s_wd + h1(position.z / s_wd + h1(position.y / s_wd + h1(position.x / s_wd))));
  }

  //hash function to hash points without normal
  uint32_t H4D(ConfigurationValues c, vec3 position)
  {
    float dis  = length(position - c.camera_position);
    float s_w  = dis * tan(max(c.f / c.res.x, c.f * c.res.x / pow(c.res.y, 2)) * c.s_p);
    uint32_t s_wd = uint(pow(2, uint(log2(s_w / S_MIN)) * S_MIN));
    return H4D_SWD(position, s_wd);
  }

  // Actually used all-inclusive function
  uint32_t H7D(ConfigurationValues c, vec3 position, vec3 normal)
  {
    normal         = normal * c.s_nd;
    ivec3 normal_d = ivec3(normal);
    return h1(normal_d.z + h1(normal_d.y + h1(normal_d.x + H4D(c, position)))) % HASH_MAP_SIZE;
  }

  //
  // Checksum functions
  //

  //function to specifically adress different levels for blurr later ?
  uint32_t H4D_SWD_checksum(vec3 position, uint s_wd)
  {
    return h2(s_wd + h2(position.z / s_wd + h2(position.y / s_wd + h2(position.x / s_wd))));
  }

  //hash function to hash points with normal
  uint32_t H4D_checksum(ConfigurationValues c, vec3 position)
  {
    float dis  = length(position - c.camera_position);
    float s_w  = dis * tan(max(c.f / c.res.x, c.f * c.res.x / pow(c.res.y, 2)) * c.s_p);
    uint32_t s_wd = uint(pow(2, uint(log2(s_w / S_MIN)) * S_MIN));
    return H4D_SWD_checksum(position, s_wd);
  }

  // Actually used all-inclusive function for checksum
  uint32_t H7D_checksum(ConfigurationValues c, vec3 position, vec3 normal)
  {
    normal         = normal * c.s_nd;
    nvmath::vec3i normal_d = ivec3(normal);
    return h2(normal_d.z + h2(normal_d.y + h2(normal_d.x + H4D_checksum(c, position))));
  }



*/
