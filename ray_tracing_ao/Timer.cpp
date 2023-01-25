/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <iostream>
#include "Timer.h"

void Timer::init(HelloVulkan* _helloVk, const VkCommandBuffer* _cmdBuffer)
{
  /* if(inited)
      return;

  inited = true;*/

  helloVk = _helloVk;
  cmdBuffer = _cmdBuffer;

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(helloVk->getPhysicalDevice(), &deviceProperties);
  const VkPhysicalDeviceLimits& limits = deviceProperties.limits;
  if(!limits.timestampComputeAndGraphics)
  {
    std::cout << "Error in vk::Timer::Timer: Device does not support timestamps." << std::endl;
  }

  VkQueryPoolCreateInfo queryPoolCreateInfo = {};
  queryPoolCreateInfo.sType                 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
  queryPoolCreateInfo.queryType             = VK_QUERY_TYPE_TIMESTAMP;
  queryPoolCreateInfo.queryCount            = maxNumQueries;
  vkCreateQueryPool(helloVk->getDevice(), &queryPoolCreateInfo, nullptr, &queryPool);
  vkCmdResetQueryPool(*cmdBuffer, queryPool, 0, maxNumQueries);

  queryBuffer = new uint64_t[maxNumQueries];

  // Nanoseconds per timestamp step.
  timestampPeriod = double(limits.timestampPeriod);

}

Timer::~Timer()
{
  bool hasPendingQueries = false;
  for(auto& currentFrameData : frameData)
  {
    if(currentFrameData.numQueries != 0)
    {
      hasPendingQueries = true;
    }
  }
  if(hasPendingQueries)
  {
    nvvk::ScopeCommandBuffer commandBuffer(helloVk->getDevice(), helloVk->getQueueFamily(), helloVk->getQueue());
    finishGPU(commandBuffer);
  }

  clear();
  vkDeviceWaitIdle(helloVk->getDevice());
  vkDestroyQueryPool(helloVk->getDevice(), queryPool, nullptr);
  delete[] queryBuffer;
}

void Timer::_onSwapchainRecreated()
{
  baseFrameIdx = std::numeric_limits<uint32_t>::max();
  finishGPU();
}

void Timer::startGPU(const std::string& eventName)
{
  
  const nvvk::SwapChain* swapchain = &helloVk->getSwapChain();
  uint32_t         frameIdx  = swapchain ? swapchain->getActiveImageIndex() : 0;

  if(frameIdx >= frameData.size())
  {
    frameData.resize(frameIdx + 1);
  }

  FrameData& currentFrameData = frameData.at(frameIdx);
  //auto itStart = currentFrameData.queryStartIndices.find(eventName);
  auto itEnd = currentFrameData.queryEndIndices.find(eventName);

  if(baseFrameIdx == std::numeric_limits<uint32_t>::max())
  {
    baseFrameIdx = frameIdx;
  }
  if(itEnd != currentFrameData.queryEndIndices.end())
  {
    addTimesForFrame(frameIdx);
    currentFrameData.queryStart = std::numeric_limits<uint32_t>::max();
    currentFrameData.numQueries = 0;
    currentFrameData.queryStartIndices.clear();
    currentFrameData.queryEndIndices.clear();
    if(frameIdx == baseFrameIdx)
    {
      currentQueryIdx = 0;
    }
  }
  if(currentFrameData.queryStart == std::numeric_limits<uint32_t>::max())
  {
    currentFrameData.queryStart = currentQueryIdx;
  }

  if(currentQueryIdx + 2 > maxNumQueries)
  {
    std::cout << "Error in vk::Timer::Timer: Exceeded maximum number of simultaneous queries." << std::endl;
  }

  vkCmdWriteTimestamp(*cmdBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, currentQueryIdx);
  currentFrameData.queryStartIndices[eventName] = currentQueryIdx;
  currentFrameData.queryEndIndices[eventName]   = currentQueryIdx + 1;
  currentFrameData.numQueries += 2;
  currentQueryIdx += 2;
}

void Timer::endGPU(const std::string& eventName)
{
  const nvvk::SwapChain* swapchain = &helloVk->getSwapChain();
  uint32_t               frameIdx  = swapchain ? swapchain->getActiveImageIndex() : 0;

  FrameData& currentFrameData = frameData.at(frameIdx);
  auto       it               = currentFrameData.queryEndIndices.find(eventName);
  if(it == currentFrameData.queryEndIndices.end())
  {
    std::cout << "Error in vk::Timer::endGPU: No call to 'start' before 'end' for event \"" << it->first 
                               << "\"." << std::endl;
  }
  vkCmdWriteTimestamp(*cmdBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, it->second);
}

void Timer::addTimesForFrame(uint32_t frameIdx, VkCommandBuffer commandBuffer)
{
  FrameData& currentFrameData = frameData.at(frameIdx);
  if(currentFrameData.numQueries == 0)
  {
    return;
  }
  VkResult res = vkGetQueryPoolResults(helloVk->getDevice(), queryPool, currentFrameData.queryStart, currentFrameData.numQueries,
                        currentFrameData.numQueries * sizeof(uint64_t), queryBuffer + currentFrameData.queryStart,
                        sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

  for(auto startIt = currentFrameData.queryStartIndices.begin(); startIt != currentFrameData.queryStartIndices.end(); startIt++)
  {
    auto endIt = currentFrameData.queryEndIndices.find(startIt->first);
    if(endIt == currentFrameData.queryEndIndices.end())
    {
      std::cout << "Error in vk::Timer::addTimesForFrame: No call to 'end' for event \"" << startIt->first << "\"." << std::endl;
    }
    uint64_t startTimestamp         = queryBuffer[startIt->second];
    uint64_t endTimestamp           = queryBuffer[endIt->second];
    auto     elapsedTimeNanoseconds = uint64_t(std::round(double(endTimestamp - startTimestamp) * timestampPeriod));
    elapsedTimeNS[startIt->first] += elapsedTimeNanoseconds;
    numSamples[startIt->first] += 1;
    if(shallStoreFrameTimeList)
    {
      frameTimeList[startIt->first].push_back(elapsedTimeNanoseconds);
    }
  }

  if(commandBuffer == VK_NULL_HANDLE)
  {
    commandBuffer = *cmdBuffer;
  }
  vkCmdResetQueryPool(commandBuffer, queryPool, currentFrameData.queryStart, currentFrameData.numQueries);
}

void Timer::startCPU(const std::string& eventName)
{
  startTimesCPU[eventName] = std::chrono::system_clock::now();
}

void Timer::endCPU(const std::string& eventName)
{
  auto     startTimestamp         = startTimesCPU[eventName];
  auto     endTimestamp           = std::chrono::system_clock::now();
  auto     elapsedTime            = std::chrono::duration_cast<std::chrono::nanoseconds>(endTimestamp - startTimestamp);
  uint64_t elapsedTimeNanoseconds = elapsedTime.count();
  elapsedTimeNS[eventName] += elapsedTimeNanoseconds;
  numSamples[eventName] += 1;
  if(shallStoreFrameTimeList)
  {
    frameTimeList[eventName].push_back(elapsedTimeNanoseconds);
  }
}

void Timer::finishGPU(VkCommandBuffer commandBuffer)
{
  bool hasPendingQueries = false;
  for(auto& currentFrameData : frameData)
  {
    if(currentFrameData.numQueries != 0)
    {
      hasPendingQueries = true;
    }
  }

  if(hasPendingQueries)
  {
    vkDeviceWaitIdle(helloVk->getDevice());
    for(size_t frameIdx = 0; frameIdx < frameData.size(); frameIdx++)
    {
      FrameData& currentFrameData = frameData.at(frameIdx);

      if(currentFrameData.numQueries != 0)
      {
        addTimesForFrame(uint32_t(frameIdx), commandBuffer);
      }

      currentFrameData.queryStart = std::numeric_limits<uint32_t>::max();
      currentFrameData.numQueries = 0;
      currentFrameData.queryStartIndices.clear();
      currentFrameData.queryEndIndices.clear();
      currentQueryIdx = 0;
    }
  }
}

void Timer::clear()
{
  elapsedTimeNS.clear();
  numSamples.clear();
  frameTimeList.clear();
}

bool Timer::getHasEvent(const std::string& eventName)
{
  return elapsedTimeNS.find(eventName) != elapsedTimeNS.end();
}

double Timer::getTimeMS(const std::string& name)
{
  return static_cast<double>(elapsedTimeNS[name]) / static_cast<double>(numSamples[name]) * 1e-6;
}

double Timer::getOptionalTimeMS(const std::string& eventName)
{
  auto it = elapsedTimeNS.find(eventName);
  if(it != elapsedTimeNS.end())
  {
    return getTimeMS(eventName);
  }
  return 0.0;
}

void Timer::printTimeMS(const std::string& name)
{
  double timeMS = getTimeMS(name);
  std::cout << "EVENT - " << name << ": " << timeMS << "ms" << std::endl;
}

void Timer::printTotalAvgTime()
{
  double timeMS = 0.0;
  for(auto& it : numSamples)
  {
    timeMS += static_cast<double>(elapsedTimeNS[it.first]) / static_cast<double>(it.second) * 1e-6;
  }
  std::cout << "TOTAL TIME (avg): " << timeMS << "ms" << std::endl;
}

std::vector<uint64_t> Timer::getFrameTimeList(const std::string& eventName)
{
  if(shallStoreFrameTimeList)
  {
    return frameTimeList[eventName];
  }
  else
  {
    return {elapsedTimeNS[eventName] / numSamples[eventName]};
  }
}
