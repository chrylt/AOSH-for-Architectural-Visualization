/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION.  All rights reserved.
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
 * SPDX-FileCopyrightText: Copyright (c) 2014-2021 NVIDIA CORPORATION
 * SPDX-License-Identifier: Apache-2.0
 */


// ImGui - standalone example application for Glfw + Vulkan, using programmable
// pipeline If you are new to ImGui, see examples/README.txt and documentation
// at the top of imgui.cpp.

#include <array>
#include <iostream>

#include "backends/imgui_impl_glfw.h"
#include "imgui.h"

#include "hello_vulkan.h"
#include "imgui/imgui_camera_widget.h"
#include "nvh/cameramanipulator.hpp"
#include "nvh/fileoperations.hpp"
#include "nvpsystem.hpp"
#include "nvvk/commands_vk.hpp"
#include "nvvk/context_vk.hpp"

#include "SH_HashTester.h"
#include "Timer.h"



//////////////////////////////////////////////////////////////////////////
#define UNUSED(x) (void)(x)
//////////////////////////////////////////////////////////////////////////

// Default search path for shaders
std::vector<std::string> defaultSearchPaths;


// GLFW Callback functions
static void onErrorCallback(int error, const char* description)
{
  fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

// Extra UI
void renderUI(HelloVulkan& helloVk)
{
  
  ImGuiH::CameraWidget();/*
  if(ImGui::CollapsingHeader("Light"))
  {
    ImGui::RadioButton("Point", &helloVk.m_pcRaster.lightType, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Infinite", &helloVk.m_pcRaster.lightType, 1);

    ImGui::SliderFloat3("Position", &helloVk.m_pcRaster.lightPosition.x, -20.f, 20.f);
    ImGui::SliderFloat("Intensity", &helloVk.m_pcRaster.lightIntensity, 0.f, 150.f);
  }*/
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
static int const SAMPLE_WIDTH  = 1280;
static int const SAMPLE_HEIGHT = 720;

//--------------------------------------------------------------------------------------------------
// Application Entry
//
int main(int argc, char** argv)
{
  UNUSED(argc);

  // Setup GLFW window
  glfwSetErrorCallback(onErrorCallback);
  if(!glfwInit())
  {
    return 1;
  }
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(SAMPLE_WIDTH, SAMPLE_HEIGHT, PROJECT_NAME, nullptr, nullptr);


  // Setup camera
  CameraManip.setWindowSize(SAMPLE_WIDTH, SAMPLE_HEIGHT);
  CameraManip.setLookat(nvmath::vec3f(17.5, 5.2, -5.6), nvmath::vec3f(10.4, 4.5, -4.7), nvmath::vec3f(0, 1, 0));

  // Setup Vulkan
  if(!glfwVulkanSupported())
  {
    printf("GLFW: Vulkan Not Supported\n");
    return 1;
  }

  // setup some basic things for the sample, logging file for example
  NVPSystem system(PROJECT_NAME);

  // Search path for shaders and other media
  defaultSearchPaths = {
      NVPSystem::exePath() + PROJECT_RELDIRECTORY,
      NVPSystem::exePath() + PROJECT_RELDIRECTORY "..",
      std::string(PROJECT_NAME),
  };

  // Vulkan required extensions
  assert(glfwVulkanSupported() == 1);
  uint32_t count{0};
  auto     reqExtensions = glfwGetRequiredInstanceExtensions(&count);

  // Requesting Vulkan extensions and layers
  nvvk::ContextCreateInfo contextInfo;
  contextInfo.setVersion(1, 2);                       // Using Vulkan 1.2
  for(uint32_t ext_id = 0; ext_id < count; ext_id++)  // Adding required extensions (surface, win32, linux, ..)
    contextInfo.addInstanceExtension(reqExtensions[ext_id]);
  contextInfo.addInstanceLayer("VK_LAYER_LUNARG_monitor", true);              // FPS in titlebar
  contextInfo.addInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, true);  // Allow debug names
  contextInfo.addDeviceExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);            // Enabling ability to present rendering

  // #VKRay: Activate the ray tracing extension
  VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
  contextInfo.addDeviceExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, false, &accelFeature);  // To build acceleration structures
  VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
  contextInfo.addDeviceExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME, false, &rayQueryFeatures);  // Ray tracing in compute shader
  contextInfo.addDeviceExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);  // Required by ray tracing pipeline

  // Creating Vulkan base application
  nvvk::Context vkctx{};
  vkctx.initInstance(contextInfo);
  // Find all compatible devices
  auto compatibleDevices = vkctx.getCompatibleDevices(contextInfo);
  assert(!compatibleDevices.empty());
  // Use a compatible device
  vkctx.initDevice(compatibleDevices[0], contextInfo);

  // Create example
  HelloVulkan helloVk;

  // Window need to be opened to get the surface on which to draw
  const VkSurfaceKHR surface = helloVk.getVkSurface(vkctx.m_instance, window);
  vkctx.setGCTQueueWithPresent(surface);

  helloVk.setup(vkctx.m_instance, vkctx.m_device, vkctx.m_physicalDevice, vkctx.m_queueGCT.familyIndex);
  helloVk.createSwapchain(surface, SAMPLE_WIDTH, SAMPLE_HEIGHT);
  helloVk.createDepthBuffer();
  helloVk.createRenderPass();
  helloVk.createFrameBuffers();

  // Setup Imgui
  helloVk.initGUI(0);  // Using sub-pass 0

  std::string filePath{"media/scenes/sponza_small.obj"};

  // Creation of the example
  nvmath::mat4f t = nvmath::translation_mat4(nvmath::vec3f{0, 20, -100});
  //nvmath::mat4f t = nvmath::translation_mat4(nvmath::vec3f{0, 10.0, 0});
  //helloVk.loadModel(nvh::findFile("media/scenes/plane.obj", defaultSearchPaths, true), t);
  //helloVk.loadModel(nvh::findFile("media/scenes/wuson.obj", defaultSearchPaths, true));
  //helloVk.loadModel(nvh::findFile("media/scenes/Medieval_building.obj", defaultSearchPaths, true));
  //helloVk.loadModel(nvh::findFile("media/scenes/sponza_small.obj", defaultSearchPaths, true));
  helloVk.loadModel(nvh::findFile("media/scenes/sponza_new.obj", defaultSearchPaths, true));
  //helloVk.loadModel(nvh::findFile("media/scenes/sample_city_scaled.obj", defaultSearchPaths, true));
  //helloVk.loadModel(nvh::findFile("media/scenes/japan_street.obj", defaultSearchPaths, true));



  helloVk.createOffscreenRender();
  helloVk.createDescriptorSetLayout();
  helloVk.createGraphicsPipeline();
  helloVk.createUniformBuffer();
  helloVk.createObjDescriptionBuffer();

  // #VKRay
  helloVk.initRayTracing();
  helloVk.createBottomLevelAS();
  helloVk.createTopLevelAS();

  // Need the Top level AS
  helloVk.updateDescriptorSet();

  helloVk.createPostDescriptor();
  helloVk.createPostPipeline();
  helloVk.updatePostDescriptorSet();


  helloVk.createCompDescriptors();
  helloVk.updateCompDescriptors();
  helloVk.createCompPipelines();

  helloVk.createFilterDescriptors();
  helloVk.updateFilterDescriptors();
  helloVk.createFilterPipelines();


  nvmath::vec4f clearColor = nvmath::vec4f(0, 0, 0, 0);

  helloVk.setupGlfwCallbacks(window);
  ImGui_ImplGlfw_InitForVulkan(window, true);


  AoControl aoControl;
  
  Timer timer{&helloVk};

  helloVk.hashControl.debug_cells = false;
  aoControl.rtao_samples          = 4;

  bool sponza            = true;
  bool japanese_measures = false;
  bool sample_city       = false;

  nvmath::vec3f ctr;
  nvmath::vec3f eye;

  nvmath::vec3f ctr2;
  nvmath::vec3f eye2;

  if(japanese_measures)
  {
    ctr = nvmath::vec3f{23, 23, 11};
    eye = nvmath::vec3f{-17, 35, 83};

    ctr2 = nvmath::vec3f{-10, 27, -8};
    eye2 = nvmath::vec3f{-10, 27, -8};
  }

  if(sample_city)
  {
    ctr = nvmath::vec3f{4, 8, -1};
    eye = nvmath::vec3f{-1, 11, -17};

    ctr2 = nvmath::vec3f{46, 2, 19};
    eye2 = nvmath::vec3f{45, 2, 16};
  }

  if(sponza)
  {
    ctr = nvmath::vec3f{-50.5, 22.1, 11.4};
    eye = nvmath::vec3f{29.1, 3.4, -2.7};
  }

  CameraManip.setLookat(eye, ctr, nvmath::vec3f{0, 1, 0}, true);
  CameraManip.setMode(CameraManip.Fly);

  bool          setanim = false;

  int counter = 0;
  bool done    = false;

  int complete_counter = 0;
  int complete_end     = 1000;
  
  // Main loop
  while(!glfwWindowShouldClose(window))
  {
    try
    {
      complete_counter++;
      if(complete_counter > complete_end)
      {

        //goto end
      }
      if(!done)
        counter++;
      if(counter > 1000)
      {
        //CameraManip.setLookat(eye2, ctr2, nvmath::vec3f{0, 1, 0}, false);
        done = true;
        counter = -1;
      }
  

      glfwPollEvents();
      if(helloVk.isMinimized())
        continue;

      
      // Start the Dear ImGui frame
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      // Show UI window.
      if(helloVk.showGui())
      {
        ImGuiH::Panel::Begin();

        renderUI(helloVk);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::Checkbox("Show Hash Cells", (bool*)&helloVk.hashControl.debug_cells);

        
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if(ImGui::CollapsingHeader("Write Ambient Occlusion to Hash Table"))
        {
          bool changed{false};
          changed |= ImGui::SliderFloat("Radius", &aoControl.rtao_radius, 0, 10);
          changed |= ImGui::SliderInt("Rays per Pixel", &aoControl.rtao_samples, 1, 4096);
          changed |= ImGui::SliderFloat("Power", &aoControl.rtao_power, 1, 5);
          changed |= ImGui::Checkbox("Distanced Based", (bool*)&aoControl.rtao_distance_based);
          changed |= ImGui::SliderFloat("Normal Tolerance", &helloVk.hashControl.s_nd, 1, 5);
          changed |= ImGui::SliderFloat("Cell Size in Pixel", &helloVk.hashControl.s_p, 0.001, 3);

          if(changed)
            helloVk.resetFrame();
        }
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if(ImGui::CollapsingHeader("Read Ambient Occlusion from Hash Table and Filter"))
        {
          bool changed{false};
          changed |= ImGui::SliderInt("Min. Samples per Cell", &helloVk.m_configObject->min_nr_samples, 0, 150);
          changed |= ImGui::SliderFloat("Filtering Distance Falloff", &helloVk.m_configObject->gauss_var1, 0.01, 5);
          changed |= ImGui::SliderFloat("Filtering Value Falloff", &helloVk.m_configObject->gauss_var2, 0.0001, 5);
          changed |= ImGui::SliderInt("Filtering Level Increase", &helloVk.m_configObject->filter_level_increase, 0, 7);
        }
        
        ImGuiH::Panel::End();
      }

      // Start rendering the scene
      helloVk.prepareFrame();

      // Start command buffer of this frame
      auto                   curFrame = helloVk.getCurFrame();
      const VkCommandBuffer& cmdBuf   = helloVk.getCommandBuffers()[curFrame];

      VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      vkBeginCommandBuffer(cmdBuf, &beginInfo);
      timer.init(&cmdBuf, curFrame);


      // Updating camera buffer
      helloVk.updateUniformBuffer(cmdBuf);

      // Clearing screen
      std::array<VkClearValue, 3> clearValues{};
      clearValues[0].color = {{clearColor[0], clearColor[1], clearColor[2], clearColor[3]}};


      // Offscreen render pass
      {
        clearValues[1].color        = {{0, 0, 0, 0}};
        clearValues[2].depthStencil = {1.0f, 0};
        VkRenderPassBeginInfo offscreenRenderPassBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        offscreenRenderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
        offscreenRenderPassBeginInfo.pClearValues    = clearValues.data();
        offscreenRenderPassBeginInfo.renderPass      = helloVk.m_offscreenRenderPass;
        offscreenRenderPassBeginInfo.framebuffer     = helloVk.m_offscreenFramebuffer;
        offscreenRenderPassBeginInfo.renderArea      = {{0, 0}, helloVk.getSize()};

        // Rendering Scene
        {
          
          vkCmdBeginRenderPass(cmdBuf, &offscreenRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
          helloVk.rasterize(cmdBuf, timer);
          vkCmdEndRenderPass(cmdBuf);
          helloVk.runCompute(cmdBuf, aoControl, timer);
          
        }
      }

      // 2nd rendering pass: tone mapper, UI
      {
        clearValues[1].depthStencil = {1.0f, 0};
        VkRenderPassBeginInfo postRenderPassBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        postRenderPassBeginInfo.clearValueCount = 2;
        postRenderPassBeginInfo.pClearValues    = clearValues.data();
        postRenderPassBeginInfo.renderPass      = helloVk.getRenderPass();
        postRenderPassBeginInfo.framebuffer     = helloVk.getFramebuffers()[curFrame];
        postRenderPassBeginInfo.renderArea      = {{0, 0}, helloVk.getSize()};

        // Rendering tonemapper
        vkCmdBeginRenderPass(cmdBuf, &postRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        helloVk.drawPost(cmdBuf, timer);
        // Rendering UI
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuf);
        vkCmdEndRenderPass(cmdBuf);
      }

      // Submit for display

      vkEndCommandBuffer(cmdBuf);

      helloVk.submitFrame();

      {
        nvvk::ScopeCommandBuffer commandBuffer(helloVk.getDevice(), helloVk.getQueueFamily(), helloVk.getQueue());
        timer.finishGPU(commandBuffer);
      }

      
      CameraManip.updateAnim();

    }
    catch(const std::system_error& e)
    {
      if(e.code().value() == VK_ERROR_DEVICE_LOST)
      {
#if _WIN32
        MessageBoxA(nullptr, e.what(), "Fatal Error", MB_ICONERROR | MB_OK | MB_DEFBUTTON1);
#endif
      }
      std::cout << e.what() << std::endl;
      return e.code().value();
    }
  }
  end:
  timer.printTimeMS("rasterize");
  timer.printTimeMS("ao_calc");
  timer.printTimeMS("image_synth");
  timer.printTimeMS("post");

  timer.conclude();

  // Cleanup
  vkDeviceWaitIdle(helloVk.getDevice());

  helloVk.destroyResources();
  helloVk.destroy();
  vkctx.deinit();

  glfwDestroyWindow(window);
  glfwTerminate();



  return 0;
}
